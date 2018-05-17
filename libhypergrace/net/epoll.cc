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

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <hg/debug/debug.hh>
#include <hg/util/error.hh>

#include "epoll.hh"

#define INITIAL_EPOLL_SIZE 128


namespace Hypergrace::Net {

EpollImpl::EpollImpl() :
    epfd_(epoll_create(INITIAL_EPOLL_SIZE)),
    events_(reinterpret_cast<epoll_event *>(
                malloc(sizeof(epoll_event) * INITIAL_EPOLL_SIZE))),
    fdCount_(0),
    fdMax_(INITIAL_EPOLL_SIZE)
{
}

EpollImpl::~EpollImpl()
{
    ::close(epfd_);
    free(events_);
}

void EpollImpl::add(Socket *socket, short int events)
{
    if (fdCount_ + 1 >= fdMax_) {
        events_ = reinterpret_cast<epoll_event *>(
            realloc(events_, (fdMax_ * 2) * sizeof(epoll_event)));
    }

    int fd = socket->fd();
    epoll_event evt = { events, { socket, fd } };

    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &evt) == -1) {
        hWarning() << "Failed to add socket (" << fd << ") into epoll"
                   << "(" << strerror(errno) << ")";
    }
}

void EpollImpl::remove(Socket *socket)
{
    int fd = socket->fd();

    if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
        hWarning() << "Failed to remove socket (" <<fd  << ") from epoll"
                   << "(" << strerror(errno) << ")";
    }
}

std::error_code EpollImpl::poll(int timeout)
{
    int active = epoll_wait(epfd_, events_, fdMax_, timeout);

    if (active >= 0) {
        for (int i = 0; i < active; ++i) {
            handle(reinterpret_cast<Socket *>(events_[i].data.ptr),
                   events_[i].events);
        }
    } else {
    }
}

} /* namespace Hypergrace::Net */
