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

#ifndef HTTP_REQUEST_HH_
#define HTTP_REQUEST_HH_

#include <http/uri.hh>
#include <net/packet.hh>

namespace Hypergrace { namespace Http { class Uri; } }

namespace Hypergrace {
namespace Http {

class Request : public Net::Packet
{
public:
    explicit Request(const Uri &);
    explicit Request(const char *);
    ~Request() = default;

    void appendHeader(const char *, const std::string &);
    void setHeader(const char *, const std::string &);

    std::string serialize() const;

private:
    Uri uri_;
};

} /* namespace Http */
} /* namespace Hypergrace */

#endif /* NET_HTTP_REQUEST_HH_ */
