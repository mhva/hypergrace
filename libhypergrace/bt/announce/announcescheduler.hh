/*
   Copyright (C) 2009 Anton Mihalyov <anton@glyphsense.com>

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

#ifndef BT_ANNOUNCE_ANNOUNCETASK_HH_
#define BT_ANNOUNCE_ANNOUNCETASK_HH_

#include <set>
#include <string>

#include <bt/types.hh>
#include <delegate/delegate.hh>
#include <net/hostaddress.hh>
#include <net/task.hh>
#include <util/shared.hh>

namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Net { class Reactor; }}


namespace Hypergrace {
namespace Bt {

class AnnounceScheduler : public Net::Task
{
public:
    AnnounceScheduler(const TorrentBundle &, Net::Reactor &);

    Delegate::Delegate<void (const PeerList &)> onPeersAvailable;

private:
    void start();
    void execute();
    void stop();

private:
    const TorrentBundle &bundle_;
    Net::Reactor &reactor_;

    std::set<std::string> announcingTrackers_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_ANNOUNCE_ANNOUNCETASK_HH_ */
