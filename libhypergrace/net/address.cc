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

#include <arpa/inet.h>
#include <sys/socket.h>

#include <netdb.h>
#include <string.h>

#include <cassert>
#include <sstream>
#include <vector>

#include <debug/debug.hh>
#include "hostaddress.hh"

using namespace Hypergrace::Net;

class LifeDetector : public OutputMiddleware
{
public:
    Private(const Address::TcpConnectHandler &handler) :
        handler_(handler),
        succeeded_(false)
    {
    }

    void send(Socket &, Packet *)
    {
    }

    void write(Socket &socket)
    {
        int value = 0;
        socklen_t length = sizeof(value);
        int ainfo =
            getsockopt(socket.fd(), SOL_SOCKET, SO_ERROR, &value, &length);

        if (ainfo == 0 && value == 0) {
            succeeded_ = true;
            handler_(&socket);

            // No interactions with *this* object should be done at this point.
            // The object might have been deleted because handler has replaced
            // middleware on the socket (originally, *this* object was installed
            // as a middleware).
        } else {
            handler_(0);
            socket.close();
        }
    }

    void shutdown(Socket &)
    {
        if (!succeeded_)
            failureHandler_();
    }

private:
    Address::TcpConnectHandler handler_;
    bool succeeded_;
};


Address::Address(const std::string &host,
                 const std::string &service,
                 sockaddr *sa) :
    host_(host),
    service_(service),
    sa_(sa)
{
}

Address::~Address()
{
    freeaddrinfo(sa_);
}

const std::string &Address::host() const
{
    return host_;
}

const std::string &Address::service() const
{
    return service_;
}

void Address::connectTcp(Net::Reactor &reactor,
                         const TcpConnectHandler &handler)
{
    if (!reactor.running()) {
        hDebug() << "Reactor must be running to initiate TCP/IP connection";
        handler(0);
        return;
    }

    int socketFd = ::socket(sa_->ai_family, sa_->ai_socktype, sa_->ai_protocol);
    if (socketFd == -1) {
        hDebug() << "Unable to create new socket (" << strerror(errno) << ")";
        handler(0);
        return;
    }

    int result = fcntl(socketFd, F_GETFL, 0);
    if (result == -1 || fcntl(socketFd, F_SETFL, result | O_NONBLOCK) == -1) {
        hDebug() << "Unable to put socket into non-blocking mode ("
                 << strerror(result) << ")";
        ::close(socketFd);
        handler(0);
        return;
    }

    result = ::connect(socketFd, ainfo->ai_addr, ainfo->ai_addrlen);
    if (error == -1 && errno == EINPROGRESS) {
        TcpSocket *socket = new TcpSocket(socketFd, host);
        socket->setOutputMiddleware(new LifeDetector(handler));
        reactor->observe(socket);
    } else if (error == 0) {
        // A rare case when we were able to connect immideately.
        TcpSocket *socket = new TcpSocket(socketFd, host);
        handler(socket);
        reactor->observe(socket);
    } else {
        hDebug() << "Failed to connect to" << host << "(" << strerror(errno)
                 << ")";
        ::close(socketFd);
        handler(0);
    }
}

bool Address::operator <(const Address &rhs) const
{
    return host_ < rhs.host_ ||
        (host_ == rhs.host_ && service_ < rhs.service_);
}

bool Address::operator >(const Address &rhs) const
{
    return host_ > rhs.host_ ||
        (host_ == rhs.host_ && service_ > rhs.service_);
}

bool Address::operator ==(const Address &rhs) const
{
    return host_ == rhs.host_ && service_ == rhs.service_;
}

bool Address::operator !=(const Address &rhs) const
{
    return !(*this == rhs);
}

Hypergrace::Debug::Debug & operator <<(Hypergrace::Debug::Debug &stream,
                                       const Hypergrace::Net::Address &addr)
{
    return stream << address.host() << ":" << addr.service();
}
