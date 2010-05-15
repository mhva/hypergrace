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

#include <algorithm>
#include <cassert>

#include <bt/peerwire/peerdata.hh>

#include "peerregistry.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


void PeerRegistry::registerPeer(std::shared_ptr<PeerData> peer)
{
    std::lock_guard<std::mutex> l(anchor_);

    assert(sharedList_.size() == internalList_.size());

    sharedList_.push_back(peer);
    internalList_.push_back(peer.get());
}

void PeerRegistry::unregisterPeer(std::shared_ptr<PeerData> peer)
{
    std::lock_guard<std::mutex> l(anchor_);

    auto peerIt = std::find(sharedList_.begin(), sharedList_.end(), peer);

    if (peerIt != sharedList_.end()) {
        assert(sharedList_.size() == internalList_.size());

        size_t index = peerIt - sharedList_.begin();
        size_t last = sharedList_.size() - 1;

        std::swap(internalList_[index], internalList_[last]);
        std::swap(sharedList_[index], sharedList_[last]);

        internalList_.pop_back();
        sharedList_.pop_back();
    }
}

SharedPeerList PeerRegistry::peerList() const
{
    std::lock_guard<std::mutex> l(anchor_);

    return sharedList_;
}

unsigned int PeerRegistry::peerCount() const
{
    return sharedList_.size();
}

bool PeerRegistry::peerIdRegistered(const PeerId &peerId) const
{
    auto peerIt = std::find_if(
            internalList_.begin(), internalList_.end(),
            [&peerId](PeerData *p) { return p->peerId() == peerId; }
    );

    return peerIt != internalList_.end();
}

const InternalPeerList &PeerRegistry::internalPeerList()
{
    return internalList_;
}
