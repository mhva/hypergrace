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

#ifndef BT_PEERWIRE_PEERDATACOLLECTOR_HH_
#define BT_PEERWIRE_PEERDATACOLLECTOR_HH_

#include <bt/peerwire/inputmiddleware.hh>
#include <bt/peerwire/message.hh>
#include <net/outputmiddleware.hh>

namespace Hypergrace { namespace Bt { class PeerData; }}
namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Net { class Socket; }}


namespace Hypergrace {
namespace Bt {

class PeerDataCollector : public Bt::InputMiddleware, public Net::OutputMiddleware
{
public:
    PeerDataCollector(const TorrentBundle &, std::shared_ptr<PeerData>,
                      Bt::InputMiddleware::Pointer);
    PeerDataCollector(const TorrentBundle &, std::shared_ptr<PeerData>,
                      Bt::InputMiddleware::Pointer, Net::OutputMiddleware::Pointer);
    ~PeerDataCollector();

public:
    void processMessage(Net::Socket &, ChokeMessage &);
    void processMessage(Net::Socket &, UnchokeMessage &);
    void processMessage(Net::Socket &, InterestedMessage &);
    void processMessage(Net::Socket &, NotInterestedMessage &);
    void processMessage(Net::Socket &, HaveMessage &);
    void processMessage(Net::Socket &, BitfieldMessage &);
    void processMessage(Net::Socket &, PieceMessage &);
    void processMessage(Net::Socket &, RequestMessage &);

    void send(Net::Socket &, Net::Packet *);

private:
    const TorrentBundle &bundle_;
    std::shared_ptr<PeerData> peerData_;

    bool dropBitfield_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_PEERDATACOLLECTOR_HH_ */
