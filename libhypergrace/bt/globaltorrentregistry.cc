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
#include <cstdlib>
#include <stdexcept>
#include <random>

#include <debug/debug.hh>

#include <bt/announce/announcescheduler.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/bundle/trackerregistry.hh>
#include <bt/io/diskio.hh>
#include <bt/peerwire/commandtask.hh>
#include <bt/peerwire/connectioninitiator.hh>
#include <bt/peerwire/choketask.hh>
#include <bt/peerwire/downloadtask.hh>
#include <bt/peerwire/uploadtask.hh>
#include <bt/bundle/savestatetask.hh>

#include <delegate/delegate.hh>

#include <net/reactor.hh>

#include <http/uri.hh>

#include <util/sha1hash.hh>
#include <util/time.hh>

#include "globaltorrentregistry.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


GlobalTorrentRegistry::GlobalTorrentRegistry() :
    defaultIoThread_(new DiskIo()),
    acceptorService_(6881),
    port_(6881),
    connectionLimit_(180),
    dloadRateLimit_(0),
    uloadRateLimit_(0),
    cacheSizeLimit_(4 * 1024 * 1024)
{
    srand(Util::Time::monotonicTime().toMilliseconds());

    unsigned int n = rand();

    peerId_ = Util::Sha1Hash::oneshot((const char *)&n, sizeof(n));

    // FIXME: Version number is hardcoded
    peerId_[0] = '-'; peerId_[1] = 'H'; peerId_[2] = 'G'; peerId_[7] = '-';
    peerId_[3] = '0'; peerId_[4] = '0'; peerId_[5] = '1'; peerId_[6] = '0';
}

GlobalTorrentRegistry::~GlobalTorrentRegistry()
{
}

bool GlobalTorrentRegistry::createTorrent(TorrentBundle *bundle)
{
    std::lock_guard<std::mutex> l(anchor_);

    // Refuse to accept torrent if the torrent with similar info-hash
    // already exists.
    for (auto it = torrents_.begin(); it != torrents_.end(); ++it) {
        if (bundle->model().hash() == (*it).second.bundle->model().hash()) {
            hWarning() << "Failed to create torrent because torrent with name"
                       << (*it).second.bundle->model().name() << "has similar info-hash";
            return false;
        }
    }

    const std::string &torrentName = bundle->model().name();

    if (bundle->state().trackerRegistry().tiers()->empty()) {
        populateTrackerRegistry(*bundle);
        hcDebug(torrentName) << "Restored torrent registry from the original announce list";
    } else {
        hcDebug(torrentName) << "Restored torrent registry from the previous session data";
    }

    Torrent torrent;

    torrent.bundle = bundle;
    torrent.reactor = new Net::Reactor();
    torrent.commandTask = 0;

    torrents_.insert(std::make_pair(bundle, torrent));

    return true;
}

void GlobalTorrentRegistry::deleteTorrent(TorrentBundle *bundle)
{
    std::lock_guard<std::mutex> l(anchor_);

    auto torrentIt = torrents_.find(bundle);

    if (torrentIt == torrents_.end()) {
        hDebug() << "Failed to delete torrent because doesn't exist in the torrent registry";
        return;
    }

    stopTorrent(bundle);

    Torrent &torrent = (*torrentIt).second;

    torrent.reactor->stop();

    // We don't need to delete CommandTask explicitly because reactor
    // will do this for us automatically.
    delete torrent.reactor;
    delete torrent.bundle;

    torrents_.erase(torrentIt);
}

bool GlobalTorrentRegistry::startTorrent(TorrentBundle *bundle)
{
    std::lock_guard<std::mutex> l(anchor_);

    auto torrentIt = torrents_.find(bundle);

    if (torrentIt == torrents_.end()) {
        hWarning() << "Failed to start torrent because it doesn't exist in the torrent registry";
        return false;
    }

    if (!acceptorService_.running() && !acceptorService_.start()) {
        hSevere() << "Unable to start acceptor service for receiving incoming connections";
        return false;
    }

    Torrent &torrent = (*torrentIt).second;

    if (torrent.reactor->running()) {
        hSevere() << "Torrent is already running";
        return false;
    }

    // Initialize tasks if torrent was never started before.
    if (torrent.commandTask == 0) {
        TorrentBundle *bundle = torrent.bundle;
        Net::Reactor *reactor = torrent.reactor;

        AnnounceScheduler *announceScheduler = new AnnounceScheduler(*bundle, *reactor);
        ConnectionInitiator *connectionInitiator = new ConnectionInitiator(*bundle, *reactor);
        SaveStateTask *saveStateTask = new SaveStateTask(*bundle);
        ChokeTask *chokeTask = new ChokeTask(*bundle);
        DownloadTask *downloadTask = new DownloadTask(*bundle);
        UploadTask *uploadTask = new UploadTask(*bundle, defaultIoThread_);
        CommandTask *commandTask = new CommandTask(*bundle, *reactor, defaultIoThread_,
                downloadAllocator_, uploadAllocator_, *chokeTask, *downloadTask, *uploadTask);

        // Make announce service submit peers directly to the
        // connection initiator service.
        announceScheduler->onPeersAvailable =
            Delegate::bind(&ConnectionInitiator::enqueuePeers, connectionInitiator, _1);

        connectionInitiator->onConnectedToPeer =
            Delegate::bind(&CommandTask::notifyPeerConnected, commandTask, _1);

        reactor->scheduleTask(commandTask, 1 * 1000);
        reactor->scheduleTask(downloadTask, 1 * 1000);
        reactor->scheduleTask(announceScheduler, 10 * 1000);
        reactor->scheduleTask(connectionInitiator, 10 * 1000);
        reactor->scheduleTask(chokeTask, 60 * 1000);
        reactor->scheduleTask(uploadTask, 150);
        reactor->scheduleTask(saveStateTask, 120 * 1000);

        torrent.commandTask = commandTask;

        torrent.bundle->configuration().onFileMaskChanged.connect(
                commandTask, &CommandTask::notifyFileMaskChanged);

        torrent.bundle->configuration().onDownloadRateLimitChanged.connect(
                commandTask, &CommandTask::notifyRateLimitChanged);

        torrent.bundle->configuration().onUploadRateLimitChanged.connect(
                commandTask, &CommandTask::notifyRateLimitChanged);
    }

    if (!torrent.commandTask->allocateStorage()) {
        hSevere() << "Failed to allocate storage for torrent";
        return false;
    }

    if (!torrent.reactor->start()) {
        hSevere() << "Unable to start torrent's reactor";
        return false;
    }

    acceptorService_.acceptTorrentConnections(*torrent.bundle,
            Delegate::bind(&CommandTask::notifyPeerConnected, torrent.commandTask, _1));

    return true;
}

