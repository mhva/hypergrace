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

#ifndef BT_GLOBALTORRENTREGISTRY_HH_
#define BT_GLOBALTORRENTREGISTRY_HH_

#include <map>
#include <memory>
#include <mutex>

#include <bt/types.hh>
#include <bt/peerwire/acceptorservice.hh>
#include <net/bandwidthallocator.hh>

namespace Hypergrace { namespace Bt { class CommandTask; }}
namespace Hypergrace { namespace Bt { class DiskIo; }}
namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Net { class Reactor; }}


namespace Hypergrace {
namespace Bt {

class GlobalTorrentRegistry
{
public:
    GlobalTorrentRegistry();
    ~GlobalTorrentRegistry();

    bool createTorrent(TorrentBundle *);
    void deleteTorrent(TorrentBundle *);

    bool startTorrent(TorrentBundle *);
    void stopTorrent(TorrentBundle *);

    void setListeningPort(int);
    void setConnectionLimit(int);
    void limitDownloadRate(int);
    void limitUploadRate(int);
    void setCacheSizeLimit(int);

    const PeerId &peerId() const;

    unsigned int listeningPort() const;
    unsigned int connectionLimit() const;
    unsigned int uploadRateLimit() const;
    unsigned int downloadRateLimit() const;
    unsigned int cacheSizeLimit() const;

    unsigned int connectionCount() const;
    std::vector<TorrentBundle *> torrents() const;

    static GlobalTorrentRegistry *self();

private:
    bool allocateFiles();
    void populateTrackerRegistry(TorrentBundle &);

private:
    struct Torrent {
        TorrentBundle *bundle;
        Net::Reactor *reactor;
        CommandTask *commandTask;
    };

    std::map<TorrentBundle *, Torrent> torrents_;

    std::shared_ptr<DiskIo> defaultIoThread_;
    Bt::AcceptorService acceptorService_;
    Net::BandwidthAllocator downloadAllocator_;
    Net::BandwidthAllocator uploadAllocator_;

    int port_;
    unsigned int connectionLimit_;
    unsigned int dloadRateLimit_;
    unsigned int uloadRateLimit_;
    unsigned int cacheSizeLimit_;
    PeerId peerId_;

    std::mutex anchor_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_GLOBALTORRENTREGISTRY_HH_ */
