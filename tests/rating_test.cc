#include <gtest/gtest.h>

#include <debug/debug.hh>
#include <util/rating.hh>
#include <net/hostaddress.hh>

using namespace Hypergrace::Util;


TEST(RatingTest, CalculatesCorrectRatings)
{
    ASSERT_TRUE(Rating(1, 2) < Rating(100, 200));
    ASSERT_TRUE(Rating(1, 1) < Rating(100, 100));
    ASSERT_TRUE(Rating(1, 0) < Rating(100, 0));
    ASSERT_TRUE(Rating(5, 10) < Rating(50, 100));
    ASSERT_TRUE(Rating(100, 1) < Rating(100, 0));

    // TODO: Add more tests
}
