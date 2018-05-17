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

#ifndef NET_CONNECTION_HH_
#define NET_CONNECTION_HH_

#include <deque>
#include <string>
#include <utility>

#include <delegate/delegate.hh>
#include <delegate/signal.hh>

#include <net/hostaddress.hh>
#include <net/inputmiddleware.hh>
#include <net/outputmiddleware.hh>

namespace Hypergrace { namespace Net { class BandwidthAllocator; }}
namespace Hypergrace { namespace Net { class Packet; }}
namespace Hypergrace { namespace Net { class Reactor; }}


namespace Hypergrace {
namespace Net {

class Socket
{
public:
    Socket(int, const HostAddress &);
    virtual ~Socket();

    /**
     * TODO: Documentation
     */
    void send(Packet *);

    virtual ssize_t send(const char *, size_t) = 0;
    virtual ssize_t receive(std::string &, size_t) = 0;

    void setLocalBandwidthAllocators(BandwidthAllocator *, BandwidthAllocator *);
    void setGlobalBandwidthAllocators(BandwidthAllocator *, BandwidthAllocator *);

    void setInputMiddleware(Net::InputMiddleware::Pointer);
    void setOutputMiddleware(Net::OutputMiddleware::Pointer);

    /**
     * Triggers socket into closed state.
     *
     * Socket will not be closed immideately and the file descriptor
     * will be valid for a while. Observing reactor will eventually
     * notify socket's change of state and will purge it.
     */
    void close();

    /**
     * Checks whether the socket in the closed state.
     *
     * Returns false if the socket is not scheduled to be closed or
     * true otherwise.
     */
    bool closed() const;

    /**
     * Returns socket's file descriptor.
     */
    int fd() const;

    /**
     * Returns input middleware chain associated with the current
     * socket.
     */
    Net::InputMiddleware &inputMiddleware();

    /**
     * Returns output middleware chain associated with the current
     * socket.
     */
    Net::OutputMiddleware &outputMiddleware();

    /**
     * Returns the address of connected peer.
     *
     * If the socket is bound to a local address the returned address
     * will be invalid.
     */
    const HostAddress &remoteAddress() const;

    /**
     * Makes a duplicate of socket's file descriptor.
     *
     * If operation succeeds the resulting socket file descriptor
     * must be closed manually either by issuing the close() system
     * call or deleting Socket instance to which the new file
     * descriptor was assigned.
     *
     * On success returns a duplicate file descriptor and -1 on failure.
     */
    int cloneFd();

    void setData(void *data) { data_ = data; }
    void *data() const { return data_; }

public:
    Socket(const Socket &) = delete;

    bool operator =(const Socket &) = delete;

private:
    int allocateBandwidth(BandwidthAllocator *, BandwidthAllocator *, int);
    void releaseBandwidth(BandwidthAllocator *, BandwidthAllocator *, int);

private:
    friend class Net::Reactor;

    ssize_t read();
    ssize_t write();
    void shutdown();

private:
    const int socket_;
    HostAddress remoteAddress_;

    Net::InputMiddleware::Pointer input_;
    Net::OutputMiddleware::Pointer output_;

    BandwidthAllocator *localDownloadAllocator_;
    BandwidthAllocator *localUploadAllocator_;
    BandwidthAllocator *globalDownloadAllocator_;
    BandwidthAllocator *globalUploadAllocator_;

    bool closed_;

    std::deque<Packet *> packetQueue_;

    std::string pendingData_;
    size_t pendingOffset_;

    void *data_;
};

} /* namespace Net */
} /* namespace Hypergrace */

#endif /* NET_CONNECTION_HH_ */
