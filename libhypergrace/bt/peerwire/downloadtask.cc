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

#include <limits.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>

#include <bt/peerwire/message.hh>

#include <bt/bundle/peerregistry.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/io/blockcache.hh>
#include <bt/peerwire/peerdata.hh>
#include <bt/peerwire/pieceadvisor.hh>

#include <debug/debug.hh>
#include <delegate/bind.hh>
#include <net/socket.hh>

#include <util/murmurhash2.hh>

#include "downloadtask.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


class DownloadTask::Private
{
public:
    struct BlockLocation
    {
        inline bool operator ==(const BlockLocation &other) const
        {
            return offset == other.offset && size == other.size;
        }

        inline bool operator !=(const BlockLocation &other) const
        {
            return !(*this == other);
        }

        inline bool operator <(const BlockLocation &other) const
        {
            return offset < other.offset;
        }

        unsigned int offset;
        unsigned int size;
    };

    struct BlockId
    {
        inline bool operator ==(const BlockId &other) const
        {
            return piece == other.piece && location == other.location;
        }

        inline bool operator !=(const BlockId &other) const
        {
            return !(*this == other);
        }

        inline bool operator <(const BlockId &other) const
        {
            return piece < other.piece || (piece == other.piece && location < other.location);
        }

        unsigned int piece;
        BlockLocation location;
    };

    struct BlockIdHash
    {
        inline size_t operator ()(const BlockId &blockId) const
        {
            union {
                unsigned int init[2];
                char bytes[sizeof(unsigned int[2])];
            } key = {{ blockId.piece, blockId.location.offset }};

            return Util::murmurHash2(key.bytes, sizeof(key), 1495723221);
        }
    };

    struct DownloadState : public PeerData::CustomData
    {
        bool ignorePeer;
        Util::Time ignorePeerUntil;

        std::vector<BlockId> requestedBlocks;
        Util::Time lastUploadActivity;

        std::vector<unsigned int> requestCache;
        Util::Time requestCacheUpdateTime;
    };

    struct BlockDescriptor
    {
        std::vector<PeerData *> involvedPeers;
    };

    typedef std::map<unsigned int, std::vector<BlockLocation> > PendingPiecesMap;

public:
    Private(TorrentBundle &bundle) :
        bundle_(bundle),
        peers_(bundle.state().peerRegistry().internalPeerList()),
        pieceAdvisor_(bundle),
        downloadingBlocks_(1000)
    {
    }

    void maintainUploadersState()
    {
        Util::Time now = Util::Time::monotonicTime();

        for (auto peerIt = peers_.begin(); peerIt != peers_.end(); ++peerIt) {
            PeerData *peer = *peerIt;
            auto peerState = peer->getData<DownloadState>(PeerData::DownloadTask);

            if (peer->peerChokedUs())
                continue;

            if (peerState->ignorePeer) {
                if (peerState->ignorePeerUntil > now) {
                } else {
                    peerState->ignorePeer = false;
                    peerState->lastUploadActivity = now;
                    hDebug() << "Forgiven peer" << peer->socket().remoteAddress();
                }
            } else if (peerState->requestedBlocks.size() > 0 &&
                       peerState->lastUploadActivity + Util::Time(0, 0, 60) <= now) {
                // Start ignoring peer if it didn't sent us any requested
                // blocks for a long time.
                cancelDownload(peer);

                peerState->ignorePeer = true;
                peerState->ignorePeerUntil = now + Util::Time(0, 0, 20);

                hDebug() << "Snubbed by peer" << peer->socket().remoteAddress();
            }
        }
    }

    void cancelDownload(PeerData *peer)
    {
        auto &requests = peer->getData<DownloadState>(PeerData::DownloadTask)->requestedBlocks;

        for (auto requestIt = requests.begin(); requestIt != requests.end(); ++requestIt) {
            // Remove uploader from the expected uploaders list of
            // this block.
            auto blockIt = downloadingBlocks_.find(*requestIt);
            assert(blockIt != downloadingBlocks_.end());

            BlockDescriptor &block = (*blockIt).second;

            auto peerIt = std::find(block.involvedPeers.begin(), block.involvedPeers.end(), peer);
            assert(peerIt != block.involvedPeers.end());

            std::iter_swap(peerIt, block.involvedPeers.end() - 1);
            block.involvedPeers.pop_back();

            // If we didn't request this block from anyone else, it
            // shall be returned into the pending blocks list so it
            // can possibly be requested later.
            if (block.involvedPeers.empty()) {
                pendingPieces_[(*requestIt).piece].push_back((*requestIt).location);
                downloadingBlocks_.erase(blockIt);
            }
        }

        requests.clear();
    }

