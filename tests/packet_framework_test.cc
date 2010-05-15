#include <cstdint>
#include <utility>

#include <debug/debug.hh>

#include <gtest/gtest.h>
#include <net/simplepacket.hh>

#include <array>
#include <string>

using namespace Hypergrace::Net;


TEST(PacketFrameworkTest, BasicPrimitivesWork)
{
    {
        typedef SimplePacket<ByteMatcher> PacketType;
        PacketType p;

        ASSERT_TRUE(p.absorb("\xFF"));
        ASSERT_FALSE(PacketType().absorb(""));

        ASSERT_EQ(0xFF, p.field<0>());
        ASSERT_EQ("\xFF", p.serialize());
        ASSERT_EQ(1, p.size());
    }

    {
        typedef SimplePacket<BigEndianIntegerMatcher<uint32_t > > PacketType32;
        typedef SimplePacket<BigEndianIntegerMatcher<uint16_t > > PacketType16;

        PacketType32 p32;
        PacketType16 p16;

        ASSERT_TRUE(p16.absorb("\x41\x42"));
        ASSERT_TRUE(p32.absorb("\x41\x42\x43\x44"));

        ASSERT_FALSE(PacketType32().absorb("\x41\x42\x43"));
        ASSERT_FALSE(PacketType16().absorb("\x41"));

        ASSERT_EQ(0x41424344, p32.field<0>());
        ASSERT_EQ(0x4142, p16.field<0>());

        ASSERT_EQ("\x41\x42\x43\x44", p32.serialize());
        ASSERT_EQ("\x41\x42", p16.serialize());

        ASSERT_EQ(4, p32.serialize().size());
        ASSERT_EQ(2, p16.serialize().size());

        ASSERT_EQ(4, p32.size());
        ASSERT_EQ(2, p16.size());
    }

    {
        typedef SimplePacket<StaticLengthStringMatcher<6> > PacketType;
        PacketType p;

        ASSERT_TRUE(p.absorb("abcdefg"));
        ASSERT_FALSE(PacketType().absorb("abcde"));

        ASSERT_EQ("abcdef", p.field<0>());
        ASSERT_EQ("abcdef", p.serialize());
        ASSERT_EQ(6, p.size());
    }
}

TEST(PacketFrameworkTest, CompositionOfPrimitivesWorks)
{
    {
        typedef SimplePacket<
            BigEndianIntegerMatcher<uint16_t>,
            ConcreteLengthStringMatcher<0>
        > PacketType;

        const char sample[] = "\x00\x05""abcdefgh";

        PacketType p;

        ASSERT_TRUE(p.absorb(std::string(sample, sizeof(sample) - 1)));
        ASSERT_FALSE(PacketType().absorb(std::string(sample, sizeof(sample) - 5)));

        ASSERT_EQ(7, p.size());

        ASSERT_EQ("abcde", p.field<1>());
        ASSERT_EQ(std::string("\x00\x05""abcde", p.size()), p.serialize());
    }

    {
        typedef SimplePacket<
            BigEndianIntegerMatcher<int32_t>,
            BigEndianIntegerMatcher<int16_t>,
            ConcreteLengthStringMatcher<1>,
            VariableLengthStringMatcher<0>
        > PacketType;

        const char sample[] = "\x00\x00\x00\x18\x00\x0B""ConcreteLenVariableLen...";

        PacketType p;

        ASSERT_TRUE(p.absorb(std::string(sample, sizeof(sample) - 1)));
        ASSERT_FALSE(PacketType().absorb(std::string(sample, sizeof(sample) - 5)));

        ASSERT_EQ(28, p.size());

        ASSERT_EQ(24, p.field<0>());
        ASSERT_EQ(11, p.field<1>());
        ASSERT_EQ("ConcreteLen", p.field<2>());
        ASSERT_EQ("VariableLen", p.field<3>());

        ASSERT_EQ(std::string(sample, p.size()), p.serialize());
    }
}
