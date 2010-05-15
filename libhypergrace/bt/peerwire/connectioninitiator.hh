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

#ifndef BT_PEERWIRE_CONNECTIONINITIATOR_HH_
#define BT_PEERWIRE_CONNECTIONINITIATOR_HH_

#include <map>
#include <memory>

#include <bt/types.hh>
#include <delegate/delegate.hh>
#include <net/task.hh>

namespace Hypergrace { namespace Net { class Reactor; }}
namespace Hypergrace { namespace Bt { class TorrentBundle; }}


namespace Hypergrace {
namespace Bt {

class ConnectionInitiator : public Net::Task
{
public:
    ConnectionInitiator(const TorrentBundle &, Net::Reactor &);

    void enqueuePeers(const PeerList &);

public:
    Delegate::Delegate<void (const PeerSettings &)> onConnectedToPeer;

private:
    void connectToPeer(const Net::HostAddress &);

    void handleConnectSuccess(Net::HostAddress, PeerSettings);
    void handleConnectFailure(Net::HostAddress);

    unsigned int availableSlots() const;

    void execute();

private:
    struct PeerDescriptor {
        Net::HostAddress address;

        bool connecting;

        Util::Time retryTime;
        unsigned int retryCount;
    };

    const TorrentBundle &bundle_;
    Net::Reactor &reactor_;

    std::map<Net::HostAddress, PeerDescriptor> knownPeers_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_CONNECTIONINITIATOR_HH_ */
