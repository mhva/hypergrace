#include <gtest/gtest.h>

#include <util/time.hh>

using namespace Hypergrace;
using namespace Util;


TEST(TimeTest, MakesGoodTimeValues)
{
    {
        Time t(24, 32, 12);
        ASSERT_EQ(24, t.hour());
        ASSERT_EQ(32, t.minute());
        ASSERT_EQ(12, t.second());
        ASSERT_EQ(0, t.millisecond());
    }

    {
        Time t(0, 9, 61);
        ASSERT_EQ(0, t.hour());
        ASSERT_EQ(10, t.minute());
        ASSERT_EQ(1, t.second());
        ASSERT_EQ(0, t.millisecond());
    }

    {
        Time t(0, 0, 3661);
        ASSERT_EQ(1, t.hour());
        ASSERT_EQ(1, t.minute());
        ASSERT_EQ(1, t.second());
        ASSERT_EQ(0, t.millisecond());
    }

    {
        // Initialize by passing milliseconds
        Time t(36611001);

        ASSERT_EQ(10, t.hour());
        ASSERT_EQ(10, t.minute());
        ASSERT_EQ(11, t.second());
        ASSERT_EQ(1, t.millisecond());
    }
}

TEST(TimeTest, ProperlyConvertsUnits)
{
    Time t(0, 0, 3661);

    ASSERT_EQ(1, t.toHours());
    ASSERT_EQ(61, t.toMinutes());
    ASSERT_EQ(3661, t.toSeconds());
    ASSERT_EQ(3661000, t.toMilliseconds());
}

TEST(TimeTest, DefaultConstructorInitializesToZero)
{
    ASSERT_EQ(0, Time().toMilliseconds());
}

TEST(TimeTest, ExcelentAtArithmetics)
{
    {
        Time result = Time(2100) - Time(200);

        ASSERT_EQ(1, result.second());
        ASSERT_EQ(900, result.millisecond());
        ASSERT_EQ(1900, result.toMilliseconds());
    }

    {
        Time result = Time(1500) + Time(700);

        ASSERT_EQ(2, result.second());
        ASSERT_EQ(200, result.millisecond());
        ASSERT_EQ(2200, result.toMilliseconds());
    }

    ASSERT_TRUE((Time(1) - Time(200000)) == Time());
}
