/*
   Copyright (C) 2009 Anton Mihalyov <anton@glyphsense.com>

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

#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>

#include <util/bitfield.hh>

#include "pieceadvisor.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


PieceAdvisor::PieceAdvisor(const TorrentBundle &bundle) :
    bundle_(bundle),
    dirtyPieces_(bundle.model().pieceCount())
{
    references_.resize(bundle.model().pieceCount(), 0);
}

void PieceAdvisor::reference(unsigned int piece)
{
    assert(piece < references_.size());

    ++references_[piece];
}

void PieceAdvisor::reference(const Util::Bitfield &mask)
{
    assert(mask.bitCount() == references_.size());

    for (size_t piece = 0; piece < mask.bitCount(); ++piece) {
        if (mask.bit(piece))
            ++references_[piece];
    }
}

void PieceAdvisor::unreference(unsigned int piece)
{
    assert(piece < references_.size());
    assert(references_[piece] > 0);

    --references_[piece];
}

void PieceAdvisor::unreference(const Util::Bitfield &mask)
{
    assert(mask.bitCount() == references_.size());

    for (size_t piece = 0; piece < mask.bitCount(); ++piece) {
        if (mask.bit(piece)) {
            assert(references_[piece] > 0);
            --references_[piece];
        }
    }
}

void PieceAdvisor::markDirty(unsigned int piece)
{
    assert(piece < dirtyPieces_.bitCount());

    dirtyPieces_.set(piece);
}

void PieceAdvisor::markClean(unsigned int piece)
{
    assert(piece < dirtyPieces_.bitCount());

    dirtyPieces_.unset(piece);
}

bool PieceAdvisor::isDirty(unsigned int piece) const
{
    assert(piece < dirtyPieces_.bitCount());

    return dirtyPieces_.bit(piece);
}

unsigned int PieceAdvisor::recommend()
{
    if (cache_.empty())
        refillCache();

    // Fail with an invalid index if last refill didn't succeed
    if (cache_.empty())
        return -1;

    unsigned int piece = cache_.back();

    cache_.pop_back();

    // Check whether the piece is still downloadable and if not
    // recursively try to recommend something better
    if (bundle_.state().scheduledPieces().bit(piece) &&
        references_[piece] > 0 &&
        !dirtyPieces_.bit(piece))
    {
        dirtyPieces_.set(piece);
        return piece;
    } else {
        return recommend();
    }
}

void PieceAdvisor::refillCache()
{
    std::vector<unsigned int> downloadablePieces;

    const Util::Bitfield& schedPieces = bundle_.state().scheduledPieces();

    for (unsigned int i = 0; i < references_.size(); ++i) {
        // Insert index only if it's valid and there's at least one peer
        // that can serve the piece
        if (references_[i] > 0 && schedPieces.bit(i) && !dirtyPieces_.bit(i))
            downloadablePieces.push_back(i);
    }

    if (downloadablePieces.size() == 0)
        return;

    if (downloadablePieces.size() > 10) {
        cache_.clear();
        cache_.resize(10);

        std::partial_sort_copy(
                downloadablePieces.begin(), downloadablePieces.end(),
                cache_.begin(), cache_.end(),
                [this](size_t l, size_t r) { return references_[l] < references_[r]; }
        );
    } else {
        cache_ = std::move(downloadablePieces);
    }

    std::random_shuffle(cache_.begin(), cache_.end());
}
