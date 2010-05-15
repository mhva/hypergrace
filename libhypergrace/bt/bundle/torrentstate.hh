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

#ifndef BT_BUNDLE_TORRENTSTATE_HH_
#define BT_BUNDLE_TORRENTSTATE_HH_

#include <bt/bundle/peerregistry.hh>
#include <bt/bundle/trackerregistry.hh>

#include <delegate/signal.hh>

#include <util/bitfield.hh>


namespace Hypergrace {
namespace Bt {

class TorrentState
{
public:
    explicit TorrentState(unsigned int);
    ~TorrentState();

public:
    unsigned long long downloaded() const;
    unsigned long long uploaded() const;

    size_t downloadRate() const;
    size_t uploadRate() const;

    const Util::Bitfield &availablePieces() const;
    const Util::Bitfield &scheduledPieces() const;
    const Util::Bitfield &verifiedPieces() const;

    TrackerRegistry &trackerRegistry();
    PeerRegistry &peerRegistry();

    const TrackerRegistry &trackerRegistry() const;
    const PeerRegistry &peerRegistry() const;

public:
    void updateDownloaded(unsigned int);
    void updateUploaded(unsigned int);

    void setDownloadRate(size_t);
    void setUploadRate(size_t);

    void markPieceAsAvailable(unsigned int);
    void markPieceAsUnavailable(unsigned int);

    void markPieceAsInteresting(unsigned int);
    void markPieceAsUninteresting(unsigned int);

    void markPieceAsVerified(unsigned int);

    std::string toString() const;
    static TorrentState *fromString(const std::string &);

public:
    TorrentState(const TorrentState &) = delete;
    void operator =(const TorrentState &) = delete;

public:
    Util::Bitfield availablePieces_;
    Util::Bitfield scheduledPieces_;
    Util::Bitfield verifiedPieces_;

    volatile unsigned long long downloaded_;
    volatile unsigned long long uploaded_;

    volatile size_t downloadRate_;
    volatile size_t uploadRate_;

    PeerRegistry peerRegistry_;
    TrackerRegistry trackerRegistry_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_BUNDLE_TORRENTSTATE_HH_ */
