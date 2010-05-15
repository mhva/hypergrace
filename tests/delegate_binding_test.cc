#include <gtest/gtest.h>

#include <exception>
#include <functional>
#include <stdexcept>

#include <debug/debug.hh>
#include <delegate/delegate.hh>

using namespace Hypergrace;


class FailOnCopy
{
public:
    FailOnCopy() = default;
    FailOnCopy(const FailOnCopy &)
    {
        ADD_FAILURE() << "Copy constructor has been called";
    }

    const FailOnCopy &operator =(const FailOnCopy &)
    {
        ADD_FAILURE() << "Assignment operator has been called";
        return *this;
    }

    static void passage(FailOnCopy &) {};
};

class BindingTest : public ::testing::Test
{
public:
    static void st_1(int a)
    {
        // Throw an exception so we know that the function is
        // reachable
        ASSERT_EQ(1, a);

        throw std::runtime_error("");
    }

    static void st_2(int a, int b)
    {
        ASSERT_EQ(1, a);
        ASSERT_EQ(2, b);

        throw std::runtime_error("");
    }

    static void st_3(int a, int b, int c)
    {
        ASSERT_EQ(1, a);
        ASSERT_EQ(2, b);
        ASSERT_EQ(3, c);

        throw std::runtime_error("");
    }

    static void st_4(int a, int b, int c, int d)
    {
        ASSERT_EQ(1, a);
        ASSERT_EQ(2, b);
        ASSERT_EQ(3, c);
        ASSERT_EQ(4, d);

        throw std::runtime_error("");
    }

    static void st_5(int a, int b, int c, int d, int e)
    {
        ASSERT_EQ(1, a);
        ASSERT_EQ(2, b);
        ASSERT_EQ(3, c);
        ASSERT_EQ(4, d);
        ASSERT_EQ(5, e);

        throw std::runtime_error("");
    }

    static void st_6(int a, int b, int c, int d, int e, int f)
    {
        ASSERT_EQ(1, a);
        ASSERT_EQ(2, b);
        ASSERT_EQ(3, c);
        ASSERT_EQ(4, d);
        ASSERT_EQ(5, e);
        ASSERT_EQ(6, f);

        throw std::runtime_error("");
    }

    static void st_7(int a, int b, int c, int d, int e, int f, int g)
    {
        ASSERT_EQ(1, a);
        ASSERT_EQ(2, b);
        ASSERT_EQ(3, c);
        ASSERT_EQ(4, d);
        ASSERT_EQ(5, e);
        ASSERT_EQ(6, f);
        ASSERT_EQ(7, g);

        throw std::runtime_error("");
    }

    void t_1(int a) const { st_1(a); }
    void t_2(int a, int b) { st_2(a, b); }
    void t_3(int a, int b, int c) { st_3(a, b, c); }
    void t_4(int a, int b, int c, int d) { st_4(a, b, c, d); }
    void t_5(int a, int b, int c, int d, int e) { st_5(a, b, c, d, e); }
    void t_6(int a, int b, int c, int d, int e, int f) { st_6(a, b, c, d, e, f); }

    int sum(int a, int b) { return a + b; }
};

TEST_F(BindingTest, NaiveBindingWorks)
{
    typedef Delegate::Delegate<void ()> D;

    // Test if enclosed functions are reachable and can be properly
    // called

    /* Static functions */
    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::st_1, 1)())
        << "st_1 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::st_2, 1, 2)())
        << "st_2 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::st_3, 1, 2, 3)())
        << "st_3 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::st_4, 1, 2, 3, 4)())
        << "st_4 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::st_5, 1, 2, 3, 4, 5)())
        << "st_5 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::st_6, 1, 2, 3, 4, 5, 6)())
        << "st_6 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::st_7, 1, 2, 3, 4, 5, 6, 7)())
        << "st_7 is unreachable";

    /* Member functions */
    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::t_1, this, 1)())
        << "t_1 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::t_2, this, 1, 2)())
        << "t_2 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::t_3, this, 1, 2, 3)())
        << "t_3 is unreachable";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::t_4, this, 1, 2, 3, 4)())
        << "t_4 is inacessible";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::t_5, this, 1, 2, 3, 4, 5)())
        << "t_5 is inacessible";

    ASSERT_ANY_THROW(Delegate::bind(&BindingTest::t_6, this, 1, 2, 3, 4, 5, 6)())
        << "t_6 is inacessible";

    /* Function objects */
    auto greaterThan8 = Delegate::bind<bool>(std::greater<int>(), _1, 8);
    ASSERT_TRUE(greaterThan8(9));
    ASSERT_FALSE(greaterThan8(8));
}

TEST_F(BindingTest, DoesntCreateUnneccessaryCopies)
{
    FailOnCopy fc;
    Delegate::bind(&FailOnCopy::passage, _1)(fc);
}

TEST_F(BindingTest, DelegateDoesSupportBinder)
{
    Delegate::Delegate<int (int)> d1(Delegate::bind(&BindingTest::sum, this, 41, _1));
    ASSERT_EQ(42, d1(1));

    Delegate::Delegate<void (int)> d2 = Delegate::bind(&BindingTest::t_4, this, 1, 2, _1, 4);
    ASSERT_ANY_THROW(d2(3))
        << "The bound function t_4 was unreachable when calling through delegate";
}

TEST_F(BindingTest, PlaceholdersWork)
{
    // All arguments are being placeholders (even the pointer to *this)
    {
        auto f = Delegate::bind(&BindingTest::t_6, _1, _2, _3, _4, _5, _6, _7);
        ASSERT_ANY_THROW(f(this, 1, 2, 3, 4, 5, 6));
    }

    // Mixture of placeholders and real arguments
    {
        auto f = Delegate::bind(&BindingTest::t_5, this, _1, 2, _2, 4, _3);
        ASSERT_ANY_THROW(f(1, 3, 5));
    }
}

TEST_F(BindingTest, BinderCanProperlyReturnValue)
{
    auto f = Delegate::bind(&BindingTest::sum, this, 41, _1);
    ASSERT_EQ(42, f(1));
}
