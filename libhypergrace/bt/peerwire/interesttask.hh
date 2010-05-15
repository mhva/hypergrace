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

#ifndef BT_PEERWIRE_INTERESTTASK_HH_
#define BT_PEERWIRE_INTERESTTASK_HH_

#include <bt/peerwire/peerdata.hh>

namespace Hypergrace { namespace Bt { class TorrentBundle; }}


namespace Hypergrace {
namespace Bt {

class InterestTask
{
public:
    InterestTask(TorrentBundle &);

    void registerPeer(PeerData *);
    void unregisterPeer(PeerData *);

    void notifyHaveEvent(PeerData *, unsigned int);
    void notifyBitfieldEvent(PeerData *);

    void notifyDownloadedGoodPiece(unsigned int);

    void reconsiderInterest();

private:
    void updateWantedPieceCount(PeerData *);

private:
    struct InterestState : public PeerData::CustomData
    {
        unsigned int piecesWanted;
    };

    TorrentBundle &bundle_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_INTERESTTASK_HH_ */
