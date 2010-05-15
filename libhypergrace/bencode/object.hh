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

#ifndef BENCODE_OBJECT_HH_
#define BENCODE_OBJECT_HH_

#include "any.hh"

namespace Hypergrace { namespace Bencode { class ObjectVisitor; } }
namespace Hypergrace {
namespace Bencode {

/**
 * The  Object class  provides a  common  base for  all classes  that
 * implement Bencode data types.
 */
class Object : public Any
{
public:
    /**
     * Accepts a visitor.
     *
     * Each  subclass   must  implement   this  method  to   call  an
     * appropriate method of the visitor.
     */
    virtual void accept(ObjectVisitor &visitor) = 0;

public:
    template<typename T> explicit Object(const T &object) :
        Any(object)
    {
    }
};

} // namespace Bencode
} // namespace Hypergrace

#endif // BENCODE_OBJECT_HH_
