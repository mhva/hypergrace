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

#include <cassert>
#include <cstdint>
#include <deque>
#include <memory>

#include <debug/debug.hh>

#include <bt/peerwire/message.hh>

#include <net/simplepacket.hh>
#include <net/socket.hh>

#include "messageassembler.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;

typedef Net::SimplePacket<Net::BigEndianIntegerMatcher<uint32_t> > MessageSizeField;
typedef Net::SimplePacket<Net::ByteMatcher> MessageIdField;


MessageAssembler::MessageAssembler(Bt::InputMiddleware::Pointer delegate) :
    delegate_(delegate)
{
}

void MessageAssembler::receive(Net::Socket &socket, std::string &data)
{
    if (!buffer_.empty())
        buffer_.append(data);
    else
        buffer_ = data;

    size_t offset = 0;
    MessageSizeField sizeField;

    while (sizeField.absorb(buffer_, offset)) {
        size_t payloadSize = sizeField.field<0>();

        if (payloadSize == 0) {
            // The message is the keep-alive message
            offset += Net::YieldPacketSize<MessageSizeField>::value;

            // TODO: Update socket liveness
            continue;
        } else if (payloadSize > 32768) {
            // Refuse to accept huge messages
            hDebug() << "Peer" << socket.remoteAddress() << "tried to send a message"
                     << "larger than 32 KiB. Connection will be dropped.";
            socket.close();
            return;
        }

        size_t messageSize = Net::YieldPacketSize<MessageSizeField>::value + payloadSize;

        if (buffer_.size() >= offset + messageSize) {
            // We can assemble at least one message.
            if (dispatchMessage(socket, offset)) {
                offset += messageSize;
            } else {
                socket.close();
                return;
            }
        } else {
            // Message is still being downloaded.
            break;
        }
    }

    // Remove parsed messages from buffer.
    buffer_ = buffer_.substr(offset);
}

void MessageAssembler::shutdown(Net::Socket &socket)
{
}

template<typename MessageClass>
bool MessageAssembler::delegateMessage(Net::Socket &socket, size_t bufferOffset)
{
    MessageClass message;

    if (message.absorb(buffer_, bufferOffset)) {
        delegate_->processMessage(socket, message);
        return true;
    } else {
        hDebug() << "Received an invalid message from" << socket.remoteAddress();
        return false;
    }
}

bool MessageAssembler::dispatchMessage(Net::Socket &socket, size_t bufferOffset)
{
    MessageIdField idField;

    bool r = idField.absorb(buffer_, bufferOffset + Net::YieldPacketSize<MessageSizeField>::value);

    assert(r == true);

    unsigned char messageId = idField.field<0>();

    switch (messageId) {
    case ChokeMessage::id:
        //hDebug() << socket.fd() << "Got choke message from" << socket.remoteAddress();
        return delegateMessage<ChokeMessage>(socket, bufferOffset);
    case UnchokeMessage::id:
        //hDebug() << socket.fd() << "Got unchoke message from" << socket.remoteAddress();
        return delegateMessage<UnchokeMessage>(socket, bufferOffset);
    case InterestedMessage::id:
        //hDebug() << socket.fd() << "Got interested message from" << socket.remoteAddress();
        return delegateMessage<InterestedMessage>(socket, bufferOffset);
    case NotInterestedMessage::id:
        //hDebug() << socket.fd() << "Got not-interested message from" << socket.remoteAddress();
        return delegateMessage<NotInterestedMessage>(socket, bufferOffset);
    case HaveMessage::id:
        //hDebug() << socket.fd() << "Got have message from" << socket.remoteAddress();
        return delegateMessage<HaveMessage>(socket, bufferOffset);
    case BitfieldMessage::id:
        //hDebug() << socket.fd() << "Got bitfield message from" << socket.remoteAddress();
        return delegateMessage<BitfieldMessage>(socket, bufferOffset);
    case RequestMessage::id:
        //hDebug() << socket.fd() << "Got request message from" << socket.remoteAddress();
        return delegateMessage<RequestMessage>(socket, bufferOffset);
    case PieceMessage::id:
        //hDebug() << socket.fd() << "Got piece message from" << socket.remoteAddress();
        return delegateMessage<PieceMessage>(socket, bufferOffset);
    case CancelMessage::id:
        //hDebug() << socket.fd() << "Got cancel message from" << socket.remoteAddress();
        return delegateMessage<CancelMessage>(socket, bufferOffset);
    default:
        //hDebug() << "Received unknown message" << (unsigned int)messageId
        //         << "from" << socket.remoteAddress();
        return false;
    }

    return false;
}
