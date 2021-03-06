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

#ifndef BENCODE_STRINGWORKER_HH_
#define BENCODE_STRINGWORKER_HH_

#include <istream>
#include "worker.hh"

namespace Hypergrace {
namespace Bencode {

/**
 * The StringWorker  class defines  a means  for extracting  a string
 * from beginning of Bencoded input sequence.
 *
 * See documentation for @Bencode::Worker class for more details.
 */
class StringWorker : public Worker
{
public:
    bool matches(std::istream &stream) const;
    Object *process(std::istream &stream) const;
};

} /* namespace Bencode */
} /* namespace Hypergrace */

#endif /* BENCODE_STRINGWORKER_H_ */
