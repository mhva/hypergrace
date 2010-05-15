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

#include "sha1hash.hh"
typedef Hypergrace::Util::Sha1Hash::Context blk_SHA_CTX;
#include "sha1impl.c"

using namespace Hypergrace::Util;


Sha1Hash::Sha1Hash()
{
    blk_SHA1_Init(&context);
}

void Sha1Hash::update(const char *data, size_t size)
{
    blk_SHA1_Update(&context, data, size);
}

void Sha1Hash::update(const std::string &data)
{
    blk_SHA1_Update(&context, data.data(), data.size());
}

Sha1Hash::Hash Sha1Hash::final()
{
    Hash result;

    blk_SHA1_Final(result.data(), &context);
    return result;
}

Sha1Hash::Hash Sha1Hash::oneshot(const char *data, size_t size)
{
    Sha1Hash hash;

    hash.update(data, size);
    return hash.final();
}

Sha1Hash::Hash Sha1Hash::oneshot(const std::string &data)
{
    return oneshot(data.data(), data.size());
}
