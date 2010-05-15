/*
   Copyright (C) 2010 Anton Mihalyov <anton@glyphsense.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef BT_IO_BLOCKCACHE_HH_
#define BT_IO_BLOCKCACHE_HH_

#include <deque>
#include <string>
#include <unordered_map>
#include <utility>

#include <bt/io/diskio.hh>


namespace Hypergrace {
namespace Bt {

class BlockCache
{
public:
    typedef std::deque<std::pair<unsigned int, DiskIo::WriteList> > CompletePieceList;

    BlockCache();
    ~BlockCache();

    void reserve(unsigned int, unsigned int);
    bool store(unsigned int, unsigned int, const std::string &);

    DiskIo::WriteList flushEverything();
    CompletePieceList flushComplete();

    unsigned int load() const;

    unsigned int storedPieceCount() const;
    unsigned int completePieceCount() const;

private:
    struct CacheEntry;

    typedef std::pair<unsigned int, std::string> Block;
    typedef std::deque<Block> BlockList;
    typedef std::unordered_map<unsigned int, CacheEntry> Cache;

    struct CacheEntry {
        unsigned int blocksHave;
        unsigned int blockCount;
        BlockList blocks;
    };

    Cache cache_;

    unsigned int load_;
    unsigned int completeCount_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_IO_BLOCKCACHE_HH_ */
