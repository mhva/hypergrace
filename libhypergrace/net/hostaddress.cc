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
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cassert>
#include <sstream>
#include <vector>

#include <debug/debug.hh>
#include "hostaddress.hh"

using namespace Hypergrace::Net;


HostAddress::HostAddress() :
    size_(0)
{
    memset(&address_, 0, sizeof(address_));
}

HostAddress::HostAddress(const std::string &ipaddr, const std::string &service) :
    size_(0)
{
    initialize(ipaddr, service);
}

HostAddress::HostAddress(const std::string &ipaddr, int port) :
    size_(0)
{
    std::ostringstream service;
    service << port;

    initialize(ipaddr, service.str());
}

HostAddress::HostAddress(const sockaddr &sa) :
    size_(0)
{
    switch (sa.sa_family) {
    case AF_INET:
        memcpy(&address_.sin4, &sa, sizeof(sockaddr_in));
        size_ = sizeof(sockaddr_in);
        break;
    case AF_INET6:
        memcpy(&address_.sin6, &sa, sizeof(sockaddr_in6));
        size_ = sizeof(sockaddr_in6);
        break;
    default:
        memset(&address_, 0, sizeof(address_));
        break;
    }
}

void HostAddress::initialize(const std::string &ipaddr, const std::string &service)
{
    addrinfo *ai;
    int error = getaddrinfo(ipaddr.c_str(), service.c_str(), 0, &ai);

    if (error != 0) {
        hDebug() << "Failed to resolve address" << ipaddr << ":" << service
                 << "(" << gai_strerror(error) << ")";

        memset(&address_, 0, sizeof(address_));
        size_ = 0;
        return;
    }

    switch (ai->ai_family) {
        case AF_INET:
            memcpy(&address_.sin4, ai->ai_addr, ai->ai_addrlen);
            size_ = ai->ai_addrlen;
            break;
        case AF_INET6:
            memcpy(&address_.sin6, ai->ai_addr, ai->ai_addrlen);
            size_ = ai->ai_addrlen;
            break;
        default:
            memset(&address_, 0, sizeof(address_));
            size_ = 0;
            break;
    }

    freeaddrinfo(ai);
}

bool HostAddress::valid() const
{
    return size_ != 0;
}

std::string HostAddress::address() const
{
    if (!valid())
        return "<addr_invalid>";

    char buffer[INET6_ADDRSTRLEN];
    const char *result = 0;

    switch (address_.sa.sa_family) {
    case AF_INET:
        result = inet_ntop(AF_INET, &address_.sin4.sin_addr, buffer, sizeof(buffer));
        break;
    case AF_INET6:
        result = inet_ntop(AF_INET6, &address_.sin6.sin6_addr, buffer, sizeof(buffer));
        break;
    default:
        assert(!"Should not be here");
        return "<addr_invalid>";
    }

    if (result != 0) {
        return buffer;
    } else {
        std::ostringstream out;
        out << "<addr_conv_error:" << strerror(errno) << ">";
        return out.str();
    }
}

int HostAddress::port() const
{
    if (!valid())
        return -1;

    switch (address_.sa.sa_family) {
    case AF_INET:
        return ntohs(address_.sin4.sin_port);
    case AF_INET6:
        return ntohs(address_.sin6.sin6_port);
    default:
        assert(!"Should not be here");
        return -1;
    }
}

bool HostAddress::operator <(const HostAddress &right) const
{
    return memcmp(&address_, &right.address_, size_) < 0;
}

bool HostAddress::operator >(const HostAddress &right) const
{
    return memcmp(&address_, &right.address_, size_) > 0;
}

bool HostAddress::operator ==(const HostAddress &right) const
{
    return memcmp(&address_, &right.address_, size_) == 0;
}

bool HostAddress::operator !=(const HostAddress &right) const
{
    return !(*this == right);
}

Hypergrace::Debug::Debug &
operator <<(Hypergrace::Debug::Debug &stream, const Hypergrace::Net::HostAddress &address)
{
    return stream << address.address() << ":" << address.port();
}
