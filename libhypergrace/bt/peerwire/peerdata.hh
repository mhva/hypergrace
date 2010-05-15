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

#ifndef BT_PEERWIRE_PEERDATA_CC_
#define BT_PEERWIRE_PEERDATA_CC_

#include <memory>

#include <bt/types.hh>

#include <util/rating.hh>
#include <util/bitfield.hh>

namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Net { class Socket; }}


namespace Hypergrace {
namespace Bt {

class PeerData
{
public:
    enum ReservedDataIds {
        DownloadTask = 0,
        InterestTask = 1,
        UploadTask = 2
    };

public:
    PeerData(const TorrentBundle &, Net::Socket &, const PeerId &);

    const PeerId &peerId() const;

    const Util::Bitfield &bitfield() const;
    const Util::Rating &rating() const;

    bool peerIsInterested() const;
    bool peerChokedUs() const;

    bool weAreInterested() const;
    bool weChokedPeer() const;

    Net::Socket &socket() const;

public:
    Util::Bitfield &bitfield();
    Util::Rating &rating();

    void setPeerIsInterested(bool);
    void setPeerChokedUs(bool);

    void setWeAreInterested(bool);
    void setWeChokedPeer(bool);

public:
    class CustomData { public: virtual ~CustomData() {} };

    template<typename Type> inline std::shared_ptr<Type> getData(size_t id) const
    {
        return std::static_pointer_cast<Type>(data_[id]);
    }

    template<typename Type> inline void setData(size_t id, std::shared_ptr<Type> data)
    {
        data_[id].swap(data);
    }

    template<typename Type> inline void setData(size_t id, Type *data)
    {
        data_[id].reset(data);
    }

private:
    Net::Socket &socket_;
    PeerId peerId_;

    Util::Bitfield bitfield_;
    Util::Rating rating_;

    bool dropBitfield_;

    bool peerChokedUs_;
    bool weChokedPeer_;

    bool peerIsInterested_;
    bool weAreInterested_;

    std::shared_ptr<CustomData> data_[5];
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_PEERDATA_CC_ */