void GlobalTorrentRegistry::stopTorrent(TorrentBundle *bundle)
{
    std::lock_guard<std::mutex> l(anchor_);

    auto torrentIt = torrents_.find(bundle);

    if (torrentIt == torrents_.end() || !(*torrentIt).second.reactor->running())
        return;

    acceptorService_.rejectTorrentConnections(*bundle);
    (*torrentIt).second.reactor->stop();

    if (acceptorService_.torrentCount() == 0)
        acceptorService_.stop();
}

void GlobalTorrentRegistry::setListeningPort(int port)
{
    port_ = port;
}

void GlobalTorrentRegistry::setConnectionLimit(int limit)
{
    connectionLimit_ = limit;
}

void GlobalTorrentRegistry::limitDownloadRate(int limit)
{
    dloadRateLimit_ = limit;
}

void GlobalTorrentRegistry::limitUploadRate(int limit)
{
    uloadRateLimit_ = limit;
}

void GlobalTorrentRegistry::setCacheSizeLimit(int limit)
{
    cacheSizeLimit_ = limit;
}

const PeerId &GlobalTorrentRegistry::peerId() const
{
    return peerId_;
}

unsigned int GlobalTorrentRegistry::listeningPort() const
{
    return port_;
}

unsigned int GlobalTorrentRegistry::connectionLimit() const
{
    return connectionLimit_;
}

unsigned int GlobalTorrentRegistry::uploadRateLimit() const
{
    return dloadRateLimit_;
}

unsigned int GlobalTorrentRegistry::downloadRateLimit() const
{
    return uloadRateLimit_;
}

unsigned int GlobalTorrentRegistry::cacheSizeLimit() const
{
    return cacheSizeLimit_;
}

unsigned int GlobalTorrentRegistry::connectionCount() const
{
    unsigned int connections = 0;

    for (auto torrent = torrents_.begin(); torrent != torrents_.end(); ++torrent)
        connections += (*torrent).second.bundle->state().peerRegistry().peerCount();

    return connections;
}

std::vector<TorrentBundle *> GlobalTorrentRegistry::torrents() const
{
    std::vector<TorrentBundle *> torrents;

    for (auto it = torrents_.begin(); it != torrents_.end(); ++it)
        torrents.push_back((*it).second.bundle);

    return std::move(torrents);
}

GlobalTorrentRegistry *GlobalTorrentRegistry::self()
{
    static GlobalTorrentRegistry instance;
    return &instance;
}

void GlobalTorrentRegistry::populateTrackerRegistry(TorrentBundle &bundle)
{
    const AnnounceList &announceList = bundle.model().announceList();

    TierList tiers;

    if (!announceList.empty()) {
        unsigned trackerCount = 0;

        tiers.reserve(announceList.size());

        for (auto tier = announceList.begin(); tier != announceList.end(); ++tier) {
            tiers.resize(tiers.size() + 1);
            tiers.back().reserve((*tier).size());

            for (auto uri = (*tier).begin(); uri != (*tier).end(); ++uri) {
                Http::Uri uriTest(*uri);

                if (!uriTest.valid()) {
                    hDebug() << "Tracker URI is invalid; Tracker" << *uri << "will be ignored";
                    continue;
                } else if (uriTest.scheme() != "http") {
                    hDebug() << "URI scheme" << uriTest.scheme() << "is not supported;"
                             << "Tracker" << *uri << "will be ignored";
                    continue;
                }

                tiers.back().push_back(std::make_shared<Tracker>(bundle, *uri));
            }

            trackerCount += (*tier).size();
        }

        hcDebug(bundle.model().name()) << "Found" << trackerCount << "trackers in"
                                       << tiers.size() << "tiers";
    } else {
        std::shared_ptr<Tracker> tracker(new Tracker(bundle, bundle.model().announceUri()));

        tiers.resize(1);
        tiers[0].push_back(tracker);

        hcDebug(bundle.model().name()) << "Found 1 tracker; Announce list doesn't exist";
    }

    bundle.state().trackerRegistry().replaceTiers(tiers);
}
