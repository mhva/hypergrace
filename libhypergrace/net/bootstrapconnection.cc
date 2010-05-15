/*
   Copyright (C) 2009 Anton Mihalyov <anton@glyphsense.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <cassert>
#include <sstream>

#include <debug/debug.hh>

#include <net/socket.hh>
#include <net/hostaddress.hh>
#include <net/inputmiddleware.hh>
#include <net/outputmiddleware.hh>
#include <net/packet.hh>
#include <net/reactor.hh>
#include <net/tcpsocket.hh>

#include "bootstrapconnection.hh"

using namespace Hypergrace;
using namespace Net;


static void noop() {}

class BootstrapConnection::Private :
    public OutputMiddleware,
    public std::enable_shared_from_this<Private>
{
public:
    Private() :
        reactor_(0),
        errorHandler_(&noop),
        succeeded_(false)
    {
    }

    ~Private()
    {
        for (auto packet = packetQueue_.begin(); packet != packetQueue_.end(); ++packet)
            delete *packet;
    }

    void send(Socket &, Packet *)
    {
    }

    void write(Socket &socket)
    {
        int error = 0;
        socklen_t errorSize = sizeof(error);

        int result = getsockopt(socket.fd(), SOL_SOCKET, SO_ERROR, &error, &errorSize);

        if (result == 0 && error == 0) {
            std::shared_ptr<Private> livenessGuarantee = shared_from_this();

            if (inputMw_)
                socket.setInputMiddleware(inputMw_);

            if (outputMw_)
                socket.setOutputMiddleware(outputMw_);

            // Send queued packets
            for (auto packet = packetQueue_.begin(); packet != packetQueue_.end(); ++packet)
                socket.send(*packet);

            packetQueue_.clear();

            succeeded_ = true;
        } else {
            errorHandler_();
            socket.close();
        }
    }

    void shutdown(Socket &)
    {
        if (!succeeded_)
            errorHandler_();
    }

public:
    std::string host_;
    std::string service_;

    Reactor *reactor_;

    Net::InputMiddleware::Pointer inputMw_;
    Net::OutputMiddleware::Pointer outputMw_;

    Delegate::Delegate<void ()> errorHandler_;

    std::deque<Packet *> packetQueue_;

private:
    bool succeeded_;
};

BootstrapConnection::BootstrapConnection() :
    d(new Private())
{
}

BootstrapConnection &
BootstrapConnection::withEndpoint(const std::string &host, const std::string &service)
{
    d->host_ = host;
    d->service_ = service;

    return *this;
}

BootstrapConnection &BootstrapConnection::withEndpoint(const std::string &host, int port)
{
    std::ostringstream oss;
    oss << port;

    d->host_ = host;
    d->service_ = oss.str();

    return *this;
}

BootstrapConnection &BootstrapConnection::withEndpoint(const HostAddress &address)
{
    return withEndpoint(address.address(), address.port());
}

BootstrapConnection &BootstrapConnection::withReactor(Reactor *reactor)
{
    d->reactor_ = reactor;
    return *this;
}

BootstrapConnection &BootstrapConnection::withMiddleware(Net::InputMiddleware *imw)
{
    d->inputMw_.reset(imw);
    return *this;
}

BootstrapConnection &BootstrapConnection::withMiddleware(Net::OutputMiddleware *omw)
{
    d->outputMw_.reset(omw);
    return *this;
}

BootstrapConnection &BootstrapConnection::withMiddleware(Net::InputMiddleware::Pointer imw)
{
    d->inputMw_.swap(imw);
    return *this;
}

BootstrapConnection &BootstrapConnection::withMiddleware(Net::OutputMiddleware::Pointer omw)
{
    d->outputMw_.swap(omw);
    return *this;
}

BootstrapConnection &
BootstrapConnection::withErrorHandler(Delegate::Delegate<void ()> handler)
{
    d->errorHandler_ = handler;
    return *this;
}

BootstrapConnection &BootstrapConnection::withQueuedPacket(Packet *packet)
{
    d->packetQueue_.push_back(packet);
    return *this;
}

void BootstrapConnection::initiate()
{
    if (d->host_.empty() || d->service_.empty()) {
        hDebug() << "Connection cannot be initiated because remote address was not specified";
        d->errorHandler_();
        return;
    }

    if (!d->reactor_ || !d->reactor_->running()) {
        hDebug() << "Connection cannot be initiated without working reactor";
        d->errorHandler_();
        return;
    }

    addrinfo *result = 0;
    addrinfo hints = { 0 };

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // TODO: IPv6 Support
    int error = ::getaddrinfo(d->host_.c_str(), d->service_.c_str(), &hints, &result);

    if (error != 0 || result == 0) {
        hDebug() << "Cannot resolve host" << d->host_ << "(" << d->service_ << ")"
                 << "(" << gai_strerror(error) << ")";
        d->errorHandler_();
        return;
    }

    HostAddress host(*result->ai_addr);
    int socketFd = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (socketFd == -1) {
        hDebug() << "Unable to allocate new socket file descriptor (" << strerror(errno) << ")";
        ::freeaddrinfo(result);
        d->errorHandler_();
        return;
    }

    int flags = fcntl(socketFd, F_GETFL, 0);
    if (flags == -1 || fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        hDebug() << "Unable to set socket into non-blocking mode (" << strerror(errno) << ")";

        ::freeaddrinfo(result);
        ::close(socketFd);

        d->errorHandler_();
        return;
    }

    if (::connect(socketFd, result->ai_addr, result->ai_addrlen) == -1 && errno != EINPROGRESS) {
        hDebug() << "Failed to initiate connection to" << host << "(" << strerror(errno) << ")";

        ::freeaddrinfo(result);
        ::close(socketFd);

        d->errorHandler_();
        return;
    }

    TcpSocket *socket = new TcpSocket(socketFd, host);

    socket->setOutputMiddleware(Net::OutputMiddleware::Pointer(d, d.get()));

    d->reactor_->observe(socket);
    freeaddrinfo(result);
}
