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
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>

#include <debug/debug.hh>
#include <delegate/delegate.hh>

#include <net/socket.hh>
#include <net/hostaddress.hh>
#include <net/inputmiddleware.hh>
#include <net/outputmiddleware.hh>
#include <net/tcpsocket.hh>

#include "acceptorservice.hh"

using namespace Hypergrace;
using namespace Net;


class AcceptorService::AcceptorMiddleware : public Net::InputMiddleware
{
public:
    AcceptorMiddleware(AcceptorService *service) :
        q(service)
    {
    }

    void receive(Net::Socket &socket, std::string &)
    {
        int fd = -1;
        sockaddr sa;
        socklen_t saSize = sizeof(sa);

        while ((fd = ::accept4(socket.fd(), &sa, &saSize, O_NONBLOCK)) != -1) {
            HostAddress address(sa);

            if (q->willAccept())
                q->demultiplex(fd, address);
            else
                ::close(fd);
        }

        if (errno != EAGAIN || errno != EWOULDBLOCK)
            hDebug() << "Failed to accept peer (" << strerror(errno) << ")";
    }

private:
    AcceptorService *q;
};

AcceptorService::AcceptorService(uint16_t port) :
    port_(port)
{
}

AcceptorService::~AcceptorService()
{
}

bool AcceptorService::start()
{
    if (!reactor_.start()) {
        hSevere() << "Failed to start acceptor service's reactor";
        return false;
    }

    // TODO: IPv6 support
    int fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (fd == -1) {
        hSevere() << "Failed to create a fd file descriptor for accepting new connections"
                  << "(" << strerror(errno) << ")";
        return false;
    }

    int enable = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        hWarning() << "Failed to mark local address as reusable (" << strerror(errno) << ")";
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        hSevere() << "Failed to change socket into non-blocking mode (" << strerror(errno) << ")";

        ::close(fd);
        return false;
    }

    sockaddr_in sin = { 0 };
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port_);

    if (::bind(fd, reinterpret_cast<sockaddr *>(&sin), sizeof(sin)) == -1) {
        hSevere() << "Failed to bind socket (" << strerror(errno) << ")";

        ::close(fd);
        return false;
    }

    if (::listen(fd, 5) == -1) {
        hSevere() << "Failed to listen on socket (" << strerror(errno) << ")";

        ::close(fd);
        return false;
    }

    Net::TcpSocket *socket = new Net::TcpSocket(fd, HostAddress());
    socket->setInputMiddleware(Net::InputMiddleware::Pointer(new AcceptorMiddleware(this)));

    reactor_.observe(socket);

    hDebug() << "Acceptor service started listening on port" << port_;
    return true;
}

void AcceptorService::stop()
{
    reactor_.stop();
}

bool AcceptorService::setPort(uint16_t newPort)
{
    if (port_ != newPort) {
        port_ = newPort;

        stop();
        return start();
    } else {
        return true;
    }
}

bool AcceptorService::running() const
{
    return reactor_.running();
}

Reactor &AcceptorService::reactor()
{
    return reactor_;
}
