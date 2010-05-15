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

#ifndef BT_PEERWIRE_EVENTHUB_HH_
#define BT_PEERWIRE_EVENTHUB_HH_

#include <map>

#include <bt/peerwire/message.hh>
#include <bt/peerwire/inputmiddleware.hh>

namespace Hypergrace { namespace Bt { class ChokeTask; }}
namespace Hypergrace { namespace Bt { class DownloadTask; }}
namespace Hypergrace { namespace Bt { class InterestTask; }}
namespace Hypergrace { namespace Bt { class PeerData; }}
namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Bt { class UploadTask; }}

namespace Hypergrace {
namespace Bt {

class EventHub : public Bt::InputMiddleware
{
public:
    EventHub(TorrentBundle &, std::shared_ptr<PeerData>, ChokeTask &, InterestTask &,
             DownloadTask &, UploadTask &);
    ~EventHub();

public:
    void processMessage(Net::Socket &, ChokeMessage &);
    void processMessage(Net::Socket &, UnchokeMessage &);
    void processMessage(Net::Socket &, InterestedMessage &);
    void processMessage(Net::Socket &, NotInterestedMessage &);
    void processMessage(Net::Socket &, HaveMessage &);
    void processMessage(Net::Socket &, BitfieldMessage &);
    void processMessage(Net::Socket &, PieceMessage &);
    void processMessage(Net::Socket &, RequestMessage &);
    void processMessage(Net::Socket &, CancelMessage &);

private:
    TorrentBundle &bundle_;
    std::shared_ptr<PeerData> peer_;

    ChokeTask &chokeTask_;
    InterestTask &interestTask_;
    DownloadTask &downloadTask_;
    UploadTask &uploadTask_;

    unsigned int wantedPiecesFromPeer_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_EVENTHUB_HH_ */
