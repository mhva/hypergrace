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

#include <map>
#include <memory>
#include <mutex>

#include <bt/bundle/peerregistry.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/peerwire/handshakenegotiator.hh>
#include <bt/peerwire/message.hh>
#include <bt/globaltorrentregistry.hh>

#include <net/socket.hh>
#include <net/tcpsocket.hh>

#include "acceptorservice.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


class AcceptorService::NegotiatorMiddleware : public HandshakeNegotiator
{
public:
    bool processInfoHash(Net::Socket &socket, const InfoHash &receivedHash)
    {
        std::lock_guard<std::mutex> l(mapLock_);

        auto pos = torrentMap_.find(receivedHash);

        if (pos != torrentMap_.end()) {
            unsigned int connectionLimit = (*pos).second.bundle.configuration().connectionLimit();
            const PeerRegistry &peerRegistry = (*pos).second.bundle.state().peerRegistry();

            if (peerRegistry.peerCount() >= connectionLimit)
                return false;

            const PeerId &peerId = GlobalTorrentRegistry::self()->peerId();
            socket.send(new HandshakeMessage(receivedHash, peerId));

            return true;
        } else {
            return false;
        }
    }

    void reconfigurePeer(const PeerSettings &peerSettings)
    {
        std::lock_guard<std::mutex> l(mapLock_);

        auto pos = torrentMap_.find(peerSettings.infoHash);

        // Torrent might have been unregistered while we were waiting
        // for PeerId to arrive.
        if (pos != torrentMap_.end()) {
            // Only proceed if the connected peer is not us
            if (peerSettings.peerId != GlobalTorrentRegistry::self()->peerId())
                (*pos).second.socketHandler(peerSettings);
            else
                ::close(peerSettings.socketFd);
        } else {
            ::close(peerSettings.socketFd);
        }
    }

public:
    struct MapValue {
        const TorrentBundle &bundle;
        AcceptorService::ConnectionHandler socketHandler;
    };

    std::map<InfoHash, MapValue> torrentMap_;
    std::mutex mapLock_;
};

AcceptorService::AcceptorService(int port) :
    Net::AcceptorService(port),
    d(new NegotiatorMiddleware())
{
}

AcceptorService::~AcceptorService()
{
    std::lock_guard<std::mutex> l(d->mapLock_);
    d->torrentMap_.clear();
}

void AcceptorService::acceptTorrentConnections(const TorrentBundle &bundle,
                                               ConnectionHandler handler)
{
    std::lock_guard<std::mutex> l(d->mapLock_);

    const InfoHash &infoHash = bundle.model().hash();

    if (d->torrentMap_.find(infoHash) == d->torrentMap_.end()) {
        NegotiatorMiddleware::MapValue value = { bundle, handler };

        d->torrentMap_.insert(std::make_pair(infoHash, value));
    } else {
        // TODO: hexdump info hash
        hDebug() << "Already accepting sockets for the" << bundle.model().name() << "torrent";
    }
}

void AcceptorService::rejectTorrentConnections(const TorrentBundle &bundle)
{
    std::lock_guard<std::mutex> l(d->mapLock_);
    d->torrentMap_.erase(bundle.model().hash());
}


unsigned int AcceptorService::torrentCount() const
{
    return d->torrentMap_.size();
}

bool AcceptorService::willAccept()
{
    return GlobalTorrentRegistry::self()->connectionCount() <
           GlobalTorrentRegistry::self()->connectionLimit();
}

void AcceptorService::demultiplex(int fd, const Net::HostAddress &address)
{
    Net::Socket *socket = new Net::TcpSocket(fd, address);

    socket->setInputMiddleware(Net::InputMiddleware::Pointer(d));

    reactor().observe(socket);
}
