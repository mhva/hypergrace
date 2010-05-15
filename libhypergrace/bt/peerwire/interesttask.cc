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

#include <bt/bundle/peerregistry.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/peerwire/message.hh>
#include <bt/peerwire/peerdata.hh>
#include <bt/peerwire/pieceadvisor.hh>
#include <bt/types.hh>

#include <net/socket.hh>

#include <util/bitfield.hh>

#include "interesttask.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


InterestTask::InterestTask(TorrentBundle &bundle) :
    bundle_(bundle)
{
}

void InterestTask::registerPeer(PeerData *peer)
{
    const Util::Bitfield &bitfield = bundle_.state().availablePieces();
    InterestState *state = new InterestState();

    state->piecesWanted = 0;

    peer->setData(PeerData::InterestTask, state);

    BitfieldMessage *message = new BitfieldMessage(
            std::string((char *)bitfield.cstr(), bitfield.byteCount())
    );

    peer->socket().send(message);
}

void InterestTask::unregisterPeer(PeerData *peer)
{
}

void InterestTask::notifyHaveEvent(PeerData *peer, unsigned int piece)
{
    const Util::Bitfield &schedPieces = bundle_.state().scheduledPieces();

    if (schedPieces.bit(piece)) {
        auto interestState = peer->getData<InterestState>(PeerData::InterestTask);

        interestState->piecesWanted += 1;

        if (!peer->weAreInterested())
            peer->socket().send(new InterestedMessage());
    }
}

void InterestTask::notifyBitfieldEvent(PeerData *peer)
{
    auto interestState = peer->getData<InterestState>(PeerData::InterestTask);

    updateWantedPieceCount(peer);

    if (interestState->piecesWanted > 0)
        peer->socket().send(new InterestedMessage());
    else
        peer->socket().send(new NotInterestedMessage());
}

void InterestTask::notifyDownloadedGoodPiece(unsigned int piece)
{
    const InternalPeerList &peers = bundle_.state().peerRegistry().internalPeerList();

    for (auto peerIt = peers.begin(); peerIt != peers.end(); ++peerIt) {
        PeerData *peer = *peerIt;
        auto interestState = peer->getData<InterestState>(PeerData::InterestTask);

        if (peer->bitfield().bit(piece)) {
            assert(interestState->piecesWanted > 0);
            assert(peer->weAreInterested());

            interestState->piecesWanted -= 1;

            if (interestState->piecesWanted == 0)
                peer->socket().send(new NotInterestedMessage());
        }
    }
}

void InterestTask::reconsiderInterest()
{
    const InternalPeerList &peers = bundle_.state().peerRegistry().internalPeerList();

    for (auto peerIt = peers.begin(); peerIt != peers.end(); ++peerIt) {
        PeerData *peer = *peerIt;
        auto interestState = peer->getData<InterestState>(PeerData::InterestTask);

        updateWantedPieceCount(peer);

        if (interestState->piecesWanted > 0 && !peer->weAreInterested())
            peer->socket().send(new InterestedMessage());
        else if (interestState->piecesWanted == 0 && peer->weAreInterested());
            peer->socket().send(new NotInterestedMessage());
    }
}

void InterestTask::updateWantedPieceCount(PeerData *peer)
{
    auto interestState = peer->getData<InterestState>(PeerData::InterestTask);
    const Util::Bitfield &schedPieces = bundle_.state().scheduledPieces();
    const Util::Bitfield &peerBitfield = peer->bitfield();

    interestState->piecesWanted = 0;

    for (size_t piece = 0; piece < schedPieces.bitCount(); ++piece) {
        if (peerBitfield.bit(piece) && schedPieces.bit(piece))
            interestState->piecesWanted += 1;
    }
}
