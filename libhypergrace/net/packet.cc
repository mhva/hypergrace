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

#include "packet.hh"

using namespace Hypergrace::Net;


static void noop() {}

Packet::Packet() :
    onSent(&noop),
    packetClass_(0),
    packetId_(0),
    discarded_(false)
{
}

void Packet::discard()
{
    discarded_ = true;
}

void Packet::setPacketClass(int cls)
{
    packetClass_ = cls;
}

void Packet::setPacketId(int id)
{
    packetId_ = id;
}

bool Packet::discarded() const
{
    return discarded_;
}

int Packet::packetClass() const
{
    return packetClass_;
}

int Packet::packetId() const
{
    return packetId_;
}
