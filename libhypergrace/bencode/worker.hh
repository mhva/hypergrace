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

#ifndef BENCODE_WORKER_HH_
#define BENCODE_WORKER_HH_

#include <iostream>

namespace Hypergrace {
namespace Bencode {

class Object;

/**
 * The  Worker class  is a  base class  for defining  algorithms that
 * parse raw data and convert it into Bencode objects.
 *
 * Both @matches() and @process() methods are guaranteed to be called
 * only once per an object.
 */
class Worker
{
public:
    /**
     * Checks whether  a given  input sequence  of characters  can be
     * handled by this class.
     *
     * If the method returns true it's guaranteed that the @process()
     * method of this class will be called next.
     *
     * Stream pointer must stay unmodified after an unsuccessful call
     * to this method.
     */
    virtual bool matches(std::istream &) const = 0;

    /**
     * Processes  an input  sequence  of characters  and, as  result,
     * returns a Bencode object.
     *
     * The method may  return 0 when it can't process  input.
     */
    virtual Object *process(std::istream &) const = 0;
};

} // namespace Bencode
} // namespace Hypergrace

#endif // BENCODE_WORKER_HH_
