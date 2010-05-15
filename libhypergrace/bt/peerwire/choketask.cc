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

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iterator>
#include <vector>

#include <bt/bundle/peerregistry.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/peerwire/message.hh>
#include <bt/peerwire/peerdata.hh>

#include <net/socket.hh>

#include "choketask.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


ChokeTask::ChokeTask(TorrentBundle &bundle) :
    bundle_(bundle),
    unchokedPeerCount_(0)
{
}

void ChokeTask::registerPeer(PeerData *peer)
{
}

void ChokeTask::unregisterPeer(PeerData *peer)
{
    if (!peer->weChokedPeer()) {
        assert(unchokedPeerCount_ > 0);

        unchokedPeerCount_ -= 1;

        doPartialChokeRound();
    }
}

void ChokeTask::notifyPeerBecameInterested(PeerData *peer)
{
    //hDebug() << "Peer" << peer->socket().remoteAddress() << "became interested";

    if (unchokedPeerCount_ < bundle_.configuration().uploadSlotCount()) {
        doPartialChokeRound();
    }
}

void ChokeTask::notifyPeerBecameNotInterested(PeerData *peer)
{
    //hDebug() << "Peer" << peer->socket().remoteAddress() << "became not interested";

    if (!peer->weChokedPeer()) {
        assert(unchokedPeerCount_ > 0);

        unchokedPeerCount_ -= 1;

        doPartialChokeRound();
    }
}

void ChokeTask::doFullChokeRound()
{
    const InternalPeerList &allPeers = bundle_.state().peerRegistry().internalPeerList();

    std::vector<PeerData *> peers;
    unsigned int uploadSlots = bundle_.configuration().uploadSlotCount();

    std::copy_if(
        allPeers.begin(), allPeers.end(), std::back_inserter(peers),
        [](PeerData *p) { return p->peerIsInterested(); }
    );

    if (peers.size() > uploadSlots) {

        std::partial_sort(
            peers.begin(), peers.begin() + uploadSlots - 1, peers.end(),
            [](PeerData *l, PeerData *r) { return l->rating() < r->rating(); }
        );

        // Create a list of choked peers and randomly select one
        // for optimistic unchoke.
        std::vector<PeerData *> chokedPeers;

        std::copy_if(
            peers.begin() + uploadSlots - 1, peers.end(),
            std::back_inserter(chokedPeers),
            [](PeerData *p) { return p->weChokedPeer(); }
        );

        if (!chokedPeers.empty())
            peers[uploadSlots - 1] = chokedPeers[rand() % chokedPeers.size()];
    }

    const unsigned int willUnchoke = std::min(uploadSlots, (unsigned int)peers.size());

    // Unchoked peers with highest rating and one optimistically
    // selected peer.
    for (unsigned int i = 0; i < willUnchoke; ++i) {
        //hDebug() << "Unchoking" << peers[i]->socket().remoteAddress();

        if (peers[i]->weChokedPeer())
            peers[i]->socket().send(new UnchokeMessage());
    }

    unchokedPeerCount_ = willUnchoke;

    // Choke other peers that were previously unchoked.
    for (unsigned int i = willUnchoke; i < peers.size(); ++i) {
        if (!peers[i]->weChokedPeer()) {
            //hDebug() << "Choking" << peers[i]->socket().remoteAddress();

            peers[i]->socket().send(new ChokeMessage());
        }
    }
}

void ChokeTask::doPartialChokeRound()
{
    const InternalPeerList &allPeers = bundle_.state().peerRegistry().internalPeerList();

    std::vector<PeerData *> peers;
    size_t uploadSlots = bundle_.configuration().uploadSlotCount();

    std::copy_if(
        allPeers.begin(), allPeers.end(), std::back_inserter(peers),
        [](PeerData *p) { return p->peerIsInterested() && p->weChokedPeer(); }
    );

    if (unchokedPeerCount_ >= uploadSlots || peers.empty())
        return;

    size_t willUnchoke = std::min<size_t>(uploadSlots - unchokedPeerCount_, peers.size());

    std::partial_sort(
        peers.begin(), peers.begin() + willUnchoke, peers.end(),
        [](PeerData *l, PeerData *r) { return l->rating() < r->rating(); }
    );

    for (size_t i = 0; i < willUnchoke; ++i) {
        //hDebug() << "Unchoking" << peers[i]->socket().remoteAddress();

        peers[i]->socket().send(new UnchokeMessage());
    }

    unchokedPeerCount_ += willUnchoke;
}

void ChokeTask::execute()
{
    doFullChokeRound();
}
