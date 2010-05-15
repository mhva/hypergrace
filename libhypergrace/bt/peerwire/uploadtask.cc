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

#include <bt/bundle/peerregistry.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/io/diskio.hh>
#include <bt/peerwire/message.hh>
#include <bt/peerwire/peerdata.hh>

#include <debug/debug.hh>

#include <net/socket.hh>

#include "uploadtask.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


UploadTask::UploadTask(TorrentBundle &bundle, std::shared_ptr<DiskIo> ioThread) :
    bundle_(bundle),
    ioThread_(ioThread)
{
}

void UploadTask::registerPeer(PeerData *peer)
{
    UploadState *uploadState = new UploadState();
    uploadState->requestsInProcessing = 0;

    peer->setData(PeerData::UploadTask, uploadState);
}

void UploadTask::unregisterPeer(PeerData *peer)
{
    auto uploadState = peer->getData<UploadState>(PeerData::UploadTask);

    std::lock_guard<std::mutex> l(uploadState->anchor);

    std::for_each(
        uploadState->assembledMessages.begin(), uploadState->assembledMessages.end(),
        [](Net::Packet *p) { delete p; }
    );
}

void UploadTask::notifyRequestEvent(
        PeerData *peer,
        unsigned int piece,
        unsigned int offset,
        unsigned int size)
{
    auto uploadState = peer->getData<UploadState>(PeerData::UploadTask);

    if (uploadState->requestsInProcessing < 20) {
        ++uploadState->requestsInProcessing;

        ioThread_->readBlock(
            bundle_, piece, offset, size,
            Delegate::bind(&UploadTask::handleReadSuccess, this, uploadState, piece, offset, _1),
            Delegate::bind(&UploadTask::handleReadFailure, this, uploadState, piece, offset)
        );
    }
}

void UploadTask::notifyCancelEvent(
        PeerData *peer,
        unsigned int piece,
        unsigned int offset,
        unsigned int size)
{
    auto uploadState = peer->getData<UploadState>(PeerData::UploadTask);

    auto predicate = [piece, offset, size](PieceMessage *m) {
        return m->field<2>() == piece && m->field<3>() == offset && m->field<4>().size() == size;
    };

    std::lock_guard<std::mutex> l(uploadState->anchor);

    // We assume that there's only one instance of particular request
    // can be present in the either of lists. In case of duplicate a
    // requests we will waste some amount of bandwidth. Although proper
    // clients should never request duplicate blocks from one peer.
    {
        auto messageIt = std::find_if(
                uploadState->assembledMessages.begin(),
                uploadState->assembledMessages.end(),
                predicate
        );


        if (messageIt != uploadState->assembledMessages.end()) {
            PieceMessage *message = *messageIt;

            uploadState->assembledMessages.erase(messageIt);
            delete message;
            return;
        }
    }

    {
        auto messageIt = std::find_if(
                uploadState->sentMessages.begin(),
                uploadState->sentMessages.end(),
                predicate
        );


        if (messageIt != uploadState->sentMessages.end()) {
            (*messageIt)->discard();
            uploadState->sentMessages.erase(messageIt);
            return;
        }
    }
}

void UploadTask::uploadBlocks(PeerData *peer)
{
    auto uploadState = peer->getData<UploadState>(PeerData::UploadTask);

    std::lock_guard<std::mutex> l(uploadState->anchor);

    while (!uploadState->assembledMessages.empty()) {
        PieceMessage *message = uploadState->assembledMessages.front();

        // FIXME: Catch the occurences when the message is 0. It's
        // never supposed to be 0, but it crashed once.
        assert(message != 0);

        uploadState->assembledMessages.pop_front();
        uploadState->sentMessages.push_back(message);

        peer->socket().send(message);
    }
}

void UploadTask::cancelUpload(PeerData *peer)
{
    auto uploadState = peer->getData<UploadState>(PeerData::UploadTask);

    std::lock_guard<std::mutex> l(uploadState->anchor);

    std::for_each(
        uploadState->assembledMessages.begin(), uploadState->assembledMessages.end(),
        [](PieceMessage *p) { delete p; }
    );

    std::for_each(
        uploadState->sentMessages.begin(), uploadState->sentMessages.end(),
        [](PieceMessage *p) { p->discard(); }
    );

    // Update the number of requests in processing accordingly. We
    // cannot just reset it to 0 because there might be some requests
    // that are being processed by the I/O thread.
    uploadState->requestsInProcessing -=
        uploadState->assembledMessages.size() + uploadState->sentMessages.size();

    uploadState->assembledMessages.clear();
    uploadState->sentMessages.clear();
}

void UploadTask::handleReadSuccess(
        std::shared_ptr<UploadState> uploadState,
        unsigned int piece,
        unsigned int offset,
        std::string data)
{
    PieceMessage *message = new PieceMessage(piece, offset, data);

    message->onSent = Delegate::bind(&UploadTask::handleMessageSentEvent, this, uploadState);

    std::lock_guard<std::mutex> l(uploadState->anchor);
    uploadState->assembledMessages.push_back(message);
}

void UploadTask::handleReadFailure(
        std::shared_ptr<UploadState> uploadState,
        unsigned int piece,
        unsigned int offset)
{
    assert(uploadState->requestsInProcessing > 0);

    hWarning() << "Failed to read piece" << piece << "at offset" << offset << "from disk";

    --uploadState->requestsInProcessing;
}

void UploadTask::handleMessageSentEvent(std::shared_ptr<UploadState> uploadState)
{
    std::lock_guard<std::mutex> l(uploadState->anchor);

    // Send order is never violated thus it's safe to assume that the
    // sent message would be in front.
    uploadState->sentMessages.pop_front();

    assert(uploadState->requestsInProcessing > 0);
    --uploadState->requestsInProcessing;
}

void UploadTask::execute()
{
    auto allPeers = bundle_.state().peerRegistry().internalPeerList();

    for (auto peerIt = allPeers.begin(); peerIt != allPeers.end(); ++peerIt) {
        PeerData *peer = *peerIt;
        auto uploadState = peer->getData<UploadState>(PeerData::UploadTask);

        if (peer->weChokedPeer()) {
            if (!uploadState->assembledMessages.empty() || !uploadState->sentMessages.empty())
                cancelUpload(peer);
        } else {
            if (!uploadState->assembledMessages.empty())
                uploadBlocks(peer);
        }
    }
}
