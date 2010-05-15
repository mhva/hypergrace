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

#ifndef NET_ACCEPTORSERVICE_HH_
#define NET_ACCEPTORSERVICE_HH_

#include <cstdint>
#include <net/reactor.hh>

namespace Hypergrace { namespace Net { class HostAddress; }}

namespace Hypergrace {
namespace Net {

class AcceptorService
{
public:
    explicit AcceptorService(uint16_t);
    virtual ~AcceptorService();

    bool start();
    void stop();

    bool setPort(uint16_t);

    bool running() const;

    AcceptorService(const AcceptorService &) = delete;
    const AcceptorService &operator =(const AcceptorService &) = delete;

protected:
    Reactor &reactor();

    virtual bool willAccept() = 0;
    virtual void demultiplex(int, const HostAddress &) = 0;

private:
    Reactor reactor_;
    uint16_t port_;

    class AcceptorMiddleware;
    friend class AcceptorMiddleware;
};

} /* namespace Net */
} /* namespace Hypergrace */

#endif /* NET_ACCEPTORSERVICE_HH_ */
