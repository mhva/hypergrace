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

#include <cassert>
#include <utility>

#include <bt/bundle/peerregistry.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/peerwire/choketask.hh>
#include <bt/peerwire/interesttask.hh>
#include <bt/peerwire/downloadtask.hh>
#include <bt/peerwire/peerdata.hh>
#include <bt/peerwire/uploadtask.hh>
#include <bt/peerwire/pieceadvisor.hh>
#include <bt/peerwire/message.hh>
#include <bt/globaltorrentregistry.hh>

#include <net/socket.hh>

#include "eventhub.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


EventHub::EventHub(
        TorrentBundle &bundle,
        std::shared_ptr<PeerData> peer,
        ChokeTask &chokeTask,
        InterestTask &interestTask,
        DownloadTask &downloadTask,
        UploadTask &uploadTask) :
    bundle_(bundle),
    peer_(peer),
    chokeTask_(chokeTask),
    interestTask_(interestTask),
    downloadTask_(downloadTask),
    uploadTask_(uploadTask),
    wantedPiecesFromPeer_(0)
{
    bundle_.state().peerRegistry().registerPeer(peer_);

    interestTask_.registerPeer(peer_.get());
    chokeTask_.registerPeer(peer_.get());
    downloadTask_.registerPeer(peer_.get());
    uploadTask_.registerPeer(peer_.get());
}

EventHub::~EventHub()
{
    bundle_.state().peerRegistry().unregisterPeer(peer_);

    uploadTask_.unregisterPeer(peer_.get());
    downloadTask_.unregisterPeer(peer_.get());
    chokeTask_.unregisterPeer(peer_.get());
    interestTask_.unregisterPeer(peer_.get());
}

void EventHub::processMessage(Net::Socket &, ChokeMessage &)
{
    downloadTask_.notifyChokeEvent(peer_.get());
}

void EventHub::processMessage(Net::Socket &, UnchokeMessage &)
{
    downloadTask_.notifyUnchokeEvent(peer_.get());
}

void EventHub::processMessage(Net::Socket &, InterestedMessage &)
{
    chokeTask_.notifyPeerBecameInterested(peer_.get());
}

void EventHub::processMessage(Net::Socket &, NotInterestedMessage &)
{
    chokeTask_.notifyPeerBecameNotInterested(peer_.get());
}

void EventHub::processMessage(Net::Socket &, HaveMessage &message)
{
    unsigned int piece = message.field<2>();

    interestTask_.notifyHaveEvent(peer_.get(), piece);
    downloadTask_.notifyHaveEvent(peer_.get(), piece);
}

void EventHub::processMessage(Net::Socket &, BitfieldMessage &)
{
    interestTask_.notifyBitfieldEvent(peer_.get());
    downloadTask_.notifyBitfieldEvent(peer_.get());
}

void EventHub::processMessage(Net::Socket &, PieceMessage &message)
{
    unsigned int piece = message.field<2>();
    unsigned int offset = message.field<3>();
    const std::string &payload = message.field<4>();

    bool good = downloadTask_.notifyDownloadedBlock(peer_.get(), piece, offset, payload);

    if (good)
        bundle_.state().updateDownloaded(payload.size());
}

void EventHub::processMessage(Net::Socket &, RequestMessage &message)
{
    unsigned int piece = message.field<2>();
    unsigned int offset = message.field<3>();
    unsigned int size = message.field<4>();

    uploadTask_.notifyRequestEvent(peer_.get(), piece, offset, size);
}

void EventHub::processMessage(Net::Socket &, CancelMessage &message)
{
    unsigned int piece = message.field<2>();
    unsigned int offset = message.field<3>();
    unsigned int size = message.field<4>();

    uploadTask_.notifyCancelEvent(peer_.get(), piece, offset, size);
}
