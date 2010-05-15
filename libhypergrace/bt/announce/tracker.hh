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

#ifndef BT_TRACKER_HH_
#define BT_TRACKER_HH_

#include <memory>
#include <string>

#include <delegate/signal.hh>
#include <util/time.hh>

namespace Hypergrace { namespace Net { class Reactor; }}
namespace Hypergrace { namespace Bt { class TorrentBundle; }}
namespace Hypergrace { namespace Bt { class TrackerResponse; }}
namespace Hypergrace { namespace Http { class Response; }}


namespace Hypergrace {
namespace Bt {

class Tracker
{
public:
    enum AnnounceStatus {
        AnnounceFailed = 0,
        AnnounceSucceeded = 1,
        AnnounceInProcess = 2
    };

public:
    Tracker(const TorrentBundle &, const std::string &);

    void announce(Net::Reactor &);
    void announceStopped(Net::Reactor &);
    void announceCompleted(Net::Reactor &);

    void reset(const std::string &);

    void setSuggestedAnnounceTime(const Util::Time &);

public:
    const std::string &uri() const;

    AnnounceStatus announceStatus() const;
    std::shared_ptr<const TrackerResponse> lastResponse() const;

    unsigned int timesAnnounceSucceeded() const;
    unsigned int timesAnnounceFailed() const;

    Util::Time lastAnnounceStartTime() const;
    Util::Time suggestedAnnounceTime() const;

public:
    Delegate::Signal<> onAnnounceSucceeded;
    Delegate::Signal<> onAnnounceFailed;

private:
    void announce(Net::Reactor &, const char *);

    void processTrackerResponse(const Http::Response &);
    void processGoodHttpResponse(const Http::Response &);

    void handleInvalidHttpResponse();
    void handleConnectError();

    void succeed();
    void fail(const std::string &);

    void increaseSuccessCounter();
    void increaseFailureCounter();

private:
    const TorrentBundle &bundle_;

    std::string uri_;

    std::string trackerId_;
    std::string error_;

    std::shared_ptr<TrackerResponse> lastResponse_;

    AnnounceStatus announceStatus_;
    Util::Time suggestedAnnounceTime_;
    Util::Time announceStartTime_;

    unsigned int timesSucceeded_;
    unsigned int timesFailed_;

    bool freezeSuccessFailureCounters_;

    unsigned long long downloadedDelta_;
    unsigned long long uploadedDelta_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_TRACKER_HH_ */
