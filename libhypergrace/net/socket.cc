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

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cassert>

#include <debug/debug.hh>

#include <net/bandwidthallocator.hh>
#include <net/packet.hh>

#include <util/backtrace.hh>

#include "socket.hh"

using namespace Hypergrace;
using namespace Net;


Socket::Socket(int socket, const HostAddress &host) :
    socket_(socket),
    remoteAddress_(host),
    localDownloadAllocator_(0),
    localUploadAllocator_(0),
    globalDownloadAllocator_(0),
    globalUploadAllocator_(0),
    closed_(false),
    pendingData_(),
    pendingOffset_(0),
    data_(0)
{
    assert(socket_ != -1);
}

Socket::~Socket()
{
    if (::close(socket_) == -1) {
        hDebug() << "Failed to close socket file descriptor associated with remote address"
                 << remoteAddress_ << "(" << strerror(errno) << ")";
    }

    std::for_each(packetQueue_.begin(), packetQueue_.end(), [](Packet *p) { delete p; });
}

void Socket::send(Packet *packet)
{
    if (output_)
        output_->send(*this, packet);

    packetQueue_.push_back(packet);
}

void Socket::setLocalBandwidthAllocators(BandwidthAllocator *dl, BandwidthAllocator *ul)
{
    localDownloadAllocator_ = dl;
    localUploadAllocator_ = ul;
}

void Socket::setGlobalBandwidthAllocators(BandwidthAllocator *dl, BandwidthAllocator *ul)
{
    globalDownloadAllocator_ = dl;
    globalUploadAllocator_ = ul;
}

void Socket::setInputMiddleware(Net::InputMiddleware::Pointer input)
{
    input_ = input;
}

void Socket::setOutputMiddleware(Net::OutputMiddleware::Pointer output)
{
    output_ = output;
}

void Socket::close()
{
    closed_ = true;

    std::for_each(packetQueue_.begin(), packetQueue_.end(), [](Packet *p) { delete p; });

    packetQueue_.clear();
}

int Socket::cloneFd()
{
    return fcntl(socket_, F_DUPFD, 0);
}

int Socket::fd() const
{
    return socket_;
}

Net::InputMiddleware &Socket::inputMiddleware()
{
    return *input_;
}

Net::OutputMiddleware &Socket::outputMiddleware()
{
    return *output_;
}

const HostAddress &Socket::remoteAddress() const
{
    return remoteAddress_;
}

bool Socket::closed() const
{
    return closed_;
}

int Socket::allocateBandwidth(BandwidthAllocator *local, BandwidthAllocator *global, int size)
{
    // First try to allocate from the local bandwidth allocator.
    int allocatedNow = 0;

    if (local != 0) {
        allocatedNow = local->allocate(size);

        assert(allocatedNow >= 0);

        if (allocatedNow == 0 || global == 0)
            return allocatedNow;
    } else if (global != 0) {
        return global->allocate(size);
    } else {
        return size;
    }

    // If we have successfully allocated at least one byte from the
    // local allocator and the global allocator is present we should
    // request the same amount of bandwidth from the global allocator.
    int globallyAllocated = global->allocate(allocatedNow);

    if (globallyAllocated == allocatedNow) {
        return allocatedNow;
    } else {
        assert(globallyAllocated >= 0);
        assert(globallyAllocated < allocatedNow);

        local->release(allocatedNow - globallyAllocated);

        return globallyAllocated;
    }
}

void Socket::releaseBandwidth(BandwidthAllocator *local, BandwidthAllocator *global, int size)
{
    if (size > 0) {
        if (local != 0)
            local->release(size);

        if (global != 0)
            global->release(size);
    }
}

ssize_t Socket::read()
{
    std::string buffer;
    ssize_t received = 0;
    ssize_t allocated = 0;

    // Receive incoming data in 16 KB chunks.
    do {
        allocated = allocateBandwidth(localDownloadAllocator_, globalDownloadAllocator_, 0x4000);

        if (allocated > 0) {
            received = receive(buffer, allocated);
        } else {
            break;
        }
    } while (received == allocated);

    // Subsequent middleware calls might change the size of the buffer
    // thus we need to store the total amount of data we received now.
    size_t totalReceived = buffer.size();

    if (received >= 0) {
        // Last receive() call might left some bandwidth unused we
        // should return it back to the allocators.
        releaseBandwidth(localDownloadAllocator_, globalDownloadAllocator_, allocated - received);

        if (input_ && totalReceived > 0)
            input_->receive(*this, buffer);
    } else if (received == -1) {
        releaseBandwidth(localDownloadAllocator_, globalDownloadAllocator_, allocated);
        close();
    }

    return totalReceived;
}

ssize_t Socket::write()
{
    ssize_t wrote = 0;

    if (output_)
        output_->write(*this);

    while (!packetQueue_.empty()) {
        Packet *packet = packetQueue_.front();

        // Load pending data from the new packet if we never sent
        // anything before or the last time we sent all packets
        // completely.
        if (pendingData_.empty()) {
            pendingData_ = packet->serialize();
            pendingOffset_ = 0;
        }

        assert(!pendingData_.empty());

        // Don't send the packet if it has been discarded and no data
        // from it has been uploaded yet.
        if (packet->discarded() && pendingOffset_ == 0) {
            packetQueue_.pop_front();
            delete packet;

            continue;
        }

        int remain = pendingData_.size() - pendingOffset_;
        int allocated = allocateBandwidth(localUploadAllocator_, globalUploadAllocator_, remain);

        if (allocated == 0)
            return wrote;

        ssize_t sent = send(pendingData_.data() + pendingOffset_, allocated);

        if (sent == remain) {
            // Sent the whole packet.

            // Don't issue the onSent() callback if the packet was
            // discarded, because the sender is, obviously, not
            // interested in learning about this packet being
            // successfully sent anymore.
            if (!packet->discarded())
                packet->onSent();

            packetQueue_.pop_front();
            pendingData_.clear();
            pendingOffset_ = 0;

            wrote += sent;

            delete packet;
        } else if (sent >= 0) {
            // Sent only a part of the packet.
            assert(sent >= 0 && sent <= allocated);

            releaseBandwidth(localUploadAllocator_, globalUploadAllocator_, allocated - sent);

            pendingOffset_ += sent;
            wrote += sent;

            return wrote;
        } else {
            releaseBandwidth(localUploadAllocator_, globalUploadAllocator_, allocated);
            close();

            return wrote;
        }
    }

    return wrote;
}

void Socket::shutdown()
{
    if (input_)
        input_->shutdown(*this);

    if (output_)
        output_->shutdown(*this);
}
