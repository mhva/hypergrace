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

#include <limits.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>

#include <debug/debug.hh>

#include "torrentstate.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


TorrentState::TorrentState(unsigned int pieceCount) :
    availablePieces_(pieceCount),
    scheduledPieces_(pieceCount),
    verifiedPieces_(pieceCount),
    downloaded_(0),
    uploaded_(0),
    downloadRate_(0),
    uploadRate_(0)
{
    availablePieces_.unsetAll();
    scheduledPieces_.setAll();
    verifiedPieces_.unsetAll();
}

TorrentState::~TorrentState()
{
}

unsigned long long TorrentState::downloaded() const
{
    return downloaded_;
}

unsigned long long TorrentState::uploaded() const
{
    return uploaded_;
}

size_t TorrentState::downloadRate() const
{
    return downloadRate_;
}

size_t TorrentState::uploadRate() const
{
    return uploadRate_;
}

const Util::Bitfield &TorrentState::availablePieces() const
{
    return availablePieces_;
}

const Util::Bitfield &TorrentState::scheduledPieces() const
{
    return scheduledPieces_;
}

const Util::Bitfield &TorrentState::verifiedPieces() const
{
    return verifiedPieces_;
}

PeerRegistry &TorrentState::peerRegistry()
{
    return peerRegistry_;
}

const PeerRegistry &TorrentState::peerRegistry() const
{
    return peerRegistry_;
}

TrackerRegistry &TorrentState::trackerRegistry()
{
    return trackerRegistry_;
}

const TrackerRegistry &TorrentState::trackerRegistry() const
{
    return trackerRegistry_;
}

void TorrentState::updateDownloaded(unsigned int increment)
{
    downloaded_ += increment;
}

void TorrentState::updateUploaded(unsigned int increment)
{
    uploaded_ += increment;
}

void TorrentState::setDownloadRate(size_t rate)
{
    downloadRate_ = rate;
}

void TorrentState::setUploadRate(size_t rate)
{
    uploadRate_ = rate;
}

void TorrentState::markPieceAsAvailable(unsigned int piece)
{
    assert(piece < availablePieces_.bitCount());

    availablePieces_.set(piece);
    markPieceAsUninteresting(piece);
}

void TorrentState::markPieceAsUnavailable(unsigned int piece)
{
    assert(piece < availablePieces_.bitCount());

    availablePieces_.unset(piece);
    markPieceAsInteresting(piece);
}

void TorrentState::markPieceAsInteresting(unsigned int piece)
{
    assert(piece < availablePieces_.bitCount());

    if (!availablePieces_.bit(piece))
        scheduledPieces_.set(piece);
}

void TorrentState::markPieceAsUninteresting(unsigned int piece)
{
    assert(piece < availablePieces_.bitCount());

    scheduledPieces_.unset(piece);
}

void TorrentState::markPieceAsVerified(unsigned int piece)
{
    assert(piece < verifiedPieces_.bitCount());

    verifiedPieces_.set(piece);
}

std::string TorrentState::toString() const
{
    std::ostringstream out(std::ios_base::binary | std::ios_base::out);

    unsigned int pieceCount = availablePieces_.bitCount();

    // TODO: Store integers in the big-endian order.
    out.write((char *)&downloaded_, sizeof(downloaded_));
    out.write((char *)&uploaded_, sizeof(uploaded_));
    out.write((char *)&pieceCount, sizeof(pieceCount));
    out.write((char *)availablePieces_.cstr(), availablePieces_.byteCount());

    return out.str();
}

TorrentState *TorrentState::fromString(const std::string &string)
{
    unsigned long long downloaded = 0;
    unsigned long long uploaded = 0;
    uint32_t pieceCount = 0;

    std::istringstream in(string, std::ios_base::binary | std::ios_base::in);

    in.read((char *)&downloaded, sizeof(downloaded));
    in.read((char *)&uploaded, sizeof(uploaded));
    in.read((char *)&pieceCount, sizeof(pieceCount));

    if (in.fail())
        return 0;

    size_t bufferSize = pieceCount / CHAR_BIT + (pieceCount % CHAR_BIT != 0);
    unsigned char pieceBuffer[bufferSize];

    in.read((char *)pieceBuffer, bufferSize);

    TorrentState *torrentState = new TorrentState(pieceCount);

    if (!torrentState->availablePieces_.assign(pieceBuffer, bufferSize)) {
        delete torrentState;
        return 0;
    }

    for (size_t piece = 0; piece < pieceCount; ++piece) {
        if (!torrentState->availablePieces_.bit(piece)) {
            torrentState->scheduledPieces_.set(piece);
        } else {
            torrentState->scheduledPieces_.unset(piece);
        }
    }

    torrentState->downloaded_ = downloaded;
    torrentState->uploaded_ = uploaded;

    return torrentState;
}
