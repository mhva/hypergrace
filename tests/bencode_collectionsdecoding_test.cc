#include <gtest/gtest.h>

#include <bencode/object.hh>
#include <bencode/path.hh>
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

TEST(BencodeCollectionsDecodingTest, HandlesValidDict)
{
    Object *object = Reader::parse("d8:username3:joe8:password3:abc4:home2:~/8:disabledi0ee");
    ASSERT_TRUE(object != 0);

    ASSERT_TRUE(Path(object, "username").exists());
    ASSERT_TRUE(Path(object, "password").exists());
    ASSERT_TRUE(Path(object, "home").exists());
    ASSERT_TRUE(Path(object, "disabled").exists());

    ASSERT_NO_THROW(ASSERT_TRUE(Path(object, "username").resolve<String>() == "joe"));
    ASSERT_NO_THROW(ASSERT_TRUE(Path(object, "password").resolve<String>() == "abc"));
    ASSERT_NO_THROW(ASSERT_TRUE(Path(object, "home").resolve<String>() == "~/"));
    ASSERT_NO_THROW(ASSERT_TRUE(Path(object, "disabled").resolve<Integer>() == 0));

    releaseObject(object);
}

TEST(BencodeCollectionsDecodingTest, HandlesValidList)
{
    Object *object = Reader::parse("li1ei2e5:threee");
    ASSERT_TRUE(object != 0);

    auto &list = object->get<List>();

    ASSERT_NO_THROW(ASSERT_TRUE(list[0]->get<Integer>() == 1));
    ASSERT_NO_THROW(ASSERT_TRUE(list[1]->get<Integer>() == 2));
    ASSERT_NO_THROW(ASSERT_TRUE(list[2]->get<String>() == "three"));

    releaseObject(object);
}

TEST(BencodeCollectionsDecodingTest, FetchesFromNestedDictionary)
{
    // d
    //     4:home
    //     d
    //         3:joe
    //         d
    //             4:docs
    //             d
    //                 8:smth.txt
    //                 i1e
    //                 9:notes.txt
    //                 4:test
    //             e
    //         e
    //         3:mom
    //         d
    //             5:files
    //             d
    //                 6:my.txt
    //                 i32767e
    //             e
    //         e
    //     e
    //     6:nested
    //     d
    //         4:list
    //         l1:a1:b1:ce
    //     e
    // e

    Object *object = Reader::parse("d4:homed3:joed4:docsd8:smth.txti1e9:notes.txt4:testee3:momd5:filesd6:my.txti32767eeee6:nestedd4:listl1:a1:b1:ceee");

    ASSERT_TRUE(object != 0);
    ASSERT_NO_THROW(ASSERT_TRUE(
        Path(object, "home", "joe", "docs", "smth.txt").resolve<Integer>() == 1
    ));
    ASSERT_NO_THROW(ASSERT_TRUE(
        Path(object, "home", "joe", "docs", "notes.txt").resolve<String>() == "test"
    ));
    ASSERT_NO_THROW(ASSERT_TRUE(
        Path(object, "home", "mom", "files", "my.txt").resolve<Integer>() == 32767
    ));

    List *list;
    ASSERT_NO_THROW(list = &Path(object, "nested", "list").resolve<List>());
    ASSERT_NO_THROW(ASSERT_TRUE((*list)[0]->get<String>() == "a"));
    ASSERT_NO_THROW(ASSERT_TRUE((*list)[1]->get<String>() == "b"));
    ASSERT_NO_THROW(ASSERT_TRUE((*list)[2]->get<String>() == "c"));

    releaseObject(object);
}

TEST(BencodeCollectionsDecodingTest, HandlesBogusDict)
{
    SUPPRESS_OUTPUT;
    ASSERT_TRUE(Reader::parse("d") == 0);
    ASSERT_TRUE(Reader::parse("di1e5:boguse") == 0);
    ASSERT_TRUE(Reader::parse("dli1ee5:boguse") == 0);
    ASSERT_TRUE(Reader::parse("dd4:dict3:vale5:boguse") == 0);
}

TEST(BencodeCollectionsDecodingTest, HandlesBogusList)
{
    SUPPRESS_OUTPUT;
    ASSERT_TRUE(Reader::parse("l") == 0);
}

TEST(BencodeCollectionsDecodingTest, HandlesEmptyDict)
{
    SUPPRESS_OUTPUT;
    ASSERT_TRUE(Reader::parse("de") != 0);
}

TEST(BencodeCollectionsDecodingTest, HandlesEmptyList)
{
    SUPPRESS_OUTPUT;
    ASSERT_TRUE(Reader::parse("le") != 0);
}
