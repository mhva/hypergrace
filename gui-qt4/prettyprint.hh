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

#ifndef PRETTYNUMBER_HH_
#define PRETTYNUMBER_HH_

#include <QString>
#include <utility>


class PrettyPrint
{
public:
    enum Unit {
        Byte = 0,
        Kilobyte,
        Megabyte,
        Gigabyte,
        Terabyte
    };

    static std::pair<double, Unit> represent(unsigned long long);

    static QString printSize(unsigned long long);
    static QString printSpeed(int);
    static QString printUnit(Unit);
    static QString printRemainingTime(int);

private:
    /* data */
};

#endif /* PRETTYNUMBER_HH_ */
