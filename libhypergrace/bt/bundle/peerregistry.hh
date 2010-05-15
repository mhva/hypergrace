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

#ifndef BT_BUNDLE_PEERREGISTRY_HH_
#define BT_BUNDLE_PEERREGISTRY_HH_

#include <memory>
#include <mutex>

#include <bt/types.hh>

namespace Hypergrace { namespace Net { class PeerData; }}
namespace Hypergrace { namespace Net { class Socket; }}


namespace Hypergrace {
namespace Bt {

class PeerRegistry
{
public:
    void registerPeer(std::shared_ptr<PeerData>);
    void unregisterPeer(std::shared_ptr<PeerData>);

    SharedPeerList peerList() const;
    unsigned int peerCount() const;
    bool peerIdRegistered(const PeerId &) const;

public:
    const InternalPeerList &internalPeerList();

private:
    SharedPeerList sharedList_;
    InternalPeerList internalList_;

    mutable std::mutex anchor_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_BUNDLE_PEERREGISTRY_HH_ */
