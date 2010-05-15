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

#include <pthread.h>

#include <debug/debug.hh>
#include <util/time.hh>

#include "event.hh"

using namespace Hypergrace::Thread;


class Event::Private
{
public:
    pthread_cond_t cond;
    pthread_condattr_t condattr;
    pthread_mutex_t mutex;
};

Event::Event() :
    d(new Private())
{
    pthread_mutex_init(&d->mutex, 0);
    pthread_condattr_init(&d->condattr);
    pthread_condattr_setclock(&d->condattr, CLOCK_MONOTONIC);
    pthread_cond_init(&d->cond, &d->condattr);
}

Event::~Event()
{
    while (pthread_cond_destroy(&d->cond) == EBUSY) {
        broadcastSignal();
        pthread_yield();
    }

    pthread_condattr_destroy(&d->condattr);
    pthread_mutex_destroy(&d->mutex);
    delete d;
}

void Event::broadcastSignal()
{
    pthread_cond_broadcast(&d->cond);
}

void Event::signal()
{
    pthread_cond_signal(&d->cond);
}

void Event::wait()
{
    pthread_mutex_lock(&d->mutex);
    pthread_cond_wait(&d->cond, &d->mutex);
    pthread_mutex_unlock(&d->mutex);
}

Event::WakeReason Event::wait(int milliseconds)
{
    WakeReason wakeReason;
    Util::Time deadline = Util::Time::monotonicTime() + Util::Time(milliseconds);
    timespec tsdeadline;

    tsdeadline.tv_sec = deadline.toSeconds();
    tsdeadline.tv_nsec = deadline.nanosecond();

    pthread_mutex_lock(&d->mutex);

    if (pthread_cond_timedwait(&d->cond, &d->mutex, &tsdeadline) == ETIMEDOUT)
        wakeReason = TimedOut;
    else
        wakeReason = WokenUp;

    pthread_mutex_unlock(&d->mutex);
    return wakeReason;
}
