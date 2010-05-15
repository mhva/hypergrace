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

#include <limits.h>
#include <string.h>

#include <cassert>

#include "bitfield.hh"

using namespace Hypergrace::Util;


Bitfield::Bitfield(size_t bitCount) :
    bytes_(new unsigned char[bitCount / CHAR_BIT + (bitCount % CHAR_BIT != 0)]),
    bitCount_(bitCount),
    setCount_(0)
{
    unsetAll();
}

Bitfield::Bitfield(const std::string &bytes, size_t bitCount) :
    bytes_(0),
    bitCount_(bitCount),
    setCount_(0)
{
    size_t byteCount = bitCount / CHAR_BIT + (bitCount % CHAR_BIT != 0);
    assert(bytes.size() >= byteCount);

    bytes_ = new unsigned char[byteCount];
    memcpy(bytes_, bytes.data(), byteCount);

    updateEnabledBitsCounter();
}

Bitfield::~Bitfield()
{
    delete[] bytes_;
}

void Bitfield::set(size_t bitIndex)
{
    assert(bitIndex < bitCount_);

    if (!bit(bitIndex)) {
        bytes_[bitIndex / CHAR_BIT] |= 0x80 >> (bitIndex % CHAR_BIT);

        assert(setCount_ < bitCount_);
        ++setCount_;
    }
}

void Bitfield::unset(size_t bitIndex)
{
    assert(bitIndex < bitCount_);

    if (bit(bitIndex)) {
        bytes_[bitIndex / CHAR_BIT] &= ~(0x80 >> (bitIndex % CHAR_BIT));

        assert(setCount_ > 0);
        --setCount_;
    }
}

void Bitfield::setAll()
{
    memset(bytes_, 0xFF, byteCount());
    setCount_ = bitCount_;
}

void Bitfield::unsetAll()
{
    memset(bytes_, 0, byteCount());
    setCount_ = 0;
}

bool Bitfield::assign(const std::string &bytes)
{
    return assign((unsigned char *)bytes.data(), bytes.size());
}

bool Bitfield::assign(const unsigned char *bytes, size_t size)
{
    if (byteCount() == size) {
        unsigned char *newBytes = new unsigned char[size];

        memcpy(newBytes, bytes, size);
        delete[] bytes_;

        bytes_ = newBytes;
        updateEnabledBitsCounter();

        return true;
    } else {
        return false;
    }
}

size_t Bitfield::bitCount() const
{
    return bitCount_;
}

size_t Bitfield::byteCount() const
{
    return bitCount_ / CHAR_BIT + (bitCount_ % CHAR_BIT != 0);
}

size_t Bitfield::enabledCount() const
{
    return setCount_;
}

size_t Bitfield::disabledCount() const
{
    return bitCount_ - setCount_;
}

bool Bitfield::bit(size_t bitIndex) const
{
    assert(bitIndex < bitCount_);
    return bytes_[bitIndex / CHAR_BIT] & (0x80 >> (bitIndex % CHAR_BIT));
}

unsigned char Bitfield::byte(size_t byteIndex) const
{
    assert(byteIndex < byteCount());
    return bytes_[byteIndex];
}

const unsigned char *Bitfield::cstr() const
{
    return bytes_;
}

void Bitfield::updateEnabledBitsCounter()
{
    setCount_ = 0;

    for (size_t i = 0; i < bitCount_; ++i)
        setCount_ += bit(i);
}
