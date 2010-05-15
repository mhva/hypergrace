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

#ifndef BENCODE_INTEGERWORKER_HH_
#define BENCODE_INTEGERWORKER_HH_

#include <iostream>
#include "worker.hh"

namespace Hypergrace {
namespace Bencode {

/**
 * The StringWorker class  defines a means for  extracting an integer
 * from the beginning of Bencoded input sequence.
 *
 * Read a documentation of the @Worker class for more details.
 */
class IntegerWorker : public Worker
{
public:
    bool matches(std::istream &stream) const;
    Object *process(std::istream &stream) const;

    /**
     * Extracts a 64-bit integer from the @buffer.
     *
     * Returns a  pair containing an  extracted integer as  the first
     * element and a number of processed bytes.
     *
     * If nothing has been extracted or integer overflow has occurred
     * number of processed bytes will be set to 0.
     */
    static std::pair<long long, int> extractInteger64(const char *buffer);

    /**
     * Extracts a 32-bit integer from the @buffer.
     *
     * Returns a  pair containing an  extracted integer as  the first
     * element and a number of processed bytes.
     *
     * If nothing has been extracted or integer overflow has occurred
     * number of processed bytes will be set to 0.
     */
    static std::pair<int, int> extractInteger32(const char *buffer);
};

} // namespace Bencode
} // namespace Hypergrace

#endif // BENCODE_INTEGERWORKER_HH_
