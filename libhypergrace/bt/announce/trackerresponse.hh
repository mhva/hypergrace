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

#ifndef BT_ANNOUNCE_TRACKERRESPONSE_HH_
#define BT_ANNOUNCE_TRACKERRESPONSE_HH_

#include <string>
#include <bt/types.hh>

namespace Hypergrace { namespace Bencode { class Object; }}


namespace Hypergrace {
namespace Bt {

class TrackerResponse
{
public:
    TrackerResponse(const std::string &);
    ~TrackerResponse();

    bool failed() const;

    std::string errorMessage() const;
    std::string warningMessage() const;
    std::string trackerId() const;

    PeerList peers() const;

    int seederCount() const;
    int leecherCount() const;
    int peerCount() const;
    int announceInterval() const;
    int minAnnounceInterval() const;

private:
    void cleanUp();

private:
    Bencode::Object *response_;
    bool peerListIsCompact_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_ANNOUNCE_TRACKERRESPONSE_HH_ */
