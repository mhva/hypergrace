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

#ifndef BENCODE_READER_HH_
#define BENCODE_READER_HH_

#include <iostream>
#include <string>

namespace Hypergrace {
namespace Bencode {

class Object;

/**
 * The Reader  class is a  heart of  Bencode layer. The  Reader class
 * provides  a means  to  parse a  raw  data and  convert  it into  a
 * meaningful object or object hierarchy.
 */
class Reader
{
public:
    /**
     * Creates  Bencode  object  or  Bencode  object  hierarchy  from
     * raw Bencode data.
     *
     * The @parse  method may actually not finish  its work  and fail
     * due to  many conditions /  reasons. The most common  origin of
     * failure is malformed input. In case of failure the method will
     * return 0.
     */
     static Object *parse(std::istream &);

    /**
     * Creates  Bencode  object  or  Bencode  object  hierarchy  from
     * raw Bencode data.
     */
    static Object *parse(const std::string &);
};

} /* namespace Bencode */
} /* namespace Hypergrace */

#endif /* BENCODE_READER_HH_ */
