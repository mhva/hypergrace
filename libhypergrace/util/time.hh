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

#ifndef UTIL_TIME_HH_
#define UTIL_TIME_HH_

#include <cstdlib>
#include <cstdint>
#include <debug/debug.hh>

namespace Hypergrace {
namespace Util {

class Time
{
public:
    Time();
    Time(int);
    Time(int, int, int);

    void addMsec(int);
    void addSec(int);

    int hour() const;
    int minute() const;
    int millisecond() const;
    int nanosecond() const;
    int second() const;

    size_t toHours() const;
    size_t toMinutes() const;
    size_t toMilliseconds() const;
    size_t toSeconds() const;

    bool equal(const Time &) const;
    bool greater(const Time &) const;
    bool greaterEqual(const Time &) const;
    bool less(const Time &) const;
    bool lessEqual(const Time &) const;

    static Time monotonicTime();
    static Time maximumTime();

public:
     Time &operator +=(const Time &);
     Time &operator -=(const Time &);

     const Time operator +(const Time &) const;
     const Time operator -(const Time &) const;

     bool operator <(const Time &) const;
     bool operator <=(const Time &) const;
     bool operator ==(const Time &) const;
     bool operator >=(const Time &) const;
     bool operator >(const Time &) const;

private:
    struct TimeValue;

    Time(const TimeValue &);

private:
    struct TimeValue
    {
        ssize_t sec;
        ssize_t nsec;
    } time_;
};

} /* namespace Util */
} /* namespace Hypergrace */

#endif /* UTIL_TIME_HH_ */
