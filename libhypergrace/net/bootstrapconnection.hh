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

#ifndef NET_CONNECTIONINITIATOR_CC_
#define NET_CONNECTIONINITIATOR_CC_

#include <deque>
#include <string>

#include <delegate/delegate.hh>
#include <net/inputmiddleware.hh>
#include <net/outputmiddleware.hh>
#include <util/shared.hh>

namespace Hypergrace { namespace Net { class Socket; }}
namespace Hypergrace { namespace Net { class Packet; }}
namespace Hypergrace { namespace Net { class Reactor; }}
namespace Hypergrace { namespace Net { class HostAddress; }}


namespace Hypergrace {
namespace Net {

class BootstrapConnection
{
public:
    BootstrapConnection();

    BootstrapConnection &withEndpoint(const std::string &, const std::string &);
    BootstrapConnection &withEndpoint(const std::string &, int);
    BootstrapConnection &withEndpoint(const HostAddress &);
    BootstrapConnection &withReactor(Reactor *);
    BootstrapConnection &withMiddleware(Net::InputMiddleware *);
    BootstrapConnection &withMiddleware(Net::OutputMiddleware *);
    BootstrapConnection &withMiddleware(Net::InputMiddleware::Pointer);
    BootstrapConnection &withMiddleware(Net::OutputMiddleware::Pointer);
    BootstrapConnection &withErrorHandler(Delegate::Delegate<void ()>);
    BootstrapConnection &withQueuedPacket(Packet *);

    void initiate();

private:
    HG_DECLARE_SHARED_PRIVATE
};

} /* namespace Net */
} /* namespace Hypergrace */

#endif /* NET_CONNECTIONINITIATOR_CC_ */
