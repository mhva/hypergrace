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

#ifndef BT_IO_DOWNLOADTASK_HH_
#define BT_IO_DOWNLOADTASK_HH_

#include <delegate/signal.hh>
#include <bt/types.hh>
#include <net/task.hh>
#include <util/shared.hh>

namespace Hypergrace { namespace Bt { class BlockCache; }}
namespace Hypergrace { namespace Bt { class DataCollector; }}
namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Net { class Socket; }}


namespace Hypergrace {
namespace Bt {

class DownloadTask : public Net::Task
{
public:
    DownloadTask(TorrentBundle &);
    ~DownloadTask();

    void registerPeer(PeerData *);
    void unregisterPeer(PeerData *);

    bool notifyDownloadedBlock(PeerData *, unsigned int, unsigned int, const std::string &);
    void notifyDownloadedGoodPiece(unsigned int);
    void notifyDownloadedBadPiece(unsigned int);

    void notifyChokeEvent(PeerData *);
    void notifyUnchokeEvent(PeerData *);

    void notifyHaveEvent(PeerData *, unsigned int);
    void notifyBitfieldEvent(PeerData *);

    BlockCache &cache();

private:
    void execute();

private:
    HG_DECLARE_PRIVATE
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_IO_DOWNLOADTASK_HH_ */
