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

#ifndef NET_EPOLL_HH_
#define NET_EPOLL_HH_

#include <sys/epoll.h>

#include <system_error>

namespace Hypergrace {
namespace Net {

class EpollImpl {
public:
    enum : short int {
        In  = EPOLLIN,
        Out = EPOLLOUT,
        Hup = EPOLLHUP | EPOLLRDHUP,
        Err = EPOLLERR,
        Pri = EPOLLPRI
    };

    Epoll();
    ~Epoll();

    void add(Socket *, short int events);
    void remove(Socket *);

    std::error_code poll(int timeout);

protected:
    virtual void handle(Socket *, short int events) = 0;

private:
    int epfd_;
    epoll_event *events_;
    size_t fdCount_;
    size_t fdMax_;
};

} /* namespace Net */
} /* namespace Hypergrace */

#endif /* NET_EPOLL_HH_ */
