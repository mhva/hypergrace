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

#ifndef DEBUG_OUTPUTFACILITY_HH_
#define DEBUG_OUTPUTFACILITY_HH_

#include <string>

namespace Hypergrace {
namespace Debug {

/**
 * The OutputFacility class provides a common base class for defining
 * custom logger backends.
 *
 * Custom  backend can  be created  by deriving  from this  class and
 * implementing   the  @output()   method.  Once   logger  has   been
 * requested for  an output  it will call  the @output()  method with
 * the only one string argument containing log message.
 */
class OutputFacility
{
public:
    /**
     * Constructs an output facility.
     */
    OutputFacility();

    /**
     * Destroys the output facility.
     */
    virtual ~OutputFacility();

    /**
     * The output() method is called when a logger has been requested
     * to output a log message.
     */
    virtual void output(const std::string &) = 0;
};

} /* namespace Debug */
} /* namespace Hypergrace */

#endif /* DEBUG_OUTPUTFACILITY_HH_ */
