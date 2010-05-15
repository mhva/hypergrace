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

#include <string.h>

#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentmodel.hh>

#include "peerdata.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


PeerData::PeerData(const TorrentBundle &bundle, Net::Socket &socket, const PeerId &peerId) :
    socket_(socket),
    peerId_(peerId),
    bitfield_(bundle.model().pieceCount()),
    peerChokedUs_(true),
    weChokedPeer_(true),
    peerIsInterested_(false),
    weAreInterested_(false)
{
}

const PeerId &PeerData::peerId() const
{
    return peerId_;
}

const Util::Bitfield &PeerData::bitfield() const
{
    return bitfield_;
}

const Util::Rating &PeerData::rating() const
{
    return rating_;
}

bool PeerData::peerIsInterested() const
{
    return peerIsInterested_;
}

bool PeerData::peerChokedUs() const
{
    return peerChokedUs_;
}

bool PeerData::weAreInterested() const
{
    return weAreInterested_;
}

bool PeerData::weChokedPeer() const
{
    return weChokedPeer_;
}

Net::Socket &PeerData::socket() const
{
    return socket_;
}

Util::Bitfield &PeerData::bitfield()
{
    return bitfield_;
}

Util::Rating &PeerData::rating()
{
    return rating_;
}

void PeerData::setPeerIsInterested(bool interested)
{
    peerIsInterested_ = interested;
}

void PeerData::setPeerChokedUs(bool choked)
{
    peerChokedUs_ = choked;
}

void PeerData::setWeAreInterested(bool interested)
{
    weAreInterested_ = interested;
}

void PeerData::setWeChokedPeer(bool choked)
{
    weChokedPeer_ = choked;
}
