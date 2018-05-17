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

#ifndef NET_POLL_HH_
#define NET_POLL_HH_

#if defined(HG_WINDOWS)
#   include <Windows.h>
#   define HG_POLLFD WSAPOLLFD
#else
#   include <poll.h>
#   define HG_POLLFD pollfd
#endif

#include <hg/net/poll.hh>
#include <hg/config.hh>

namespace Hypergrace {
namespace Net {

class PollImpl {
public:
    enum class Event : short int {
        In  = POLLIN,
        Out = POLLOUT,
        Hup = POLLHUP,
        Err = POLLERR,
        Pri = POLLPRI
    };

    PollImpl();
    ~PollImpl();

    void add(Socket *, short int events);
    void remove(Socket *);

    int poll(int timeout);

protected:
    virtual void handle(Socket *, short int events) = 0;

private:
    HG_POLLFD *pfds_;
    Socket *sockets_;
    size_t fdCount_;
    size_t fdMax_;
};

} /* namespace Net */
} /* namespace Hypergrace */

#undef HG_POLLFD

#endif /* NET_POLL_HH_ */