    template<typename OutputIterator>
    void breakPieceIntoBlocks(unsigned int piece, OutputIterator result)
    {
        const unsigned int blockSize = 16 * 1024;
        unsigned int lastPieceIndex = bundle_.model().pieceCount() - 1;
        unsigned int pieceSize = 0;

        if (piece < lastPieceIndex) {
            pieceSize = bundle_.model().pieceSize();
        } else if (piece == lastPieceIndex) {
            unsigned int modulo = bundle_.model().torrentSize() % bundle_.model().pieceSize();
            pieceSize = (modulo != 0) ? modulo : bundle_.model().pieceSize();
        } else {
            assert(!"Piece number is out of range");
            return;
        }

        // Construct full-sized blocks first.
        for (unsigned int piece = 0; piece < pieceSize / blockSize; ++piece) {
            BlockLocation location = { piece * blockSize, blockSize };
            *result++ = location;
        }

        // Construct tail block of variable size if it exists.
        if (pieceSize % blockSize != 0) {
            unsigned int offset = (pieceSize / blockSize) * blockSize;
            unsigned int size = pieceSize - offset;

            BlockLocation location = { offset, size };
            *result++ = location;
        }
    }

    template<typename OutputIterator>
    void enumPieceHolders(unsigned int piece, OutputIterator output)
    {
        for (auto peerIt = peers_.begin(); peerIt != peers_.end(); ++peerIt) {
            PeerData *peer = *peerIt;
            auto peerState = peer->getData<DownloadState>(PeerData::DownloadTask);

            if (!peer->peerChokedUs() && peer->bitfield().bit(piece) && !peerState->ignorePeer)
                *output++ = peer;
        }
    }

    std::vector<BlockLocation> &enqueueNewPiece(unsigned int piece)
    {
        auto result = pendingPieces_.insert(
                std::make_pair(piece, std::vector<BlockLocation>()));
        std::vector<BlockLocation> &blocks = (*result.first).second;

        breakPieceIntoBlocks(piece, std::back_inserter(blocks));
        blockCache_.reserve(piece, blocks.size());

        // Mark the piece as "dirty" so the piece advisor will not try
        // to include it in the future recommendations.
        pieceAdvisor_.markDirty(piece);

        assert(result.second);

        return blocks;
    }

    void pumpInPrioritizedPieces()
    {
        while (prioritizedPieces_.size() < 5) {
            unsigned int piece = pieceAdvisor_.recommend();

            if (piece == (unsigned int)-1)
                break;

            enqueueNewPiece(piece);

            auto result = prioritizedPieces_.insert(piece);
            assert(result.second);
        }
    }

    void sendDeferredRequests()
    {
        // Sort pieces by priority.
        std::deque<PendingPiecesMap::iterator> priorityQueue_;

        for (auto it = pendingPieces_.begin(); it != pendingPieces_.end(); ++it) {
            if (prioritizedPieces_.find((*it).first) != prioritizedPieces_.end())
                priorityQueue_.push_front(it);
            else
                priorityQueue_.push_back(it);
        }

        // Try to send pending requests.
        for (auto it = priorityQueue_.begin(); it != priorityQueue_.end(); ++it) {
            unsigned int piece = (**it).first;
            std::vector<BlockLocation> &blocks = (**it).second;
            std::vector<PeerData *> pieceUploaders;

            enumPieceHolders(piece, std::back_inserter(pieceUploaders));

            std::sort(
                    pieceUploaders.begin(), pieceUploaders.end(),
                    [](PeerData *l, PeerData *r) {
                        auto ls = l->getData<DownloadState>(PeerData::DownloadTask);
                        auto rs = r->getData<DownloadState>(PeerData::DownloadTask);
                        return ls->requestedBlocks.size() < rs->requestedBlocks.size();
                    }
            );

            if (!pieceUploaders.empty()) {
                distributeRequests(pieceUploaders, piece, blocks);

                if (blocks.empty())
                    pendingPieces_.erase(*it);
            } else {
                // Demote piece to avoid clogging prioritized slots
                // with dead pieces.
                prioritizedPieces_.erase(piece);
            }
        }
    }

