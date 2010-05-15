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

#ifndef UTIL_FILESYSTEM_HH_
#define UTIL_FILESYSTEM_HH_

#include <string>
#include <vector>


namespace Hypergrace {
namespace Util {

class FileSystem
{
public:
    static bool createPath(const std::string &, int);
    static bool createFile(const std::string &, int);
    static bool preallocateFile(const std::string &, long long);

    static long long fileSize(const std::string &);
    static bool fileExists(const std::string &);

    static std::vector<std::string> splitPath(const std::string &);
};

} /* namespace Util */
} /* namespace Hypergrace */

#endif /* UTIL_FILESYSTEM_HH_ */
