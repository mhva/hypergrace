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

#include <unistd.h>

#include <memory>
#include <mutex>
#include <deque>
#include <string>

#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/io/blockcache.hh>
#include <bt/io/diskio.hh>
#include <bt/peerwire/eventhub.hh>
#include <bt/peerwire/downloadtask.hh>
#include <bt/peerwire/interesttask.hh>
#include <bt/peerwire/inputmiddleware.hh>
#include <bt/peerwire/messageassembler.hh>
#include <bt/peerwire/peerdata.hh>
#include <bt/peerwire/peerdatacollector.hh>
#include <bt/globaltorrentregistry.hh>

#include <net/bandwidthallocator.hh>
#include <net/outputmiddleware.hh>
#include <net/reactor.hh>
#include <net/rateaccumulator.hh>
#include <net/tcpsocket.hh>

#include <util/filesystem.hh>

#include "commandtask.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


class CommandTask::Private
{
public:
    Private(TorrentBundle &bundle,
            Net::Reactor &reactor,
            std::shared_ptr<DiskIo> ioThread,
            Net::BandwidthAllocator &globalDownloadAllocator,
            Net::BandwidthAllocator &globalUploadAllocator,
            ChokeTask &chokeTask,
            DownloadTask &downloadTask,
            UploadTask &uploadTask) :
        bundle_(bundle),
        reactor_(reactor),
        ioThread_(ioThread),
        globalDownloadAllocator_(globalDownloadAllocator),
        globalUploadAllocator_(globalUploadAllocator),
        chokeTask_(chokeTask),
        interestTask_(bundle),
        downloadTask_(downloadTask),
        uploadTask_(uploadTask),
        resetRateLimits_(true),
        resetScheduledPiecesMask_(true)
    {
        reactor_.setDownloadRateAccumulator(&downloadRate_);
        reactor_.setUploadRateAccumulator(&uploadRate_);

        Util::FileSystem::createPath(bundle_.bundleDirectory() + "/", 0755);

        serializeBundlePart(TorrentBundle::configurationFilename(), bundle_.configuration());
        serializeBundlePart(TorrentBundle::modelFilename(), bundle_.model());
        serializeBundlePart(TorrentBundle::stateFilename(), bundle_.state());
    }

    void reconfigurePeer(PeerSettings &peerSettings)
    {
        // Drop connection if we already have a peer with the same
        // peer id.
        if (bundle_.state().peerRegistry().peerIdRegistered(peerSettings.peerId)) {
            ::close(peerSettings.socketFd);
            return;
        }

        Net::Socket *socket =
            new Net::TcpSocket(peerSettings.socketFd, peerSettings.address);

        std::shared_ptr<PeerData> peer =
            std::make_shared<PeerData>(bundle_, *socket, peerSettings.peerId);

        // Construct both I/O middleware chains.
        // Input:  i --> MessageAssembler --> PeerDataCollector --> DownloadMediator
        // Output: o --> PeerDataCollector
        EventHub *eventHub =
            new EventHub(bundle_, peer, chokeTask_, interestTask_, downloadTask_, uploadTask_);

        Bt::InputMiddleware::Pointer inputThird(eventHub);

        PeerDataCollector *peerDataCollector =
            new PeerDataCollector(bundle_, peer, inputThird);

        Bt::InputMiddleware::Pointer inputSecond(peerDataCollector);
        Net::InputMiddleware::Pointer inputFirst(new MessageAssembler(inputSecond));
        Net::OutputMiddleware::Pointer outputFirst(inputSecond, peerDataCollector);

        socket->setLocalBandwidthAllocators(&localDownloadAllocator_, &localUploadAllocator_);
        socket->setGlobalBandwidthAllocators(&globalDownloadAllocator_, &globalUploadAllocator_);
        socket->setInputMiddleware(inputFirst);
        socket->setOutputMiddleware(outputFirst);

        reactor_.observe(socket);

        // Let the input middleware process the data we got past the
        // handshake message.
        if (!peerSettings.streamContinuation.empty())
            socket->inputMiddleware().receive(*socket, peerSettings.streamContinuation);
    }

    bool updateFileStorage(const std::string &filename, long long size)
    {
        if (!Util::FileSystem::fileExists(filename)) {
            if (!Util::FileSystem::createPath(filename, 0755))
                return false;

            if (!Util::FileSystem::createFile(filename, 0644))
                return false;
        }

        if (!bundle_.configuration().preallocateStorage()) {
            return true;
        } else {
            long long fileSize = Util::FileSystem::fileSize(filename);

            if (fileSize != -1) {
                if (fileSize == size)
                    return true;
                else
                    return Util::FileSystem::preallocateFile(filename, size);
            } else {
                return false;
            }
        }
    }

