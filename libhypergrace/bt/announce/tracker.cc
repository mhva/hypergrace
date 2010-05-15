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

#include <sstream>
#include <stdexcept>

#include <debug/debug.hh>

#include <bt/announce/trackerresponse.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/globaltorrentregistry.hh>

#include <http/inputmiddleware.hh>
#include <http/uri.hh>
#include <http/request.hh>
#include <http/response.hh>
#include <http/responseassembler.hh>

#include <net/bootstrapconnection.hh>
#include <net/hostaddress.hh>
#include <net/reactor.hh>

#include "trackerresponse.hh"
#include "tracker.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


class ResponseBridge : public Http::InputMiddleware
{
public:
    void processResponse(Net::Socket &socket, Http::Response &httpResponse)
    {
        onResponse(httpResponse);
    }

    void handleError(Net::Socket &socket)
    {
        onError();
    }

    Delegate::Delegate<void (const Http::Response &)> onResponse;
    Delegate::Delegate<void ()> onError;
};

Tracker::Tracker(const TorrentBundle &bundle, const std::string &uri) :
    bundle_(bundle),
    uri_(uri),
    announceStatus_(AnnounceFailed),
    suggestedAnnounceTime_(Util::Time::monotonicTime()),
    announceStartTime_(0),
    timesSucceeded_(0),
    timesFailed_(0),
    freezeSuccessFailureCounters_(false),
    downloadedDelta_(0),
    uploadedDelta_(0)
{
    if (!Http::Uri(uri).valid())
        throw std::runtime_error("Invalid tracker URI");
}

void Tracker::announce(Net::Reactor &reactor)
{
    if (timesSucceeded_ > 0) {
        announce(reactor, "");
    } else {
        freezeSuccessFailureCounters_ = false;

        downloadedDelta_ = bundle_.state().downloaded();
        uploadedDelta_ = bundle_.state().uploaded();

        announce(reactor, "started");
    }
}

void Tracker::announceStopped(Net::Reactor &reactor)
{
    // Freeze timesSucceeded_ and timesFailed_ counters. We need them
    // to stay zeroed because the Tracker::announce() method depends
    // on them to detect the started event.
    freezeSuccessFailureCounters_ = true;

    timesSucceeded_ = 0;
    timesFailed_ = 0;

    announce(reactor, "stopped");
}

void Tracker::announceCompleted(Net::Reactor &reactor)
{
    announce(reactor, "completed");
}

void Tracker::reset(const std::string &reason)
{
    Util::Time now = Util::Time::monotonicTime();
    announceStatus_ = AnnounceFailed;

    // Suggest new deadline if we reached the current one
    if (suggestedAnnounceTime_ <= now) {
        if (lastResponse_ && !lastResponse_->failed())
            suggestedAnnounceTime_ = now + lastResponse_->announceInterval();
        else
            suggestedAnnounceTime_ = now + Util::Time(0, 20, 0);
    }

    fail(reason);
}

void Tracker::setSuggestedAnnounceTime(const Util::Time &time)
{
    suggestedAnnounceTime_ = time;
}

const std::string &Tracker::uri() const
{
    return uri_;
}

Tracker::AnnounceStatus Tracker::announceStatus() const
{
    return announceStatus_;
}

std::shared_ptr<const TrackerResponse> Tracker::lastResponse() const
{
    return lastResponse_;
}

unsigned int Tracker::timesAnnounceSucceeded() const
{
    return timesSucceeded_;
}

unsigned int Tracker::timesAnnounceFailed() const
{
    return timesFailed_;
}

Util::Time Tracker::lastAnnounceStartTime() const
{
    return announceStartTime_;
}

Util::Time Tracker::suggestedAnnounceTime() const
{
    return suggestedAnnounceTime_;
}

void Tracker::announce(Net::Reactor &reactor, const char *eventName)
{
    announceStatus_ = AnnounceInProcess;
    announceStartTime_ = Util::Time::monotonicTime();

    Http::Uri uri(uri_);

    const TorrentState &state = bundle_.state();
    const TorrentModel &model = bundle_.model();

    uri.setParameter("info_hash", model.hash());
    uri.setParameter("peer_id", GlobalTorrentRegistry::self()->peerId());
    uri.setParameter("port", GlobalTorrentRegistry::self()->listeningPort());
    uri.setParameter("left", model.torrentSize() - state.downloaded());
    uri.setParameter("compact", 1);
    uri.setParameter("no_peer_id", 1);

    uri.setParameter("downloaded", state.downloaded() - downloadedDelta_);
    uri.setParameter("uploaded", state.uploaded() - uploadedDelta_);

    uri.setParameter("event", eventName);

    if (!trackerId_.empty())
        uri.setParameter("trackerid", trackerId_);

    ResponseBridge *bridge = new ResponseBridge();
    Http::Request *request = new Http::Request(uri);

    bridge->onResponse = Delegate::bind(&Tracker::processTrackerResponse, this, _1);
    bridge->onError = Delegate::make(this, &Tracker::handleInvalidHttpResponse);

    Net::BootstrapConnection()
        .withReactor(&reactor)
        .withEndpoint(uri.host(), !uri.port().empty() ? uri.port() : "80")
        .withMiddleware(new Http::ResponseAssembler(bridge))
        .withErrorHandler(Delegate::make(this, &Tracker::handleConnectError))
        .withQueuedPacket(request)
        .initiate();
}