    void updateRequestCache(PeerData *peer)
    {
        const size_t capacity = 10;

        auto &requestCache = peer->getData<DownloadState>(PeerData::DownloadTask)->requestCache;

        const Util::Bitfield &peerBitfield = peer->bitfield();
        const Util::Bitfield &schedPieces = bundle_.state().scheduledPieces();
        unsigned int pieceCount = bundle_.model().pieceCount();

        unsigned int start = rand() % peerBitfield.byteCount();
        unsigned int byteOffset = start;

        requestCache.clear();
        requestCache.reserve(capacity);

        // Try to find not downloaded/downloading pieces. Walk forward.
        while (byteOffset < schedPieces.byteCount() && requestCache.size() < capacity) {
            if (peerBitfield.byte(byteOffset) == 0 || schedPieces.byte(byteOffset) == 0) {
                ++byteOffset;
                continue;
            }

            unsigned int limit = std::min(byteOffset * CHAR_BIT + CHAR_BIT, pieceCount);

            for (unsigned int piece = byteOffset * CHAR_BIT; piece < limit; ++piece) {
                if (peerBitfield.bit(piece) && schedPieces.bit(piece) &&
                    !pieceAdvisor_.isDirty(piece))
                {
                    requestCache.push_back(piece);
                }
            }

            ++byteOffset;
        }

        // Try to find not downloaded/downloading pieces. Walk backwards.
        byteOffset = start - 1;

        while (byteOffset != (unsigned int)-1 && requestCache.size() < capacity) {
            if (peerBitfield.byte(byteOffset) == 0 || schedPieces.byte(byteOffset) == 0) {
                --byteOffset;
                continue;
            }

            unsigned int limit = byteOffset * CHAR_BIT + CHAR_BIT;

            for (unsigned int piece = byteOffset * CHAR_BIT; piece < limit; ++piece) {
                if (peerBitfield.bit(piece) && schedPieces.bit(piece) &&
                    !pieceAdvisor_.isDirty(piece))
                {
                    requestCache.push_back(piece);
                }
            }

            --byteOffset;
        }
    }

    void feedUploader(PeerData *peer, const Util::Time &now)
    {
        auto peerState = peer->getData<DownloadState>(PeerData::DownloadTask);

        if (peerState->requestCache.empty() ||
            peerState->requestCacheUpdateTime + Util::Time(0, 0, 30) <= now)
        {
            updateRequestCache(peer);
            peerState->requestCacheUpdateTime = now;
        }

        auto pieceIt = peerState->requestCache.begin();
        unsigned int requestLimit = 8;
        unsigned int sentRequests = 0;

        const Util::Bitfield &schedPieces = bundle_.state().scheduledPieces();

        while (pieceIt != peerState->requestCache.end() &&
               peerState->requestedBlocks.size() < requestLimit)
        {
            unsigned int piece = *pieceIt;

            if (!schedPieces.bit(piece)) {
                ++pieceIt;
                continue;
            }

            if (!pieceAdvisor_.isDirty(piece)) {
                // This piece is "clean". It's not being downloaded.
                std::vector<BlockLocation> &blocks = enqueueNewPiece(piece);

                //hDebug() << "Feeding peer with clean" << piece;
                sentRequests += sendBlockRequests(peer, piece, blocks, 0);

                // Don't forget to erase pending request entry if we
                // sent all blocks.
                if (blocks.empty())
                    pendingPieces_.erase(piece);
            } else  {
                // This piece is "dirty". We can only continue if there
                // are some pending requests. No pending requests
                // indicates that we expect the piece to arrive soon or
                // just waiting an I/O manager to flush it to disk.
                assert(!bundle_.state().availablePieces().bit(piece));

                //hDebug() << "Feeding peer with dirty" << piece;
                auto pendingIt = pendingPieces_.find(piece);

                if (pendingIt != pendingPieces_.end()) {
                    sentRequests += sendBlockRequests(peer, piece, (*pendingIt).second, 0);

                    if ((*pendingIt).second.empty())
                        pendingPieces_.erase(pendingIt);
                }
            }

            ++pieceIt;
        }

        if (sentRequests == 0) {
            updateRequestCache(peer);
            peerState->requestCacheUpdateTime = now;
        }
    }

