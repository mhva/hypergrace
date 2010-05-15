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

#ifndef HTTP_RESPONSEASSEMBLER_HH_
#define HTTP_RESPONSEASSEMBLER_HH_

#include <string>

#include <net/inputmiddleware.hh>
#include <http/inputmiddleware.hh>

namespace Hypergrace {
namespace Http {

class ResponseAssembler : public Net::InputMiddleware
{
public:
    explicit ResponseAssembler(Http::InputMiddleware *);
    explicit ResponseAssembler(Http::InputMiddleware::Pointer);

    void receive(Net::Socket &, std::string &);
    void shutdown(Net::Socket &);

private:
    Http::InputMiddleware::Pointer httpDelegate_;

    std::string buffer_;
    bool assembledResponse_;
};

} /* namespace Http */
} /* namespace Hypergrace */

#endif /* HTTP_RESPONSEASSEMBLER_HH_ */
