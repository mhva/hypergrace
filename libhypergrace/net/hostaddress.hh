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

#ifndef NET_HOSTADDRESS_HH_
#define NET_HOSTADDRESS_HH_

#include <string>
#include <netinet/in.h>

namespace Hypergrace { namespace Debug { class Debug; } }
namespace Hypergrace {
namespace Net {

class HostAddress
{
public:
    HostAddress();
    HostAddress(const std::string &, const std::string &);
    HostAddress(const std::string &, int port);
    HostAddress(const sockaddr &);

    std::string address() const;
    int port() const;

    bool valid() const;

    bool operator <(const HostAddress &) const;
    bool operator >(const HostAddress &) const;
    bool operator ==(const HostAddress &) const;
    bool operator !=(const HostAddress &) const;

private:
    void initialize(const std::string &, const std::string &);

private:
    union {
        sockaddr sa;
        sockaddr_in sin4;
        sockaddr_in6 sin6;
    } address_;

    size_t size_;
};

} /* namespace Net */
} /* namespace Hypergrace */

Hypergrace::Debug::Debug &
operator <<(Hypergrace::Debug::Debug &, const Hypergrace::Net::HostAddress &);

#endif /* NET_HOSTADDRESS_HH_ */
