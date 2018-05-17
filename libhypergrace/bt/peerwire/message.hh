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

#ifndef BT_PEERWIRE_MESSAGE_HH_
#define BT_PEERWIRE_MESSAGE_HH_

#include <cassert>
#include <cstdint>

#include <net/simplepacket.hh>

#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>


namespace Hypergrace {
namespace Bt {

enum { BitTorrentPacketClass = 1 };

template<unsigned char MessageId_, typename... Matchers>
class Message :
      public Net::SimplePacket<
                 Net::BigEndianIntegerMatcher<uint32_t>,
                 Net::ByteMatcher,
                 Matchers...
             >
{
public:
    typedef Net::SimplePacket<
        Net::BigEndianIntegerMatcher<uint32_t>,
        Net::ByteMatcher,
        Matchers...
    > BaseClass;

    enum { id = MessageId_ };

public:
    Message() = default;
    virtual ~Message() = default;

    Message(const typename Matchers::ValueType &... args)
        : BaseClass(0, MessageId_, args...)
    {
        BaseClass::template modify<0>(this->size() - this->template size<0>());
        this->setPacketClass(BitTorrentPacketClass);
        this->setPacketId(MessageId_);
    }
};

template<unsigned char MessageId_>
class Message<MessageId_> :
      public Net::SimplePacket<
                 Net::BigEndianIntegerMatcher<uint32_t>,
                 Net::ByteMatcher
             >
{
public:
    typedef Net::SimplePacket<
        Net::BigEndianIntegerMatcher<uint32_t>,
        Net::ByteMatcher
    > BaseClass;

    enum { id = MessageId_ };

public:
    Message() :
        BaseClass(0, MessageId_)
    {
        BaseClass::template modify<0>(this->size() - this->template size<0>());
        this->setPacketClass(BitTorrentPacketClass);
        this->setPacketId(MessageId_);
    }

    virtual ~Message() = default;
};

class HandshakeMessage
    : public Net::SimplePacket<
                 Net::ByteMatcher,
                 Net::StaticLengthStringMatcher<sizeof("BitTorrent protocol") - 1>,
                 Net::StaticLengthStringMatcher<8>,
                 Net::StaticLengthStringMatcher<InfoHash::static_size>,
                 Net::StaticLengthStringMatcher<PeerId::static_size>
             >
{
public:
    typedef Net::SimplePacket<
        Net::ByteMatcher,
        Net::StaticLengthStringMatcher<sizeof("BitTorrent protocol") - 1>,
        Net::StaticLengthStringMatcher<8>,
        Net::StaticLengthStringMatcher<InfoHash::static_size>,
        Net::StaticLengthStringMatcher<PeerId::static_size>
    > BaseClass;

public:
    HandshakeMessage()
    {
    }

    HandshakeMessage(const InfoHash &hash, const PeerId &peerId) :
        BaseClass(sizeof("BitTorrent protocol") - 1,
                  "BitTorrent protocol",
                  std::string(8, 0), hash, peerId)
    {
    }

    virtual ~HandshakeMessage() = default;

private:
    HandshakeMessage(const BaseClass::TupleType &tuple) : BaseClass(tuple) {}
};

typedef Message<0> ChokeMessage;
typedef Message<1> UnchokeMessage;
typedef Message<2> InterestedMessage;
typedef Message<3> NotInterestedMessage;
typedef Message<4, Net::BigEndianIntegerMatcher<uint32_t> > HaveMessage;
typedef Message<5, Net::VariableLengthStringMatcher<0> > BitfieldMessage;

typedef Message<6,
    Net::BigEndianIntegerMatcher<uint32_t>,
    Net::BigEndianIntegerMatcher<uint32_t>,
    Net::BigEndianIntegerMatcher<uint32_t>
> RequestMessage;

typedef Message<7,
    Net::BigEndianIntegerMatcher<uint32_t>,
    Net::BigEndianIntegerMatcher<uint32_t>,
    Net::VariableLengthStringMatcher<0>
> PieceMessage;

typedef Message<8,
    Net::BigEndianIntegerMatcher<uint32_t>,
    Net::BigEndianIntegerMatcher<uint32_t>,
    Net::BigEndianIntegerMatcher<uint32_t>
> CancelMessage;

} /* namespace Bt */

namespace Net {

template<>
struct YieldPacketSize<Bt::HandshakeMessage>
{
    static const size_t value = YieldPacketSize<Bt::HandshakeMessage::BaseClass>::value;
};

template<unsigned int id_, typename... Args>
struct YieldPacketSize<Bt::Message<id_, Args...> >
{
    static const size_t value =
        YieldPacketSize<typename Bt::Message<id_, Args...>::BaseClass>::value;
};

}

} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_MESSAGE_HH_ */
