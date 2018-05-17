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

#include <errno.h>

#include <algorithm>
#include <cassert>
#include <sstream>

#include <debug/debug.hh>

#include <net/socket.hh>
#include <net/hostaddress.hh>
#include <net/inputmiddleware.hh>
#include <net/outputmiddleware.hh>
#include <net/packet.hh>
#include <net/reactor.hh>

#include "tcpsocket.hh"

using namespace Hypergrace;
using namespace Net;


TcpSocket::TcpSocket(int socket, const HostAddress &host) :
    Socket(socket, host)
{
}

ssize_t TcpSocket::send(const char *data, size_t size)
{
    ssize_t sent = 0;
    const char *unsent = data;
    const char *end = data + size;

    while (unsent < end && (sent = ::send(fd(), unsent, end - unsent, 0)) > 0)
        unsent += sent;

    assert(unsent <= end);

    if (unsent == end) {
        return size;
    } else if (sent == -1 && errno == EAGAIN) {
        // XXX: Check for EWOULDBLOCK too?
        return unsent - data;
    } else {
        return -1;
    }
}

ssize_t TcpSocket::receive(std::string &buffer, size_t size)
{
    char recvBuffer[size];
    ssize_t received = ::recv(fd(), recvBuffer, size, 0);

    if (received > 0) {
        buffer.append(recvBuffer, received);
        return received;
    } else if (received == 0 || (received == -1 && errno == EAGAIN)) {
        // XXX: Check for EWOULDBLOCK too?
        return 0;
    } else {
        return -1;
    }
}
