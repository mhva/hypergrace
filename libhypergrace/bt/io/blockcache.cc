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

#include <algorithm>
#include <cassert>
#include <tuple>
#include <utility>
#include <vector>

#include <debug/debug.hh>

#include "blockcache.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


BlockCache::BlockCache() :
    load_(0),
    completeCount_(0)
{
}

BlockCache::~BlockCache()
{
}

void BlockCache::reserve(unsigned int piece, unsigned int blockCount)
{
    assert(cache_.find(piece) == cache_.end());

    CacheEntry entry;
    entry.blocksHave = 0;
    entry.blockCount = blockCount;

    cache_.insert(std::make_pair(piece, entry));
}

bool BlockCache::store(unsigned int piece, unsigned int offset, const std::string &data)
{
    auto pieceIt = cache_.find(piece);

    assert(pieceIt != cache_.end());
    assert((*pieceIt).second.blocksHave < (*pieceIt).second.blockCount);

    CacheEntry &entry = (*pieceIt).second;

    entry.blocks.push_back(std::make_pair(offset, data));
    entry.blocksHave += 1;

    load_ += data.size();

    if (entry.blocksHave == entry.blockCount) {
        ++completeCount_;
        return true;
    } else {
        return false;
    }

}

DiskIo::WriteList BlockCache::flushEverything()
{
    DiskIo::WriteList writeList;

    for (auto pieceIt = cache_.begin(); pieceIt != cache_.end(); ++pieceIt) {
        unsigned int piece = (*pieceIt).first;
        BlockList &blocks = (*pieceIt).second.blocks;

        std::transform(
            blocks.begin(), blocks.end(), std::back_inserter(writeList),
            [piece](Block &b) { return std::make_tuple(piece, b.first, b.second); }
        );

        blocks.clear();
    }

    load_ = 0;

    return std::move(writeList);
}

BlockCache::CompletePieceList BlockCache::flushComplete()
{
    CompletePieceList completePieces;
    std::vector<Cache::iterator> eraseList;

    eraseList.reserve(completeCount_);

    auto pieceIt = cache_.begin();
    auto end = cache_.end();
    unsigned int found = 0;

    while (pieceIt != end && found < completeCount_) {
        unsigned int piece = (*pieceIt).first;
        BlockList &blocks = (*pieceIt).second.blocks;

        if ((*pieceIt).second.blocksHave == (*pieceIt).second.blockCount) {
            completePieces.resize(completePieces.size() + 1);

            completePieces.back().first = piece;

            for (auto blockIt = blocks.begin(); blockIt != blocks.end(); ++blockIt) {
                completePieces.back().second.push_back(
                        std::make_tuple(piece, (*blockIt).first, (*blockIt).second));

                load_ -= (*blockIt).second.size();
            }

            eraseList.push_back(pieceIt);
            ++found;
        }

        ++pieceIt;
    }

    std::for_each(eraseList.begin(), eraseList.end(),
            [this](Cache::iterator &it) { cache_.erase(it); });

    completeCount_ = 0;

    return std::move(completePieces);
}

unsigned int BlockCache::load() const
{
    return load_;
}

unsigned int BlockCache::storedPieceCount() const
{
    return cache_.size();
}

unsigned int BlockCache::completePieceCount() const
{
    return completeCount_;
}
