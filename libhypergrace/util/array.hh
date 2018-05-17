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

#ifndef UTIL_ARRAY_HH_
#define UTIL_ARRAY_HH_

#include <array>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>


namespace Hypergrace {
namespace Util {

template<typename Type, typename std::array<Type, 1>::size_type size>
class Array : public std::array<Type, size>
{
    typedef std::array<Type, size> BaseClass;

public:
    using BaseClass::const_iterator;
    using BaseClass::const_reference;
    using BaseClass::const_reverse_iterator;
    using BaseClass::difference_type;
    using BaseClass::iterator;
    using BaseClass::reference;
    using BaseClass::reverse_iterator;
    using BaseClass::size_type;
    using BaseClass::value_type;

    enum { static_size = size };

    Array()
    {
    }

    Array(const std::string &string)
    {
        if (string.size() < size)
            throw std::runtime_error("The length of string is too small to use as initializer");

        memcpy(this->data(), string.data(), size * sizeof(Type));
    }

    bool fromString(const std::string &string)
    {
        return fromString(string.data(), string.size());
    }

    bool fromString(const char *buffer, size_t bufferSize)
    {
        if (bufferSize >= size) {
            memcpy(this->data(), buffer, size * sizeof(Type));
            return true;
        } else {
            return false;
        }
    }

    inline std::string toString() const
    {
        return std::string(reinterpret_cast<const char *>(this->data()), size * sizeof(Type));
    }

    inline operator std::string() const
    {
        return toString();
    }
};

} /* namespace Util */
} /* namespace Hypergrace */

#endif /* UTIL_ARRAY_HH_ */
