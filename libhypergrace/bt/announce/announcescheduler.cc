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

#include <cassert>

#include <bt/announce/tracker.hh>
#include <bt/announce/trackerresponse.hh>
#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/bundle/trackerregistry.hh>

#include <debug/debug.hh>

#include "announcescheduler.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


AnnounceScheduler::AnnounceScheduler(const TorrentBundle &bundle, Net::Reactor &reactor) :
    bundle_(bundle),
    reactor_(reactor)
{
}

void AnnounceScheduler::start()
{
    hcDebug(bundle.model().name()) << "Initiating first announce";

    execute();
}

void AnnounceScheduler::execute()
{
    Util::Time now = Util::Time::monotonicTime();
    std::shared_ptr<const TierList> tiers = bundle_.state().trackerRegistry().tiers();

    bool hasAnnouncingTrackers = false;

    for (auto tier = tiers->begin(); tier != tiers->end(); ++tier) {
        for (auto tracker = (*tier).begin(); tracker != (*tier).end(); ++tracker) {
            // Skip the tracker if announce is not finished yet
            if ((*tracker)->announceStatus() == Tracker::AnnounceInProcess) {
                // Check if announce has timed out
                if ((*tracker)->lastAnnounceStartTime() + Util::Time(0, 0, 30) <= now) {
                    (*tracker)->reset("Socket timed out");
                    (*tracker)->setSuggestedAnnounceTime(now + Util::Time(0, 1, 0));
                } else {
                    hasAnnouncingTrackers = true;
                }

                continue;
            }

            // If we initiated announce on the current tracker earlier
            // and if it's now completed successfully we need to send
            // retrieved peers down the pipe.
            auto pos = announcingTrackers_.find((*tracker)->uri());

            if (pos != announcingTrackers_.end() &&
                (*tracker)->announceStatus() == Tracker::AnnounceSucceeded)
            {
                onPeersAvailable((*tracker)->lastResponse()->peers());
                announcingTrackers_.erase((*tracker)->uri());
            }

            // Initiate announce on the tracker if deadline is reached
            if ((*tracker)->suggestedAnnounceTime() <= now) {
                (*tracker)->announce(reactor_);

                // Insert tracker's URI into the set to indicate that
                // we initiated announce on this tracker
                auto result = announcingTrackers_.insert((*tracker)->uri());

                hasAnnouncingTrackers = true;
            }
        }
    }

    // Some trackers where we initiated announce might have been
    // deleted and are now left hanging in the announcingTrackers_
    // set. If we didn't find any currently announcing trackers in
    // the actual tier list we can safely clear the set to conserve
    // memory.
    if (!hasAnnouncingTrackers)
        announcingTrackers_.clear();
}

void AnnounceScheduler::stop()
{
    std::shared_ptr<const TierList> tiers = bundle_.state().trackerRegistry().tiers();

    for (auto tier = tiers->begin(); tier != tiers->end(); ++tier) {
        for (auto tracker = (*tier).begin(); tracker != (*tier).end(); ++tracker) {
            // TODO: If we did have many unsuccessful announces in one
            // row on this tracker it doesn't make sense to send the
            // "stopped" event because we are probably have been dropped
            // from the tracker's peer list.
            if ((*tracker)->timesAnnounceSucceeded() > 0)
                (*tracker)->announceStopped(reactor_);
        }
    }
}
