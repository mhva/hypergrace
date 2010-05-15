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

#ifndef UTIL_BITFIELD_HH_
#define UTIL_BITFIELD_HH_

#include <cstdlib>
#include <string>

namespace Hypergrace {
namespace Util {

class Bitfield
{
public:
    Bitfield(size_t);
    Bitfield(const std::string &, size_t);
    ~Bitfield();

    void set(size_t);
    void unset(size_t);

    void setAll();
    void unsetAll();

    bool assign(const std::string &);
    bool assign(const unsigned char *, size_t);

    size_t bitCount() const;
    size_t byteCount() const;

    size_t enabledCount() const;
    size_t disabledCount() const;

    bool bit(size_t) const;
    unsigned char byte(size_t) const;

    const unsigned char *cstr() const;

private:
    void updateEnabledBitsCounter();

private:
    unsigned char *bytes_;
    size_t bitCount_;
    size_t setCount_;
};

} /* namespace Util */
} /* namespace Hypergrace */

#endif /* UTIL_BITFIELD_HH_ */
