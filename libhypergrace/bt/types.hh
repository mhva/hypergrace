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

#ifndef BT_TYPES_HH_
#define BT_TYPES_HH_

#include <memory>
#include <vector>
#include <string>

#include <bt/announce/tracker.hh>
#include <net/hostaddress.hh>
#include <util/array.hh>


namespace Hypergrace { namespace Bt { class DataCollector; }}
namespace Hypergrace { namespace Bt { class PeerData; }}
namespace Hypergrace { namespace Net { class Socket; }}

namespace Hypergrace {
namespace Bt {

struct FileDescriptor {
    std::string filename;
    unsigned long long size;
    unsigned long long absOffset;
};

typedef Util::Array<unsigned char, 20> InfoHash;
typedef Util::Array<unsigned char, 20> PeerId;

typedef std::vector<std::vector<std::string> > AnnounceList;
typedef std::vector<Net::HostAddress> PeerList;

typedef std::vector<FileDescriptor> FileList;

typedef std::vector<std::shared_ptr<Tracker> > TrackerList;
typedef std::vector<TrackerList> TierList;

typedef std::vector<std::shared_ptr<PeerData> > SharedPeerList;
typedef std::vector<PeerData *> InternalPeerList;

struct PeerSettings {
    int socketFd;
    Net::HostAddress address;

    InfoHash infoHash;
    PeerId peerId;
    char supportedFeatures[8];

    std::string streamContinuation;

    // EncryptionInfo ...;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_TYPES_HH_ */