void Tracker::processTrackerResponse(const Http::Response &httpResponse)
{
    Util::Time now = Util::Time::monotonicTime();
    std::ostringstream errorStream;

    if (httpResponse.statusCode() >= 200 && httpResponse.statusCode() <= 299) {
        processGoodHttpResponse(httpResponse);
    } else if (httpResponse.statusCode() >= 300 && httpResponse.statusCode() <= 399) {
        // TODO: Handle http redirect
        suggestedAnnounceTime_ = now + Util::Time(24000, 0, 0);

        errorStream << httpResponse.statusCode() << " - HTTP Redirect";
        fail(errorStream.str());
    } else if (httpResponse.statusCode() >= 400 && httpResponse.statusCode() <= 499) {
        switch (httpResponse.statusCode()) {
        case 403: // Forbidden
        case 404: // Not Found
        case 408: // Request Timeout
            suggestedAnnounceTime_ = now + Util::Time(0, 20, 0);
            break;
        default:
            suggestedAnnounceTime_ = now + Util::Time(1, 0, 0);
            break;
        }

        errorStream << httpResponse.statusCode() << " - HTTP Client Error";
        fail(errorStream.str());
    } else if (httpResponse.statusCode() >= 500 && httpResponse.statusCode() <= 599) {
        switch (httpResponse.statusCode()) {
        case 500: // Internal Server Error
        case 502: // Bad Gateway
        case 503: // Service Unavailable
        case 504: // Gateway Timeout
            suggestedAnnounceTime_ = now + Util::Time(0, 2, 0);
            break;
        case 501: // Not Implemented
        case 505: // HTTP Version Not Supported
            suggestedAnnounceTime_ = now + Util::Time(2, 0, 0);
            break;
        default:
            suggestedAnnounceTime_ = now + Util::Time(0, 20, 0);
            break;
        }

        errorStream << httpResponse.statusCode() << " - HTTP Server Error";
        fail(errorStream.str());
    } else {
        suggestedAnnounceTime_ = now + Util::Time(1, 0, 0);

        errorStream << httpResponse.statusCode() << " - HTTP Unknown Status Code";
        fail(errorStream.str());
    }
}

void Tracker::processGoodHttpResponse(const Http::Response &httpResponse)
{
    TrackerResponse *trackerResponse;
    Util::Time now = Util::Time::monotonicTime();

    // Try to create the TrackerResponse object. Exception might be
    // raised in the case when response contains malformed bencode
    // data
    try {
        trackerResponse = new TrackerResponse(httpResponse.messageBody());
    } catch (std::exception &e) {
        hcDebug(bundle_.model().name()) << "An error occurred while parsing tracker (" << uri_
                                        << ") response:" << e.what();

        suggestedAnnounceTime_ = now + Util::Time(0, 20, 0);
        fail("Got malformed bencode data");

        return;
    }

    if (!trackerResponse->failed()) {
        trackerId_ = trackerResponse->trackerId();

        hcDebug(bundle_.model().name()) << "Got" << trackerResponse->peerCount() << "peers from"
                                        << uri_;

        // If tracker has too short re-announce interval default
        // to more suitable value
        if (trackerResponse->announceInterval() >= 120)
            suggestedAnnounceTime_ = now + Util::Time(0, 0, trackerResponse->announceInterval());
        else
            suggestedAnnounceTime_ = now + Util::Time(0, 20, 0);

        succeed();
    } else {
        hcDebug(bundle_.model().name()) << "Tracker (" << uri_ << ") returned an error:"
                                        << trackerResponse->errorMessage();

        suggestedAnnounceTime_ = now + Util::Time(0, 20, 0);
        fail(trackerResponse->errorMessage());
    }

    lastResponse_.reset(trackerResponse);
}

void Tracker::handleInvalidHttpResponse()
{
    hcDebug(bundle_.model().name()) << "Got a malformed HTTP response from tracker" << uri_;

    suggestedAnnounceTime_ = Util::Time::monotonicTime() + Util::Time(0, 5, 0);

    // TODO: If we continuously get invalid responses from the tracker
    // suggest scheduler to exclude it from retry queue for session
    fail("Got a malformed HTTP response from tracker");
}

void Tracker::handleConnectError()
{
    hcDebug(bundle_.model().name()) << "An error occurred while connecting to tracker" << uri_;

    suggestedAnnounceTime_ = Util::Time::monotonicTime() + Util::Time(0, 20, 0);
    fail("Connection error");
}

void Tracker::fail(const std::string &error)
{
    error_ = error;
    announceStatus_ = AnnounceFailed;

    if (!freezeSuccessFailureCounters_)
        ++timesFailed_;

    onAnnounceFailed();
}

void Tracker::succeed()
{
    error_.clear();
    announceStatus_ = AnnounceSucceeded;

    if (!freezeSuccessFailureCounters_)
        ++timesSucceeded_;

    onAnnounceSucceeded();
}
