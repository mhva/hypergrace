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

#ifndef BT_PEERWIRE_MESSAGEASSEMBLER_HH_
#define BT_PEERWIRE_MESSAGEASSEMBLER_HH_

#include <net/inputmiddleware.hh>
#include <bt/peerwire/inputmiddleware.hh>

namespace Hypergrace { namespace Net { class Socket; }}


namespace Hypergrace {
namespace Bt {

class MessageAssembler : public Net::InputMiddleware
{
public:
    MessageAssembler(Bt::InputMiddleware::Pointer);

    void receive(Net::Socket &, std::string &);
    void shutdown(Net::Socket &);

private:
    template<typename MessageClass>
    bool delegateMessage(Net::Socket &, size_t);
    bool dispatchMessage(Net::Socket &, size_t);

private:
    std::string buffer_;
    Bt::InputMiddleware::Pointer delegate_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_MESSAGEASSEMBLER_HH_ */