    void resetScheduledPiecesMask(const std::set<size_t> &unmaskedFiles)
    {
        const auto &files = bundle_.model().fileList();
        const unsigned int pieceCount = bundle_.model().pieceCount();
        const unsigned int pieceSize = bundle_.model().pieceSize();
        const unsigned int lastPieceSize = bundle_.model().lastPieceSize();

        unsigned int piece = 0;
        unsigned int edgeOffset = 0;
        unsigned int filesUnmaskedOnEdge = 0;

        for (size_t fileIndex = 0; fileIndex < files.size(); ++fileIndex) {
            const FileDescriptor &file = files[fileIndex];

            if (file.size == 0) {
                continue;
            }

            bool unmaskThisFile = unmaskedFiles.find(fileIndex) != unmaskedFiles.end();
            unsigned int currentPieceSize = (piece != pieceCount - 1) ? pieceSize : lastPieceSize;

            filesUnmaskedOnEdge += unmaskThisFile;

            if (file.size + edgeOffset >= currentPieceSize) {
                // Unmask piece on the left edge if it contains a part
                // of at least one file that should be unmasked.
                if (filesUnmaskedOnEdge > 0) {
                    bundle_.state().markPieceAsInteresting(piece);
                } else {
                    bundle_.state().markPieceAsUninteresting(piece);
                }

                ++piece;

                // Proceed to the next file if this file fully fits in
                // the one piece.
                if (file.size + edgeOffset == currentPieceSize) {
                    edgeOffset = 0;
                    continue;
                }

                unsigned long long alignedSize = file.size - (currentPieceSize - edgeOffset);
                unsigned int limit = piece + alignedSize / currentPieceSize;

                assert(limit <= pieceCount);

                // Mask/Unmask all full-sized pieces that the current
                // file covers.
                for (; piece < limit; ++piece) {
                    if (unmaskThisFile) {
                        bundle_.state().markPieceAsInteresting(piece);
                    } else {
                        bundle_.state().markPieceAsUninteresting(piece);
                    }
                }

                edgeOffset = alignedSize % currentPieceSize;

                // Torrent's last piece might never be [un]masked if
                // there're no files behind the current one. [Un]mask
                // the last piece of the current file preemptively.
                if (edgeOffset > 0 && piece < pieceCount) {
                    if (unmaskThisFile) {
                        bundle_.state().markPieceAsInteresting(piece);
                    } else {
                        bundle_.state().markPieceAsUninteresting(piece);
                    }
                }

                filesUnmaskedOnEdge = (edgeOffset > 0 && unmaskThisFile) ? 1 : 0;
            } else {
                // The file fully fits in the current piece.
            }
        }
    }

    bool updateUnmaskedFilesStorage(const std::set<size_t> &unmaskedFiles)
    {
        std::string storageDirectory = bundle_.configuration().storageDirectory();

        for (auto indexIt = unmaskedFiles.begin(); indexIt != unmaskedFiles.end(); ++indexIt) {
            const FileDescriptor &file = bundle_.model().fileList()[*indexIt];

            if (!updateFileStorage(storageDirectory + file.filename, file.size))
                return false;
        }

        return true;
    }

public:
    template<typename T> void serializeBundlePart(const std::string &filename, const T &object)
    {
        ioThread_->writeData(bundle_.bundleDirectory() + "/" + filename, 0, object.toString(),
                Delegate::make(this, &Private::handleBundleSerializationSuccess),
                Delegate::make(this, &Private::handleBundleSerializationFailure)
        );
    }

    void handleBundleSerializationSuccess()
    {
    }

    void handleBundleSerializationFailure()
    {
        // TODO: Stop torrent.
        hSevere() << "Failed to serialize bundle; torrent will be stopped";
    }

public:
    struct FlushResult {
        unsigned int piece;

        enum {
            WriteFailure = 0,
            VerifyFailure,
            Success
        } result;
    };

    void flushPendingPieces()
    {
        BlockCache::CompletePieceList completePieces =
            std::move(downloadTask_.cache().flushComplete());

        for (auto pieceIt = completePieces.begin(); pieceIt != completePieces.end(); ++pieceIt) {
            ioThread_->writeBlocks(bundle_, std::move((*pieceIt).second),
                    Delegate::bind(&Private::notifyWriteSuccess, this, (*pieceIt).first),
                    Delegate::bind(&Private::notifyWriteFailure, this, (*pieceIt).first)
            );
        }
    }

    void processIoResults()
    {
        unsigned int successes = 0;

        for (auto resultIt = ioResults_.begin(); resultIt != ioResults_.end(); ++resultIt) {
            FlushResult &flushResult = *resultIt;

            switch ((*resultIt).result) {
            case FlushResult::WriteFailure:
                GlobalTorrentRegistry::self()->stopTorrent(&bundle_);
                break;
            case FlushResult::VerifyFailure:
                downloadTask_.notifyDownloadedBadPiece(flushResult.piece);
                break;
            case FlushResult::Success:
                bundle_.state().markPieceAsAvailable(flushResult.piece);
                downloadTask_.notifyDownloadedGoodPiece(flushResult.piece);
                interestTask_.notifyDownloadedGoodPiece(flushResult.piece);

                ++successes;
                break;
            default:
                assert(!"Should not be here.");
                break;
            }
        }

        ioResults_.clear();

        if (successes > 0)
            serializeBundlePart(TorrentBundle::stateFilename(), bundle_.state());
    }

