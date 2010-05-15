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

#include <sys/socket.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <iterator>
#include <memory>

#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/peerwire/handshakenegotiator.hh>
#include <bt/peerwire/message.hh>
#include <bt/globaltorrentregistry.hh>

#include <delegate/connectable.hh>
#include <debug/debug.hh>

#include <net/bootstrapconnection.hh>
#include <net/reactor.hh>

#include <util/backtrace.hh>

#include "connectioninitiator.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


class NegotiatorMiddleware : public HandshakeNegotiator
{
public:
    NegotiatorMiddleware(const TorrentBundle &bundle) :
        bundle_(bundle)
    {
    }

    bool processInfoHash(Net::Socket &, const InfoHash &receivedHash)
    {
        if (receivedHash == bundle_.model().hash()) {
            return true;
        } else {
            onConnectFailure();
            return false;
        }
    }

    void reconfigurePeer(const PeerSettings &peerSettings)
    {
        onConnectSuccess(peerSettings);
    }

    Delegate::Delegate<void (PeerSettings)> onConnectSuccess;
    Delegate::Delegate<void ()> onConnectFailure;

private:
    const TorrentBundle &bundle_;
};

ConnectionInitiator::ConnectionInitiator(const TorrentBundle &bundle, Net::Reactor &reactor) :
    bundle_(bundle),
    reactor_(reactor)
{
}

void ConnectionInitiator::enqueuePeers(const PeerList &newPeers)
{
    for (auto peer = newPeers.begin(); peer != newPeers.end(); ++peer) {
        auto it = knownPeers_.lower_bound(*peer);

        if (it == knownPeers_.end() || (*it).first != *peer) {
            PeerDescriptor peerSettings;

            peerSettings.address = *peer;
            peerSettings.connecting = false;
            peerSettings.retryCount = 0;

            knownPeers_.insert(it, std::make_pair(*peer, peerSettings));
        }
    }
}

unsigned int ConnectionInitiator::availableSlots() const
{
    unsigned int globalLimit = GlobalTorrentRegistry::self()->connectionLimit();
    unsigned int localLimit = bundle_.configuration().connectionLimit();

    unsigned int localConnections = bundle_.state().peerRegistry().peerCount();
    unsigned int globalConnections = GlobalTorrentRegistry::self()->connectionCount();

    if (localConnections >= localLimit)
        return 0;

    if (globalConnections < globalLimit) {
        unsigned int localSlots = localLimit - localConnections;
        unsigned int globalSlots = globalLimit - globalConnections;

        return std::min(localSlots, globalSlots);
    } else {
        return 0;
    }
}

void ConnectionInitiator::connectToPeer(const Net::HostAddress &address)
{
    const InfoHash &hash = bundle_.model().hash();
    const PeerId &peerId = GlobalTorrentRegistry::self()->peerId();

    HandshakeMessage *packet = new HandshakeMessage(hash, peerId);
    NegotiatorMiddleware *negotiator = new NegotiatorMiddleware(bundle_);

    negotiator->onConnectSuccess =
        Delegate::bind(&ConnectionInitiator::handleConnectSuccess, this, address, _1);

    negotiator->onConnectFailure =
        Delegate::bind(&ConnectionInitiator::handleConnectFailure, this, address);

    Net::BootstrapConnection()
        .withReactor(&reactor_)
        .withEndpoint(address)
        .withMiddleware(negotiator)
        .withQueuedPacket(packet)
        .withErrorHandler(negotiator->onConnectFailure)
        .initiate();
}

void ConnectionInitiator::handleConnectSuccess(Net::HostAddress address, PeerSettings peerSettings)
{
    assert(knownPeers_.find(address) != knownPeers_.end());
    knownPeers_.erase(address);

    onConnectedToPeer(peerSettings);
}

void ConnectionInitiator::handleConnectFailure(Net::HostAddress address)
{
    auto peerIt = knownPeers_.find(address);
    assert(peerIt != knownPeers_.end());

    PeerDescriptor &peer = (*peerIt).second;

    peer.connecting = false;
    peer.retryCount += 1;

    if (peer.retryCount < 3)
        peer.retryTime = Util::Time::monotonicTime() + Util::Time(0, 3, 0);
    else
        knownPeers_.erase(peerIt);
}

void ConnectionInitiator::execute()
{
    if (knownPeers_.empty())
        return;

    unsigned int freeSlots = availableSlots();

    if (freeSlots == 0)
        return;

    Util::Time now = Util::Time::monotonicTime();
    auto peer = knownPeers_.begin();

    while (peer != knownPeers_.end() && freeSlots > 0) {
        PeerDescriptor &peerDescriptor = (*peer).second;

        if (!peerDescriptor.connecting && peerDescriptor.retryTime <= now) {
            peerDescriptor.connecting = true;
            connectToPeer(peerDescriptor.address);

            --freeSlots;
        }

        ++peer;
    }
}
