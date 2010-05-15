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

#include <ios>
#include <limits>
#include <utility>

#include <debug/debug.hh>

#include "object.hh"
#include "trivialobject.hh"
#include "typedefs.hh"

#include "integerworker.hh"

using namespace Hypergrace::Bencode;


bool IntegerWorker::matches(std::istream &stream) const
{
    return stream.peek() == 'i';
}

Object *IntegerWorker::process(std::istream &stream) const
{
    char buffer[21];

    stream.seekg(1, std::ios_base::cur);
    stream.get(buffer, 20, 'e');

    if (!stream.good() || stream.peek() != 'e' || stream.eof())
    {
        if (stream.eof())
            hSevere() << "Unexpected end of stream while parsing an integer";
        else
            hSevere() << "Integer is too large to fit a 64-bit register";
        return 0;
    }

    auto integer = IntegerWorker::extractInteger64(buffer);

    if (integer.second == 0)
    {
        hSevere() << "Integer overflow has occurred while parsing an integer";
        return 0;
    }

    stream.seekg(1, std::ios_base::cur);

    return new BencodeInteger(integer.first);
}

template<typename IntegerType, typename UnsignedIntegerType>
static inline std::pair<IntegerType, int> extractInteger(const char *buffer)
{
    typedef typename std::pair<IntegerType, int> ResultType;

    IntegerType integer = 0;
    bool sign = buffer[0] == '-';
    int length = 0 + sign;

    while (isdigit(buffer[length]))
    {
        integer = integer * 10 + buffer[length] - '0';

        // Since we're adding by small increments it's enough to
        // check if the most significant bit is set to find out
        // whether an integer overflow has occurred.
        if ((~(std::numeric_limits<UnsignedIntegerType>::max() >> 1) & integer) == 0)
            ++length;
        else
            return ResultType(0, 0);
    }

    if (!sign)
        return ResultType(integer, length);
    else
        return (length > 1) ? ResultType(-integer, length) : ResultType(0, 0);
}

std::pair<long long, int> IntegerWorker::extractInteger64(const char *buffer)
{
    return ::extractInteger<long long, unsigned long long>(buffer);
}

std::pair<int, int> IntegerWorker::extractInteger32(const char *buffer)
{
    return ::extractInteger<int, unsigned int>(buffer);
}
