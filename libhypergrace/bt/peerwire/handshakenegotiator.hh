/*
   Copyright (C) 2010 Anton Mihalyov <anton@glyphsense.com>

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

#ifndef BT_PEERWIRE_HANDSHAKENEGOTIATOR
#define BT_PEERWIRE_HANDSHAKENEGOTIATOR

#include <bt/types.hh>
#include <bt/peerwire/inputmiddleware.hh>

#include <net/inputmiddleware.hh>
#include <net/hostaddress.hh>

namespace Hypergrace { namespace Net { class Socket; }}


namespace Hypergrace {
namespace Bt {

class HandshakeNegotiator : public Net::InputMiddleware
{
protected:
    HandshakeNegotiator();
    virtual ~HandshakeNegotiator();

    virtual bool processInfoHash(Net::Socket &, const InfoHash &) = 0;
    virtual void reconfigurePeer(const PeerSettings &) = 0;

protected:
    void receive(Net::Socket &, std::string &);
    void shutdown(Net::Socket &);

private:
    std::string buffer_;
    bool hashAlreadyDiscovered_;
    PeerSettings peerSettings_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_HANDSHAKENEGOTIATOR */
