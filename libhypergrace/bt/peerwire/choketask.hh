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

#ifndef BT_IO_CHOKETASK_HH_
#define BT_IO_CHOKETASK_HH_

#include <net/task.hh>
#include <util/shared.hh>

namespace Hypergrace { namespace Net { class Socket; }}
namespace Hypergrace { namespace Bt { class PeerData; }}
namespace Hypergrace { namespace Bt { class TorrentBundle; }}


namespace Hypergrace {
namespace Bt {

class ChokeTask : public Net::Task
{
public:
    ChokeTask(TorrentBundle &);

    void registerPeer(PeerData *);
    void unregisterPeer(PeerData *);

    void notifyPeerBecameInterested(PeerData *);
    void notifyPeerBecameNotInterested(PeerData *);

private:
    void doFullChokeRound();
    void doPartialChokeRound();

    void execute();

private:
    const TorrentBundle &bundle_;

    unsigned int unchokedPeerCount_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_IO_CHOKETASK_HH_ */
