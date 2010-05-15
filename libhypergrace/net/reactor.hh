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

#ifndef NET_REACTOR_HH_
#define NET_REACTOR_HH_

#include <util/shared.hh>

namespace Hypergrace { namespace Net { class RateAccumulator; }}
namespace Hypergrace { namespace Net { class Socket; }}
namespace Hypergrace { namespace Net { class Task; }}


namespace Hypergrace {
namespace Net {

class Reactor
{
public:
    Reactor();
    ~Reactor();

public:
    bool observe(Socket *);

    void scheduleTask(Task *, int);

    void setDownloadRateAccumulator(RateAccumulator *);
    void setUploadRateAccumulator(RateAccumulator *);

public:
    bool start();
    void stop();

    bool running() const;

    Reactor(const Reactor &) = delete;
    void operator =(const Reactor &) = delete;

private:
    HG_DECLARE_PRIVATE;
};

} /* namespace Net */
} /* namespace Hypergrace */

#endif /* NET_REACTOR_HH_ */
