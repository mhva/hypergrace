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
#include <ctime>
#include <limits>

#include "time.hh"

using namespace Hypergrace::Util;


Time::Time() :
    time_({0, 0})
{
}

Time::Time(int msec) :
    time_({ msec / 1000, (msec % 1000) * 1000000 })
{
    assert(msec >= 0);
}

Time::Time(int hh, int mm, int ss) :
    time_({ hh * 3600 + mm * 60 + ss, 0 })
{
    assert(hh >= 0 && mm >= 0 && ss >= 0);
}

Time::Time(const TimeValue &tv) :
    time_(tv)
{
}

void Time::addMsec(int msec)
{
    if (msec > 0) {
        *this += Time(msec);
    } else {
        *this -= Time(-msec);
    }
}

void Time::addSec(int sec)
{
    time_.sec += sec;

    if (time_.sec < 0)
        time_.sec = time_.nsec = 0;
}

int Time::hour() const
{
    return time_.sec / 3600;
}

int Time::minute() const
{
    return (time_.sec % 3600) / 60;
}

int Time::millisecond() const
{
    return time_.nsec / 1000000;
}

int Time::nanosecond() const
{
    return time_.nsec;
}

int Time::second() const
{
    return time_.sec % 60;
}

size_t Time::toHours() const
{
    return hour();
}

size_t Time::toMinutes() const
{
    return size_t(time_.sec) / 60;
}

size_t Time::toMilliseconds() const
{
    return size_t(time_.sec) * 1000 + time_.nsec / 1000000;
}

size_t Time::toSeconds() const
{
    return time_.sec;
}

Time Time::monotonicTime()
{
    timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != -1) {
        Time time;
        time.time_.sec = ts.tv_sec;
        time.time_.nsec = ts.tv_nsec;

        return time;
    } else {
        return Time();
    }
}

Time Time::maximumTime()
{
    Time max;

    max.time_.sec = std::numeric_limits<ssize_t>::max();
    max.time_.nsec = std::numeric_limits<ssize_t>::max();

    return max;
}

bool Time::equal(const Time &right) const
{
    return time_.sec == right.time_.sec && time_.nsec == right.time_.nsec;
}

bool Time::greater(const Time &right) const
{
    return time_.sec > right.time_.sec ||
        (time_.sec == right.time_.sec && time_.nsec > right.time_.nsec);
}

bool Time::greaterEqual(const Time &right) const
{
    return greater(right) || equal(right);
}

bool Time::less(const Time &right) const
{
    return time_.sec < right.time_.sec ||
        (time_.sec == right.time_.sec && time_.nsec < right.time_.nsec);
}

bool Time::lessEqual(const Time &right) const
{
    return less(right) || equal(right);
}

Time &Time::operator +=(const Time &right)
{
    size_t nsec = time_.nsec + right.time_.nsec;

    time_.sec = time_.sec + right.time_.sec + nsec / 1000000000;
    time_.nsec = nsec % 1000000000;

    return *this;
}

Time &Time::operator -=(const Time &right)
{
    time_.sec = time_.sec - right.time_.sec;
    time_.nsec = time_.nsec - right.time_.nsec;

    // Perform a carry if needed
    if (time_.nsec < 0) {
        --time_.sec;
        time_.nsec += 1000000000;
    }

    if (time_.sec < 0) {
        time_.sec = 0;
        time_.nsec = 0;
    }

    return *this;
}

const Time Time::operator +(const Time &right) const
{
    return Time(*this) += right;
}

const Time Time::operator -(const Time &right) const
{
    return Time(*this) -= right;
}

bool Time::operator <(const Time &right) const
{
    return less(right);
}

bool Time::operator <=(const Time &right) const
{
    return lessEqual(right);
}

bool Time::operator ==(const Time &right) const
{
    return equal(right);
}

bool Time::operator >=(const Time &right) const
{
    return greaterEqual(right);
}

bool Time::operator >(const Time &right) const
{
    return greater(right);
}
