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

#ifndef THREAD_EVENT_HH_
#define THREAD_EVENT_HH_

#include <util/shared.hh>

namespace Hypergrace {
namespace Thread {

class Event
{
public:
    enum WakeReason {
        WokenUp = 0,
        TimedOut = 1
    };

public:
    Event();
    ~Event();

    void broadcastSignal();
    void signal();

    void wait();

    /**
     * Blocks the calling thread until either another thread invokes
     * the signal() method or the broadcastSignal() method or
     * specified amount of time has elapsed.
     *
     * Returns WokenUp if the thread has been woken up by another
     * thread or a spurious wake up has caused it to resume. TimeOut
     * will be returned if the timeout has expired.
     */
    WakeReason wait(int);

private:
    HG_DECLARE_PRIVATE;
};

} /* namespace Thread */
} /* namespace Hypergrace */

#endif /* THREAD_EVENT_HH_ */
