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
   write to the  Free Software Foundation, Inc.,  51 Franklin Sresponse_t,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <arpa/inet.h>

#include <exception>
#include <cstdint>
#include <stdexcept>
#include <sstream>

#include <debug/debug.hh>

#include <bencode/object.hh>
#include <bencode/path.hh>
#include <bencode/reader.hh>
#include <bencode/releasememoryvisitor.hh>
#include <bencode/typedefs.hh>
#include <bencode/trivialobject.hh>

#include "trackerresponse.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


TrackerResponse::TrackerResponse(const std::string &s) :
    response_(0)
{
    try {
        response_ = Bencode::Reader::parse(s);

        // TODO: ^ Rewrite Bencode::Reader::parse()
        if (response_ == 0)
            throw std::runtime_error("Returning 0 or throwing an exception on failure is "
                                     "fucking moronic. Fuck me.");
    } catch (std::exception &e) {
        cleanUp();
        hDebug() << e.what();
        throw std::runtime_error("Tracker has responded with malformed Bencode content.");
    }

    // Verify tracker response
    try {
        Bencode::Path(response_, "interval").resolve<Bencode::Integer>();
        Bencode::Path(response_, "min interval").resolve<Bencode::Integer>(0);

        Bencode::Path(response_, "failure reason").resolve<Bencode::String>("");
        Bencode::Path(response_, "tracker id").resolve<Bencode::String>("");
        Bencode::Path(response_, "complete").resolve<Bencode::Integer>();
        Bencode::Path(response_, "incomplete").resolve<Bencode::Integer>();

        // Verify peer list
        if (!Bencode::Path(response_, "peers").exists())
            throw std::runtime_error("Peer list doesn't exist.");

        // Some black magic type-id stuff is going here.
        // XXX: Might be good idea to add a proper type identification
        try {
            // If Bencode::Path::resolve() will not throw an exception
            // then the peer list is standard.
            Bencode::Path(response_, "peers").resolve<Bencode::List>();
            peerListIsCompact_ = false;
        } catch (...) {
            // Thrown exception means that the peer list is compact.
            peerListIsCompact_ = true;
        }

        if (peerListIsCompact_) {
            auto &zippedPeers = Bencode::Path(response_, "peers").resolve<Bencode::String>();

            if (zippedPeers.size() % 6 != 0) {
                throw std::runtime_error("The length of the string containing compressed "
                                         "peer information must be a multiple of 6.");
            }
        } else {
            auto &bePeers = Bencode::Path(response_, "peers").resolve<Bencode::List>();

            for (auto peer = bePeers.begin(); peer != bePeers.end(); ++peer) {
                Bencode::Path(*peer, "peer id").resolve<Bencode::String>();
                Bencode::Path(*peer, "ip").resolve<Bencode::String>();
                Bencode::Path(*peer, "port").resolve<Bencode::Integer>();
            }
        }
    } catch (std::exception &e) {
        cleanUp();
        hDebug() << e.what();
        throw std::runtime_error("Tracker response dictionary contains invalid data.");
    }
}

TrackerResponse::~TrackerResponse()
{
    cleanUp();
}

bool TrackerResponse::failed() const
{
    return Bencode::Path(response_, "failure reason").exists();
}

std::string TrackerResponse::errorMessage() const
{
    return Bencode::Path(response_, "failure reason").resolve<Bencode::String>("");
}

std::string TrackerResponse::warningMessage() const
{
    return Bencode::Path(response_, "warning message").resolve<Bencode::String>("");
}

std::string TrackerResponse::trackerId() const
{
    return Bencode::Path(response_, "tracker id").resolve<Bencode::String>("");
}

int TrackerResponse::seederCount() const
{
    return Bencode::Path(response_, "complete").resolve<Bencode::Integer>(0);
}

int TrackerResponse::leecherCount() const
{
    return Bencode::Path(response_, "incomplete").resolve<Bencode::Integer>(0);
}

int TrackerResponse::peerCount() const
{
    if (peerListIsCompact_)
        return Bencode::Path(response_, "peers").resolve<Bencode::String>().size() / 6;
    else
        return Bencode::Path(response_, "peers").resolve<Bencode::List>().size();
}

int TrackerResponse::announceInterval() const
{
    return Bencode::Path(response_, "interval").resolve<Bencode::Integer>(1200);
}

int TrackerResponse::minAnnounceInterval() const
{
    return Bencode::Path(response_, "min interval").resolve<Bencode::Integer>(0);
}

Net::HostAddress zipToHostAddress(const char *data, size_t offset)
{
    union IpAddress
    {
        uint32_t block;
        unsigned char parts[sizeof(uint32_t)];
    };

    IpAddress ipaddr = { *(uint32_t *)&data[offset] };
    uint16_t port = ntohs(*(uint16_t *)&data[offset + sizeof(ipaddr)]);

    // Convert ip address into the dotted-quad format
    std::ostringstream out;
    out << (int)ipaddr.parts[0] << "." << (int)ipaddr.parts[1] << "."
        << (int)ipaddr.parts[2] << "." << (int)ipaddr.parts[3];

    return Net::HostAddress(out.str(), port);
}

PeerList TrackerResponse::peers() const
{
    PeerList peers;

    if (peerListIsCompact_) {
        auto &beZip = Bencode::Path(response_, "peers").resolve<Bencode::String>();

        for (size_t offset = 0; offset < beZip.size(); offset += 6) {
            Net::HostAddress address = zipToHostAddress(beZip.c_str(), offset);

            if (address.valid())
                peers.push_back(address);
        }
    } else {
        auto &bePeers = Bencode::Path(response_, "peers").resolve<Bencode::List>();

        for (auto peer = bePeers.begin(); peer != bePeers.end(); ++peer) {
            std::string ip = Bencode::Path(*peer, "ip").resolve<Bencode::String>();
            uint16_t port = Bencode::Path(*peer, "port").resolve<Bencode::Integer>();
            Net::HostAddress address(ip, port);

            if (address.valid())
                peers.push_back(address);
        }
    }

    return peers;
}

void TrackerResponse::cleanUp()
{
    if (response_ != 0) {
        Bencode::ReleaseMemoryVisitor freemem;
        response_->accept(freemem);
    }
}
