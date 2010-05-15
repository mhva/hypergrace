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

#include <cassert>

#include <bt/peerwire/message.hh>
#include <bt/types.hh>

#include <net/socket.hh>
#include <net/simplepacket.hh>

#include "handshakenegotiator.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;

typedef Net::SimplePacket<
    Net::ByteMatcher,
    Net::StaticLengthStringMatcher<sizeof("BitTorrent protocol") - 1>,
    Net::StaticLengthStringMatcher<8>,
    Net::StaticLengthStringMatcher<InfoHash::static_size>
> HashPart;


HandshakeNegotiator::HandshakeNegotiator() :
    hashAlreadyDiscovered_(false)
{
}

HandshakeNegotiator::~HandshakeNegotiator()
{
}

void HandshakeNegotiator::receive(Net::Socket &socket, std::string &data)
{
    const static size_t minimumSize = Net::YieldPacketSize<HashPart>::value;
    const static size_t requiredSize = Net::YieldPacketSize<HandshakeMessage>::value;

    buffer_.append(data);

    // We've got enough data to peek into the info hash.
    if (!hashAlreadyDiscovered_ && buffer_.size() >= minimumSize) {
        HashPart hp;

        if (!hp.absorb(buffer_) || hp.field<1>() != "BitTorrent protocol") {
            socket.close();
            return;
        }

        const InfoHash *hash = reinterpret_cast<const InfoHash *>(hp.field<3>().data());

        if (!processInfoHash(socket, *hash)) {
            socket.close();
            return;
        }

        hashAlreadyDiscovered_ = true;
    }

    // Handshake packet completely assembled.
    if (buffer_.size() >= requiredSize) {
        HandshakeMessage hsm;

        if (!hsm.absorb(buffer_)) {
            hDebug() << "Something went wrong while shaking hands with peer";
            socket.close();
            return;
        }

        if (!peerSettings_.infoHash.fromString(hsm.field<3>()) ||
            !peerSettings_.peerId.fromString(hsm.field<4>()))
        {
            assert(!"Inconsistency between sizes of HandshakePacket's fields and Array");
            socket.close();
            return;
        }

        peerSettings_.socketFd = socket.cloneFd();
        peerSettings_.address = socket.remoteAddress();
        memcpy(peerSettings_.supportedFeatures, hsm.field<2>().data(), hsm.size<2>());
        peerSettings_.streamContinuation = buffer_.substr(requiredSize);

        socket.close();

        reconfigurePeer(peerSettings_);
    }
}

void HandshakeNegotiator::shutdown(Net::Socket &)
{
}
