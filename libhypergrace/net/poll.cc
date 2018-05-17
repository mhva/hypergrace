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

#include <util/error.hh>
#include "poll.hh"

#define INITIAL_FDS 64

#ifdef WINDOWS
#   define POLL WSAPollImpl
#else
#   define POLL poll
#endif

namespace Hypergrace::Net {

PollImpl::PollImpl() :
    pfds_(malloc(sizeof(pfds_[0]) * INITIAL_FDS)),
    sockets_(malloc(sizeof(Socket *) * INITIAL_FDS)),
    fdCount_(0),
    fdMax_(INITIAL_FDS)
{
}

PollImpl::~PollImpl()
{
    free(sockets_);
    free(pfds_);
}

void PollImpl::add(Socket *socket, short int events)
{
    // Resize fd array if neccessary.
    if (fdCount_ + 1 >= fdMax_) {
        pfds_ = realloc(pfds_, (fdMax_ * 2) * sizeof(pfds_[0]));
        sockets_ = realloc(sockets_, (fdMax_ * 2) * sizeof(Socket *));
    }

    pfds_[fdCount].fd = socket->fd();
    pfds_[fdCount].events = events;
    sockets_[fdCount_++] = socket;
}

void PollImpl::remove(Socket *socket)
{
    int fd = socket->fd();
    int index = 0;

    // Find index of the corresponding fd in the array of pollfd descriptors.
    while (index < fdCount_ && pfds_[index].fd != fd)
        ++index;

    if (index == fdCount_)
        return;

    // Since we don't care about element ordering in the pfds_ array we can
    // perform a cheap removal by copying last element in the array over an
    // element that's being removed.
    if (index < fdCount_ - 1) {
        memcpy(pfds_[index], pfds_[fdCount_ - 1], sizeof(POLLFD));
        sockets_[index] = sockets_[fdCount_ - 1];
    }

    --fdCount_;
}

int PollImpl::poll(int timeout)
{
    int active = POLL(PFD(pfds_), fdCount_, timeout);

    if (active >= 0) {
        for (size_t i = 0; i < fdCount_ && active > 0; ++i) {
            if (PFD(pfds_)[i].revents != 0) {
                handle(sockets_[i], pfds_[i].revents);
                --active;
            }
        }

        return std::error_code();
    } else {
        return -1;
    }
}

}
