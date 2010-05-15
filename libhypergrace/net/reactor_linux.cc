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

#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <string.h>

#include <cassert>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

#include <debug/debug.hh>
#include <delegate/delegate.hh>

#include <net/inputmiddleware.hh>
#include <net/outputmiddleware.hh>
#include <net/rateaccumulator.hh>
#include <net/socket.hh>
#include <net/task.hh>

#include <util/time.hh>

#include "reactor.hh"

using namespace Hypergrace;
using namespace Net;


class Reactor::Private
{
    struct TaskDescriptor
    {
        Task *task;
        Util::Time deadline;
        Util::Time interval;
    };

public:
    Private() :
        thread_(0),
        downloadRate_(0),
        uploadRate_(0),
        bailout_(true)
    {
        epollfd_ = epoll_create1(0);

        if (epollfd_ == -1) {
            hSevere() << "Failed to open a epoll file descriptor";
            hSevere() << "Reactor has failed to initialize epoll; libhypergrace  employs "
                         "epoll to provide a basic foundation for network  infrastructure. "
                         "A failure to initialize epoll renders libhypergrace useless.";
        }
    }

    ~Private()
    {
        stop();

        ::close(epollfd_);

        std::for_each(tasks_.begin(), tasks_.end(), [](TaskDescriptor &d) { delete d.task; });
    }

    bool enqueueSocket(Socket *socket)
    {
        std::lock_guard<std::mutex> l(socketQueueLock_);
        auto pos = observedSockets_.find(socket->fd());

        if (pos != observedSockets_.end())
            return false;

        waitingSockets_.push_back(socket);

        return true;
    }

    void scheduleTask(Task *task, int interval)
    {
        TaskDescriptor descriptor;

        // We can skip calculating deadline because it will be adjusted
        // once scheduler is started.
        descriptor.task = task;
        descriptor.interval = Util::Time(interval);

        tasks_.push_back(descriptor);
    }

    bool running() const
    {
        return bailout_ != true && thread_ != 0;
    }

    bool start()
    {
        if (epollfd_ == -1) {
            hSevere() << "Unable to start a reactor because epoll has failed to initialize";
            return false;
        }

        if (!running()) {
            bailout_ = false;
            thread_ = new std::thread(Delegate::make(this, &Private::eventLoop));
            return true;
        } else {
            hDebug() << "Reactor cannot be started twice";
            return false;
        }
    }

