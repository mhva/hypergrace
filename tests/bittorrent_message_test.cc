#include <gtest/gtest.h>
#include <bt/peerwire/message.hh>

using namespace Hypergrace;
using namespace Hypergrace::Bt;


TEST(BitTorrentMessageTest, TestPieceMessageSize)
{
    PieceMessage pieceMessage(0xEEEEEEEE, 0xFFFFFFFF, std::string(0x4000, 0));

    ASSERT_EQ(0x04 + 0x01 + 0x04 + 0x04 + 0x4000, pieceMessage.serialize().size());
}
