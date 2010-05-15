#include <gtest/gtest.h>
#include <util/bitfield.hh>

using namespace Hypergrace;


TEST(BitfieldTest, TestLeftToRightOrdering)
{
    for (unsigned int i = 0; i < 8; ++i) {
        Util::Bitfield bf(8);

        bf.set(i);
        ASSERT_TRUE(bf.bit(i));
        ASSERT_EQ(0x80U >> i, (unsigned int)bf.byte(0));

        bf.unset(i);
        ASSERT_FALSE(bf.bit(i));
        ASSERT_EQ(0U, (unsigned int)bf.byte(0));
    }
}

TEST(BitfieldTest, TestBoundaryCrossing)
{
    Util::Bitfield bf(9);

    bf.set(8);
    ASSERT_TRUE(bf.bit(8));
    ASSERT_EQ(0x80U, bf.byte(1));

    bf.unset(8);
    ASSERT_FALSE(bf.bit(8));
    ASSERT_EQ(0U, bf.byte(1));
}
