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

#include <string>
#include <util/array.hh>

#ifndef UTIL_SHA1_HH_
#define UTIL_SHA1_HH_

namespace Hypergrace {
namespace Util {

class Sha1Hash
{
public:
    typedef Array<unsigned char, 20> Hash;

public:
    Sha1Hash();
    ~Sha1Hash() = default;

    void update(const char *, size_t);
    void update(const std::string &);
    Hash final();

    static Hash oneshot(const char *, size_t);
    static Hash oneshot(const std::string &);

public:
    struct Context {
        unsigned int H[5];
        unsigned int W[16];
        unsigned long long size;
    };

private:
    Context context;
};

} /* namespace Util */
} /* namespace Hypergrace */

#endif /* UTIL_SHA1_HH_ */