    void feedStarvingUploaders()
    {
        Util::Time now = Util::Time::monotonicTime();

        for (auto peerIt = peers_.begin(); peerIt != peers_.end(); ++peerIt) {
            PeerData *peer = *peerIt;
            auto peerState = peer->getData<DownloadState>(PeerData::DownloadTask);

            if (peerState->requestedBlocks.size() < 5 && !peer->peerChokedUs() &&
                peer->weAreInterested() && !peerState->ignorePeer)
            {
                feedUploader(peer, now);
            }

            //hDebug() << "Peer" << peer->socket().remoteAddress() << "has"
            //         << ((peer->peerChokedUs()) ? "choked us" : "unchoked us")
            //         << "and has" << peerState->requestedBlocks.size() << "requests"
            //         << "in processing";
        }
    }

    void doEndGameRound()
    {
        std::vector<BlockId> targetBlocks;
        targetBlocks.reserve(downloadingBlocks_.size());

        for (auto it = downloadingBlocks_.begin(); it != downloadingBlocks_.end(); ++it)
            targetBlocks.push_back((*it).first);

        for (auto peerIt = peers_.begin(); peerIt != peers_.end(); ++peerIt) {
            PeerData *peer = *peerIt;
            auto peerState = peer->getData<DownloadState>(PeerData::DownloadTask);

            if (peerState->requestedBlocks.size() > 5 || peerState->ignorePeer)
                continue;

            // Shuffling the block list makes the chance to receive
            // duplicate blocks marginally lower.
            std::random_shuffle(targetBlocks.begin(), targetBlocks.end());

            auto blockIt = targetBlocks.begin();
            auto blocksEnd = targetBlocks.end();

            while (blockIt != blocksEnd && peerState->requestedBlocks.size() <= 10) {
                if (!peer->bitfield().bit((*blockIt).piece)) {
                    ++blockIt;
                    continue;
                }

                auto requestIt = std::find(peerState->requestedBlocks.begin(),
                        peerState->requestedBlocks.end(), *blockIt);

                if (requestIt != peerState->requestedBlocks.end()) {
                    ++blockIt;
                    continue;
                }

                sendBlockRequest(peer, *blockIt);

                ++blockIt;
            }
        }
    }

    void distributeRequests(
            std::vector<PeerData *> &peers,
            unsigned int piece,
            std::vector<BlockLocation> &blocks)
    {
        if (peers.empty() || blocks.empty())
            return;

        size_t requestsPerPeer = blocks.size() / peers.size();
        size_t totalSent = 0;

        if (requestsPerPeer == 0)
            requestsPerPeer = 1;

        for (auto peerIt = peers.begin(); peerIt != peers.end(); ++peerIt) {
            if (blocks.empty())
                return;

            totalSent += sendBlockRequests(*peerIt, piece, blocks, requestsPerPeer);
        }

        // Don't start new round if we didn't send any requests in
        // the current round. No sent requests means that uploaders
        // has full request queues.
        if (totalSent > 0)
            distributeRequests(peers, piece, blocks);
    }

    unsigned int sendBlockRequests(
            PeerData *peer,
            unsigned int piece,
            std::vector<BlockLocation> &blocks,
            unsigned int sendLimit)
    {
        auto peerState = peer->getData<DownloadState>(PeerData::DownloadTask);

        size_t freeSlots = 10 - peerState->requestedBlocks.size();
        size_t willSend = (sendLimit > 0)
                ? std::min(std::min((size_t)sendLimit, freeSlots), blocks.size())
                : std::min(freeSlots, blocks.size());

        unsigned int sent;

        for (sent = 0; sent < willSend; ++sent) {
            sendBlockRequest(peer, BlockId { piece, blocks.back() });
            blocks.pop_back();
        }

        return sent;
    }

    void sendBlockRequest(PeerData *peer, const BlockId &blockId)
    {
        peer->getData<DownloadState>(PeerData::DownloadTask)->requestedBlocks.push_back(blockId);
        downloadingBlocks_[blockId].involvedPeers.push_back(peer);

        RequestMessage *message =
            new RequestMessage(blockId.piece, blockId.location.offset, blockId.location.size);

        peer->socket().send(message);
    }

public:
    TorrentBundle &bundle_;
    const InternalPeerList &peers_;
    BlockCache blockCache_;

    PieceAdvisor pieceAdvisor_;

    std::unordered_map<BlockId, BlockDescriptor, BlockIdHash> downloadingBlocks_;
    PendingPiecesMap pendingPieces_;

    std::set<unsigned int> prioritizedPieces_;
};

DownloadTask::DownloadTask(TorrentBundle &bundle) :
    d(new Private(bundle))
{
}

