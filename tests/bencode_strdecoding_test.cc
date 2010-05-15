#include <sstream>
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

static void testValidStrings() {}

template<typename T, typename... Rest>
static void testValidStrings(const T &testPair, const Rest &... rest)
{
    Object *object = Reader::parse(testPair.first);
    ASSERT_TRUE(object != 0) << "Decoder has failed to parse the input: ("
                             << testPair.first << ')';
    ASSERT_EQ(object->get<String>(), testPair.second);

    ReleaseMemoryVisitor v;
    object->accept(v);

    testValidStrings(rest...);
}

TEST(BencodeStrDecodingTest, HandlesValidStrings)
{
    testValidStrings(std::make_pair("5:abcde", "abcde"),
                     std::make_pair("10:0123456789", "0123456789"),
                     std::make_pair("1:a", "a"),
                     std::make_pair("9:notes.txt", "notes.txt"));
}

TEST(BencodeStrDecodingTest, HandlesAllCharacters)
{
    std::string original;
    original.reserve(256);

    for (char i = -128; i < 127; ++i)
        original.append(1, i);

    std::ostringstream prelude;
    prelude << original.size() << ':' << original;
    testValidStrings(std::make_pair(prelude.str(), original));
}

TEST(BencodeStrDecodingTest, RejectsHugeStrings)
{
    SUPPRESS_OUTPUT;
    std::string huge;
    std::ostringstream prelude;
    size_t length = 40 * 1024 * 1024;

    prelude << length << ':';

    huge.reserve(length + 4);
    huge.append(prelude.str());
    huge.append(length, 'a');

    ASSERT_TRUE(Reader::parse(huge) == 0);
}

TEST(BencodeStrDecodingTest, HandlesBogusInput)
{
    SUPPRESS_OUTPUT;
    ASSERT_TRUE(Reader::parse("0:") == 0);
    ASSERT_TRUE(Reader::parse("1:") == 0);
    ASSERT_TRUE(Reader::parse("2:") == 0);
    ASSERT_TRUE(Reader::parse("100:") == 0);
    ASSERT_TRUE(Reader::parse("-1:abcd") == 0);
    ASSERT_TRUE(Reader::parse("2") == 0);
}
