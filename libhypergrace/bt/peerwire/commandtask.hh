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

#ifndef BT_PEERWIRE_BOOTSTRAPTASK_HH_
#define BT_PEERWIRE_BOOTSTRAPTASK_HH_

#include <memory>
#include <set>
#include <string>

#include <bt/types.hh>
#include <delegate/connectable.hh>
#include <net/task.hh>
#include <util/shared.hh>

namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Bt { class ChokeTask; }}
namespace Hypergrace { namespace Bt { class DownloadTask; }}
namespace Hypergrace { namespace Bt { class DiskIo; }}
namespace Hypergrace { namespace Bt { class UploadTask; }}
namespace Hypergrace { namespace Net { class BandwidthAllocator; }}
namespace Hypergrace { namespace Net { class Reactor; }}


namespace Hypergrace {
namespace Bt {

class CommandTask : public Net::Task, public Delegate::Connectable
{
public:
    CommandTask(TorrentBundle &, Net::Reactor &, std::shared_ptr<DiskIo>,
                Net::BandwidthAllocator &, Net::BandwidthAllocator &,
                ChokeTask &, DownloadTask &, UploadTask &);
    ~CommandTask();

    void notifyRateLimitChanged();
    void notifyFileMaskChanged();
    void notifyPeerConnected(const PeerSettings &);

private:
    void execute();

private:
    HG_DECLARE_PRIVATE
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_BOOTSTRAPTASK_HH_ */