    void notifyWriteSuccess(unsigned int piece)
    {
        ioThread_->verifyPiece(bundle_, piece,
                Delegate::make(this, &Private::notifyVerifySuccess),
                Delegate::make(this, &Private::notifyVerifyFailure)
        );
    }

    void notifyWriteFailure(unsigned int piece)
    {
        std::lock_guard<std::mutex> l(anchor_);

        hSevere() << "Failed to write piece" << piece << "on disk";
        ioResults_.push_back(FlushResult { piece, FlushResult::WriteFailure });
    }

    void notifyVerifySuccess(unsigned int piece)
    {
        std::lock_guard<std::mutex> l(anchor_);
        ioResults_.push_back(FlushResult { piece, FlushResult::Success });
    }

    void notifyVerifyFailure(unsigned int piece)
    {
        std::lock_guard<std::mutex> l(anchor_);

        hWarning() << "Piece" << piece << "checksum mismatch";
        ioResults_.push_back(FlushResult { piece, FlushResult::VerifyFailure });
    }

public:
    TorrentBundle &bundle_;
    Net::Reactor &reactor_;
    std::shared_ptr<DiskIo> ioThread_;

    Net::BandwidthAllocator &globalDownloadAllocator_;
    Net::BandwidthAllocator &globalUploadAllocator_;
    Net::BandwidthAllocator localDownloadAllocator_;
    Net::BandwidthAllocator localUploadAllocator_;

    ChokeTask &chokeTask_;
    InterestTask interestTask_;
    DownloadTask &downloadTask_;
    UploadTask &uploadTask_;

    Net::RateAccumulator downloadRate_;
    Net::RateAccumulator uploadRate_;

    volatile bool resetRateLimits_;
    volatile bool resetScheduledPiecesMask_;

    std::deque<FlushResult> ioResults_;
    std::deque<PeerSettings> waitingPeers_;

    std::mutex anchor_;
};

CommandTask::CommandTask(
        TorrentBundle &bundle,
        Net::Reactor &reactor,
        std::shared_ptr<DiskIo> ioThread,
        Net::BandwidthAllocator &dla,
        Net::BandwidthAllocator &ula,
        ChokeTask &chokeTask,
        DownloadTask &downloadTask,
        UploadTask &uploadTask) :
    d(new Private(bundle, reactor, ioThread, dla, ula, chokeTask, downloadTask, uploadTask))
{
}

CommandTask::~CommandTask()
{
    delete d;
}

void CommandTask::notifyRateLimitChanged()
{
    std::lock_guard<std::mutex> l(d->anchor_);

    d->resetRateLimits_ = true;
}

void CommandTask::notifyFileMaskChanged()
{
    std::lock_guard<std::mutex> l(d->anchor_);

    d->resetScheduledPiecesMask_ = true;
}

void CommandTask::notifyPeerConnected(const PeerSettings &peerSettings)
{
    std::lock_guard<std::mutex> l(d->anchor_);

    d->waitingPeers_.push_back(peerSettings);
}

void CommandTask::execute()
{
    std::lock_guard<std::mutex> l(d->anchor_);

    // Update I/O rates.
    d->bundle_.state().setDownloadRate(d->downloadRate_.reset());
    d->bundle_.state().setUploadRate(d->uploadRate_.reset());

    // Reset bandwidth rate limits on local allocators if limits has
    // been changed.
    if (d->resetRateLimits_) {
        d->localDownloadAllocator_.limit(d->bundle_.configuration().downloadRateLimit());
        d->localUploadAllocator_.limit(d->bundle_.configuration().uploadRateLimit());
        d->resetRateLimits_ = false;
    }

    // Update the scheduled pieces mask if the unmasked file set has
    // been changed.
    if (d->resetScheduledPiecesMask_) {
        std::set<size_t> unmaskedFiles = d->bundle_.configuration().unmaskedFiles();

        d->resetScheduledPiecesMask(unmaskedFiles);
        // TODO: Handle failure case.
        d->updateUnmaskedFilesStorage(unmaskedFiles);

        d->resetScheduledPiecesMask_ = false;
    }

    // Renew available bandwith amount.
    d->localDownloadAllocator_.renew();
    d->localUploadAllocator_.renew();

    // Maintain block cache.
    if (d->downloadTask_.cache().completePieceCount() > 0)
        d->flushPendingPieces();

    if (!d->ioResults_.empty())
        d->processIoResults();

    // Receive peers with whom we have completed handshake.
    for (auto peer = d->waitingPeers_.begin(); peer != d->waitingPeers_.end(); ++peer)
        d->reconfigurePeer(*peer);

    d->waitingPeers_.clear();
}