    void stop()
    {
        if (!running())
            return;

        bailout_ = true;
        thread_->join();

        delete thread_;
        thread_ = 0;

        std::for_each(
                observedSockets_.begin(), observedSockets_.end(),
                [](std::map<int, Socket *>::value_type &v) { delete v.second; });

        observedSockets_.clear();
    }

private:
    void executeTasks(const Util::Time &now)
    {
        nearestTaskDeadline_ = Util::Time::maximumTime();

        for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
            TaskDescriptor &descriptor = *it;

            if ((descriptor.deadline - now).toMilliseconds() == 0) {
                descriptor.task->execute();

                // XXX: Update 'now'? Might be good idea if
                // there are some heavy-lifting tasks.
                descriptor.deadline = now + descriptor.interval;
            }

            if (descriptor.deadline < nearestTaskDeadline_)
                nearestTaskDeadline_ = descriptor.deadline;
        }
    }

    void eventLoop()
    {
        if (epollfd_ == -1) {
            hSevere() << "Cannot enter into event loop when epoll is not initialized";
            return;
        }

        Util::Time now = Util::Time::monotonicTime();

        nearestTaskDeadline_ = Util::Time::maximumTime();
        pulseDeadline_ = now + Util::Time(100);

        std::for_each(tasks_.begin(), tasks_.end(),
            [this, &now](TaskDescriptor &d) {
                // Rebase task deadline to avoid bursting execute()
                d.deadline = now + d.interval;
                d.task->start();

                if (d.deadline < this->nearestTaskDeadline_)
                    this->nearestTaskDeadline_ = d.deadline;
            }
        );

        while (!bailout_) {
            now = Util::Time::monotonicTime();

            if ((nearestTaskDeadline_ - now).toMilliseconds() == 0) {
                executeTasks(now);
            }

            if ((pulseDeadline_ - now).toMilliseconds() == 0) {
                pulseDeadline_ = now + Util::Time(100);
                pulse();
            }

            if (!waitingSockets_.empty()) {
                observeNewSockets();
            }

            unsigned int sleepTime =
                (std::min(pulseDeadline_, nearestTaskDeadline_) - now).toMilliseconds() * 1000;

            ::usleep(sleepTime);
        }

        std::for_each(tasks_.begin(), tasks_.end(), [](TaskDescriptor &d) { d.task->stop(); });
    }

    void pulse()
    {
        epoll_event events[1000];
        int eventCount;

        eventCount = epoll_wait(epollfd_, events, sizeof(events) / sizeof(epoll_event), 0);

        for (int i = 0; i < eventCount; ++i) {
            epoll_event &event = events[i];
            Net::Socket *socket = reinterpret_cast<Net::Socket *>(event.data.ptr);

            assert(socket != 0);

            //hDebug() << socket->fd()
            //         << ((event.events & EPOLLIN)    ? "EPOLLIN"    : "......."   )
            //         << ((event.events & EPOLLOUT)   ? "EPOLLOUT"   : "........"  )
            //         << ((event.events & EPOLLERR)   ? "EPOLLERR"   : "........"  )
            //         << ((event.events & EPOLLHUP)   ? "EPOLLHUP"   : "........"  )
            //         << ((event.events & EPOLLRDHUP) ? "EPOLLRDHUP" : "..........")
            //         << socket->remoteAddress();

            //if (event.events & EPOLLERR) {
            //    int error = 0;
            //    socklen_t len = sizeof(error);
            //    if (getsockopt(socket->fd(), SOL_SOCKET, SO_ERROR, &error, &len) != -1) {
            //        hDebug() << "An error has occurred on socket descriptor"
            //                    "(" << strerror(error) << ")";
            //    } else {
            //        hDebug() << "An error has occurred on socket descriptor";
            //    }
            //}

            bool hungUp = event.events & EPOLLRDHUP || event.events & EPOLLHUP;

            if (event.events & EPOLLOUT && !hungUp) {
                if (uploadRate_ != 0)
                    uploadRate_->accumulate(socket->write());
                else
                    socket->write();
            }

            if (event.events & EPOLLIN) {
                if (downloadRate_ != 0)
                    downloadRate_->accumulate(socket->read());
                else
                    socket->read();
            }

            if (hungUp || socket->closed()) {
                unobserve(socket);
                observedSockets_.erase(socket->fd());

                socket->shutdown();
                delete socket;
            }
        }
    }

    void observeNewSockets()
    {
        std::lock_guard<std::mutex> l(socketQueueLock_);

        for (auto conn = waitingSockets_.begin(); conn != waitingSockets_.end(); ++conn) {
            if (!observe(*conn))
                delete *conn;
        }

        waitingSockets_.clear();
    }

    bool observe(Socket *socket)
    {
        int fd = socket->fd();
        epoll_event descriptor = { 0 };

        descriptor.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
        descriptor.data.ptr = socket;

        auto hint = observedSockets_.lower_bound(fd);

        if (hint == observedSockets_.end() || (*hint).first != fd) {
            observedSockets_.insert(hint, std::make_pair(fd, socket));

            if (::epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &descriptor) == -1) {
                hWarning() << "Failed to add socket (" << fd << ") into epoll instance"
                           << "(" << strerror(errno) << ")";
                observedSockets_.erase(fd);
                return false;
            }

            return true;
        } else {
            hDebug() << "Socket with similar file descriptor is already being observed";
            return false;
        }
    }

    void unobserve(const Net::Socket *socket)
    {
        if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, socket->fd(), 0) == -1) {
            hDebug() << socket->fd() << "Failed to remove socket from epoll instance"
                     << "(" << strerror(errno) << ")";
        }
    }

public:
    std::thread *thread_;

    int epollfd_;
    std::map<int, Socket *> observedSockets_;
    std::deque<Net::Socket *> waitingSockets_;
    std::mutex socketQueueLock_;

    RateAccumulator *downloadRate_;
    RateAccumulator *uploadRate_;

    Util::Time nearestTaskDeadline_;
    Util::Time pulseDeadline_;

    // TODO: Find some decent implementation of min/max heap.
    // std::priority_queue is no go because it doesn't provide
    // modify() method.
    std::vector<TaskDescriptor> tasks_;

    bool bailout_;
};

Reactor::Reactor() :
    d(new Private())
{
}

Reactor::~Reactor()
{
    delete d;
}

bool Reactor::observe(Net::Socket *socket)
{
    if (d->running()) {
        return d->enqueueSocket(socket);
    } else {
        hWarning() << "Cannot observe a socket when reactor is in the stopped state";
        return false;
    }
}

void Reactor::scheduleTask(Task *task, int interval)
{
    if (!d->running())
        d->scheduleTask(task, interval);
    else
        hWarning() << "Cannot schedule task while reactor is running";
}

void Reactor::setDownloadRateAccumulator(RateAccumulator *accumulator)
{
    d->downloadRate_ = accumulator;
}

void Reactor::setUploadRateAccumulator(RateAccumulator *accumulator)
{
    d->uploadRate_ = accumulator;
}

bool Reactor::running() const
{
    return d->running();
}

bool Reactor::start()
{
    return d->start();
}

void Reactor::stop()
{
    return d->stop();
}