DownloadTask::~DownloadTask()
{
    delete d;
}

void DownloadTask::registerPeer(PeerData *peer)
{
    Private::DownloadState *peerState = new Private::DownloadState();

    peerState->ignorePeer = false;
    peerState->lastUploadActivity = Util::Time::monotonicTime();

    peer->setData(PeerData::DownloadTask, peerState);
}

void DownloadTask::unregisterPeer(PeerData *peer)
{
    d->cancelDownload(peer);
    d->pieceAdvisor_.unreference(peer->bitfield());
}

bool DownloadTask::notifyDownloadedBlock(
        PeerData *peer,
        unsigned int piece,
        unsigned int offset,
        const std::string &data)
{
    Private::BlockId blockId = {piece, {offset, (unsigned int)data.size()}};
    Util::Time now = Util::Time::monotonicTime();

    auto blockIt = d->downloadingBlocks_.find(blockId);
    if (blockIt == d->downloadingBlocks_.end())
        return false;

    Private::BlockDescriptor &block = (*blockIt).second;

    for (auto peerIt = block.involvedPeers.begin(); peerIt != block.involvedPeers.end(); ++peerIt)
    {
        PeerData *involvedPeer = *peerIt;
        auto involvedPeerState =
            involvedPeer->getData<Private::DownloadState>(PeerData::DownloadTask);

        if (peer != involvedPeer)
            involvedPeer->socket().send(new CancelMessage(piece, offset, data.size()));

        auto requestIt = std::find(
                involvedPeerState->requestedBlocks.begin(),
                involvedPeerState->requestedBlocks.end(),
                blockId);

        assert(requestIt != involvedPeerState->requestedBlocks.end());

        std::iter_swap(requestIt, involvedPeerState->requestedBlocks.end() - 1);

        involvedPeerState->requestedBlocks.pop_back();
        involvedPeerState->lastUploadActivity = now;

        if (involvedPeerState->requestedBlocks.size() < 5)
            d->feedUploader(involvedPeer, now);
    }

    d->downloadingBlocks_.erase(blockIt);

    bool pieceCompleted = d->blockCache_.store(piece, offset, data);

    if (pieceCompleted)
        d->prioritizedPieces_.erase(piece);

    return true;
}

void DownloadTask::notifyDownloadedGoodPiece(unsigned int piece)
{
    d->pieceAdvisor_.markClean(piece);

    std::for_each(
        d->peers_.begin(), d->peers_.end(),
        [piece](PeerData *p) { p->socket().send(new HaveMessage(piece)); }
    );

    if (d->bundle_.state().scheduledPieces().enabledCount() == 0) {
        assert(d->downloadingBlocks_.size() == 0);
        assert(d->pendingPieces_.size() == 0);
    }
}

void DownloadTask::notifyDownloadedBadPiece(unsigned int piece)
{
    d->pieceAdvisor_.markClean(piece);
}

void DownloadTask::notifyChokeEvent(PeerData *peer)
{
    d->cancelDownload(peer);
}

void DownloadTask::notifyUnchokeEvent(PeerData *peer)
{
    auto peerState = peer->getData<Private::DownloadState>(PeerData::DownloadTask);

    peerState->ignorePeer = false;
    peerState->lastUploadActivity = Util::Time::monotonicTime();
}

void DownloadTask::notifyHaveEvent(PeerData *, unsigned int piece)
{
    d->pieceAdvisor_.reference(piece);
}

void DownloadTask::notifyBitfieldEvent(PeerData *peer)
{
    d->pieceAdvisor_.reference(peer->bitfield());
}

BlockCache &DownloadTask::cache()
{
    return d->blockCache_;
}

void DownloadTask::execute()
{
    size_t schedPieceCount = d->bundle_.state().scheduledPieces().enabledCount();

    if (schedPieceCount > 0) {
        d->maintainUploadersState();

        d->pumpInPrioritizedPieces();
        d->sendDeferredRequests();
        d->feedStarvingUploaders();

        // If we are running low on scheduled pieces, enter the end
        // game mode.

        // Decrease the end mode threshold to compensate the
        // flush-to-disk delay.
        if (schedPieceCount <= d->blockCache_.storedPieceCount() + 10) {
            hcDebug(d->bundle_.model().name())
                << "End game round:"
                << d->blockCache_.storedPieceCount() << "pieces in transit;"
                << schedPieceCount << "pieces scheduled";

            d->doEndGameRound();
        }
    }
}
