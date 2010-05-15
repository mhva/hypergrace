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

#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/peerwire/peerdata.hh>

#include <debug/debug.hh>
#include <net/socket.hh>

#include "peerdatacollector.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


PeerDataCollector::PeerDataCollector(
        const TorrentBundle &bundle,
        std::shared_ptr<PeerData> peerData,
        Bt::InputMiddleware::Pointer input) :
    InputMiddleware(input),
    OutputMiddleware(),
    bundle_(bundle),
    peerData_(peerData),
    dropBitfield_(false)
{
}

PeerDataCollector::PeerDataCollector(
        const TorrentBundle &bundle,
        std::shared_ptr<PeerData> peerData,
        Bt::InputMiddleware::Pointer input,
        Net::OutputMiddleware::Pointer output) :
    InputMiddleware(input),
    OutputMiddleware(output),
    bundle_(bundle),
    peerData_(peerData),
    dropBitfield_(false)
{
}

PeerDataCollector::~PeerDataCollector()
{
}

void PeerDataCollector::processMessage(Net::Socket &socket, ChokeMessage &message)
{
    dropBitfield_ = true;

    if (!peerData_->peerChokedUs()) {
        peerData_->setPeerChokedUs(true);

        delegateMessage(socket, message);
    }
}

void PeerDataCollector::processMessage(Net::Socket &socket, UnchokeMessage &message)
{
    dropBitfield_ = true;

    if (peerData_->peerChokedUs()) {
        peerData_->setPeerChokedUs(false);

        delegateMessage(socket, message);
    }
}

void PeerDataCollector::processMessage(Net::Socket &socket, InterestedMessage &message)
{
    dropBitfield_ = true;

    if (!peerData_->peerIsInterested()) {
        peerData_->setPeerIsInterested(true);

        delegateMessage(socket, message);
    }
}

void PeerDataCollector::processMessage(Net::Socket &socket, NotInterestedMessage &message)
{
    dropBitfield_ = true;

    if (peerData_->peerIsInterested()) {
        peerData_->setPeerIsInterested(false);

        delegateMessage(socket, message);
    }
}

void PeerDataCollector::processMessage(Net::Socket &socket, HaveMessage &message)
{
    unsigned int piece = message.field<2>();
    Util::Bitfield &bitfield = peerData_->bitfield();

    dropBitfield_ = true;

    if (piece < bitfield.bitCount() && !bitfield.bit(piece)) {
        bitfield.set(piece);

        delegateMessage(socket, message);
    }
}

void PeerDataCollector::processMessage(Net::Socket &socket, BitfieldMessage &message)
{
    if (dropBitfield_)
        return;

    if (peerData_->bitfield().assign(message.field<2>())) {
        delegateMessage(socket, message);
    } else {
        hDebug() << "Assignment of bitfield failed. Connection with" << socket.remoteAddress()
                 << "will be dropped";
        socket.close();
    }

    dropBitfield_ = true;
}

void PeerDataCollector::processMessage(Net::Socket &socket, RequestMessage &message)
{
    dropBitfield_ = true;

    unsigned int piece = message.field<2>();
    unsigned int offset = message.field<3>();
    unsigned int size = message.field<4>();
    const TorrentModel &model = bundle_.model();

    if (piece >= model.pieceCount() || offset >= model.pieceSize() ||
        offset + size > model.pieceSize())
    {
        hDebug() << "Got a bogus block request from" << socket.remoteAddress();
        return;
    }

    if (!bundle_.state().availablePieces().bit(piece)) {
        hDebug() << "Got a request for a non-existent piece from" << socket.remoteAddress();
        return;
    }

    if (peerData_->weChokedPeer()) {
        hDebug() << "Got a block request from a choked peer" << socket.remoteAddress();
        return;
    }

    if (size > 32768) {
        hDebug() << "A block of size" << size << "has been requested by"
                 << socket.remoteAddress() << "; The block is too large,"
                 << "connection will be dropped";
        socket.close();
        return;
    }

    delegateMessage(socket, message);
}

void PeerDataCollector::processMessage(Net::Socket &socket, PieceMessage &message)
{
    dropBitfield_ = true;

    peerData_->rating().upvote();

    delegateMessage(socket, message);
}

void PeerDataCollector::send(Net::Socket &socket, Net::Packet *packet)
{
    // Catch torrent messages that we are sending to the peer to update
    // state accordingly.
    if (packet->packetClass() == Bt::BitTorrentPacketClass) {
        switch (packet->packetId()) {
        case ChokeMessage::id:
            peerData_->setWeChokedPeer(true);
            break;
        case UnchokeMessage::id:
            peerData_->setWeChokedPeer(false);
            break;
        case InterestedMessage::id:
            peerData_->setWeAreInterested(true);
            break;
        case NotInterestedMessage::id:
            peerData_->setWeAreInterested(false);
            break;
        case PieceMessage::id:
            peerData_->rating().downvote();
            break;
        default:
            break;
        };
    }

    Net::OutputMiddleware::delegateSendEvent(socket, packet);
}
