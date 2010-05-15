#include <stdlib.h>
#include <time.h>

#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <bencode/object.hh>
#include <bencode/typedefs.hh>
#include <bencode/trivialobject.hh>
#include <bencode/releasememoryvisitor.hh>
#include <bencode/reader.hh>

#include "outputsuppressor.hh"

using namespace Hypergrace;
using namespace Bencode;

// TODO: Once we have a library method that does memory bookkeeping,
// this one should be deleted.
static void releaseObject(Object *o)
{
    ReleaseMemoryVisitor v;
    o->accept(v);
}

class BencodeIntDecodingTest : public ::testing::Test {
protected:
    typedef std::pair<char *, long long> GenerateResult;

    GenerateResult generateInt(const bool negative, const int length)
    {
        char *stringResult = new char[length + 5];
        long long intResult = 0;
        char *pos = stringResult;

        memset(stringResult, 0, length + 5);

        *pos++ = 'i';
        if (negative)
            *pos++ = '-';
        stringResult[length + negative + 1] = 'e';

        for (; pos < stringResult + negative + length + 1; ++pos) {
            int num = rand() % 9;
            *pos = num + '0';
            intResult = intResult * 10 + num;
        }

        return (!negative) ? GenerateResult(stringResult, intResult)
                           : GenerateResult(stringResult, -intResult);
    }

    virtual void SetUp()
    {
        srand(time(0));
    }
};

TEST_F(BencodeIntDecodingTest, HandlesValidInt)
{
    for (int i = 0; i < 1000; ++i) {
        GenerateResult r = generateInt(rand() % 2, 1 + rand() % 18);
        Object *object = Reader::parse(r.first);

        ASSERT_TRUE(object != 0) << "Decoder has a trouble parsing the following input: "
                                 << r.first;
        ASSERT_EQ(r.second, object->get<Integer>()) << "Decoder didn't handle "
                                                    << "the following input: "
                                                    << r.first;
        releaseObject(object);
        delete[] r.first;
    }
}

TEST_F(BencodeIntDecodingTest, HandlesIntOverflow)
{
    SUPPRESS_OUTPUT;
    ASSERT_TRUE(Reader::parse("i9223372036854775808e") == 0);

    for (int i = 0; i < 1000; ++i) {
        GenerateResult r = generateInt(false, 21);
        ASSERT_TRUE(Reader::parse(r.first) == 0) << "Decoder didn't handle overflow with the "
                                                 << "following input: " << r.first;
        delete[] r.first;
    }
}

TEST_F(BencodeIntDecodingTest, HandlesNegativeZero)
{
    Object *object = Reader::parse("i-0e");

    ASSERT_TRUE(object != 0);
    ASSERT_EQ(object->get<Integer>(), 0);

    releaseObject(object);
}

TEST_F(BencodeIntDecodingTest, HandlesBogusInput)
{
    SUPPRESS_OUTPUT;
    ASSERT_TRUE(Reader::parse("i-e") == 0);
    ASSERT_TRUE(Reader::parse("ie") == 0);
    ASSERT_TRUE(Reader::parse("i") == 0);
}
