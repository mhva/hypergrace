#include <gtest/gtest.h>

#include <debug/debug.hh>
#include <http/uri.hh>

using namespace Hypergrace;
using namespace Http;

TEST(UriTest, TestValidURIs)
{
    ASSERT_TRUE(Uri("http://www.example.com/").valid());
    ASSERT_TRUE(Uri("http://www.example.com:80/").valid());
    ASSERT_TRUE(Uri("http://www.example.com/announce.php").valid());
    ASSERT_TRUE(Uri("http://www.example.com:80/announce.php").valid());
    ASSERT_TRUE(Uri("http://192.168.1.10/").valid());
    ASSERT_TRUE(Uri("http://192.168.1.10:80/").valid());
    ASSERT_TRUE(Uri("http://192.168.1.10/announce.php").valid());
    ASSERT_TRUE(Uri("http://192.168.1.10:80/announce.php").valid());
}

TEST(UriTest, TestInvalidURIs)
{
    ASSERT_FALSE(Uri("http://www.example").valid());
    ASSERT_FALSE(Uri("http://www.example:80").valid());
    ASSERT_FALSE(Uri("http://192.168.1.10").valid());
    ASSERT_FALSE(Uri("http://192.168.1.10:80").valid());
    ASSERT_FALSE(Uri("www.example.com").valid());
    ASSERT_FALSE(Uri("www.example.com/").valid());
    ASSERT_FALSE(Uri("://www.example.com").valid());
    ASSERT_FALSE(Uri("://www.example.com/").valid());
    ASSERT_FALSE(Uri("http:///").valid());
    ASSERT_FALSE(Uri("http://:80/").valid());
    ASSERT_FALSE(Uri("http:///announce.php").valid());
}

TEST(UriTest, TestGettingUriScheme)
{
    ASSERT_EQ("http", Uri("http://www.example.com/").scheme());
    ASSERT_EQ("udp", Uri("udp://www.example.com/").scheme());
}

TEST(UriTest, TestGettingHost)
{
    ASSERT_EQ("www.example.com", Uri("http://www.example.com/").host());
    ASSERT_EQ("www.example.com", Uri("http://www.example.com:80/").host());
    ASSERT_EQ("192.168.1.10", Uri("http://192.168.1.10/").host());
    ASSERT_EQ("192.168.1.10", Uri("http://192.168.1.10:80/").host());
}

TEST(UriTest, TestGettingPortNumber)
{
    ASSERT_EQ("80", Uri("http://www.example.com:80/").port());
    ASSERT_EQ("10000", Uri("http://192.168.1.10:10000/").port());
}

TEST(UriTest, TestGettingAbsoluteUri)
{
    ASSERT_EQ("http://www.example.com/announce.php",
              Uri("http://www.example.com/announce.php").absoluteUri());
}

TEST(UriTest, TestGettingAbsolutePath)
{
    ASSERT_EQ("/", Uri("http://www.example.com:80/").absolutePath());
    ASSERT_EQ("/", Uri("http://192.168.1.10/").absolutePath());
    ASSERT_EQ("/announce.php", Uri("http://www.example.com/announce.php").absolutePath());
    ASSERT_EQ("/announce.php", Uri("http://192.168.1.10:80/announce.php").absolutePath());
}

TEST(UriTest, UriStringEscape)
{
    Uri uri("http://192.168.1.10:80/announce.php");
}
