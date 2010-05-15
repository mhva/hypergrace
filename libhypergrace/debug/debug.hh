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

#ifndef DEBUG_DEBUG_HH_
#define DEBUG_DEBUG_HH_

#include <string>
#include <util/shared.hh>

// TODO: No matter what macro is used a user will see the same output.
// Implement different logging levels.
#define hDebug() Hypergrace::Debug::Debug()
#define hInfo() Hypergrace::Debug::Debug()
#define hWarning() Hypergrace::Debug::Debug()
#define hSevere() Hypergrace::Debug::Debug()

#define hcDebug(ctx) Hypergrace::Debug::Debug()
#define hcInfo(ctx) Hypergrace::Debug::Debug()
#define hcWarning(ctx) Hypergrace::Debug::Debug()
#define hcSevere(ctx) Hypergrace::Debug::Debug()

namespace Hypergrace {
namespace Debug {

class OutputFacility;

/**
 * Simple logging implementation.
 *
 * The Debug  class uses  a backend, called  facility, to  stream log
 * messages. New facility  can be created by deriving  a custom class
 * from @Hypergrace::OutputFacility       and       then      calling
 * the   @Hypergrace::Debug::setOutputFacility()  static   method  to
 * replace current logging facility with the new one.
 */
class Debug
{
public:
    enum WhitespacePolicy
    {
        NoAutoWhitespace = 0,
        AutoWhitespace = 1
    };

public:
    /**
     * Creates a debug stream.
     *
     * By  default debug  stream automatically  inserts a  whitespace
     * after each output  operation. If this behavior  is not desired
     * you     can     turn     it     off     by     passing     the
     * Hypergrace::Debug::NoAutoWhitespace policy to the constructor.
     *
     * Alternatively,  this  behavior  can  be  changed  by  using  a
     * streaming         operator        and         passing      the
     * Hypergrace::Debug::NoAutoWhitespace  policy  as  an  argument,
     * i.e.:
     *
     *     Debug() << "Property:"
     *             << Hypergrace::Debug::NoAutoWhitespace
     *             << '[' << true << ']';
     *
     * As a result you would see:
     *     Property: [true]
     *
     * To   re-enable   auto-whitespace   you    need   to   pass the
     * Hypergrace::Debug::AutoWhitespace  policy to one of the  above
     * methods.
     */
    explicit Debug(WhitespacePolicy whitespacePolicy=AutoWhitespace);
    Debug(const Debug &) = delete;
    ~Debug();

    /**
     * Sets a new output facility and returns an old one.
     *
     * Releasing  memory  acquired by the old  facility must  be done
     * manually.
     */
    static OutputFacility *setOutputFacility(OutputFacility *);

public:
    Debug &operator<< (bool);
    Debug &operator<< (short);
    Debug &operator<< (unsigned short);
    Debug &operator<< (int);
    Debug &operator<< (unsigned int);
    Debug &operator<< (long);
    Debug &operator<< (long long);
    Debug &operator<< (unsigned long);
    Debug &operator<< (unsigned long long);
    Debug &operator<< (float);
    Debug &operator<< (double);
    Debug &operator<< (long double);
    Debug &operator<< (const char *);
    Debug &operator<< (const void *);

    Debug &operator<< (const std::string &);
    Debug &operator<< (WhitespacePolicy);

    void operator =(const Debug &) = delete;

private:
    HG_DECLARE_PRIVATE;
};

} /* namespace Debug */
} /* namespace Hypergrace */

#endif /* DEBUG_DEBUG_HH_ */
