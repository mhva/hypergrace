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

#ifndef BT_PEERWIRE_ACCEPTORSERVICE_HH_
#define BT_PEERWIRE_ACCEPTORSERVICE_HH_

#include <bt/types.hh>
#include <net/acceptorservice.hh>
#include <util/shared.hh>

namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Net { class HostAddress; }}


namespace Hypergrace {
namespace Bt {

class AcceptorService : public Net::AcceptorService
{
public:
    typedef Delegate::Delegate<void (const PeerSettings &)> ConnectionHandler;

public:
    AcceptorService(int);
    ~AcceptorService();

    void acceptTorrentConnections(const TorrentBundle &, ConnectionHandler);
    void rejectTorrentConnections(const TorrentBundle &);

    unsigned int torrentCount() const;

protected:
    bool willAccept();
    void demultiplex(int, const Net::HostAddress &);

private:
    HG_DECLARE_CUSTOM_SHARED_PRIVATE(NegotiatorMiddleware);
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_ACCEPTORSERVICE_HH_ */
