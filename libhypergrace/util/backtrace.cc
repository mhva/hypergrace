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

#include <execinfo.h>
#include <cxxabi.h>
#include <string>

#include <debug/debug.hh>

#include "backtrace.hh"


void Hypergrace::Util::printBacktrace(unsigned int desiredSize)
{
    void *buffer[desiredSize];
    size_t size = backtrace(buffer, desiredSize);
    char **strings = backtrace_symbols(buffer, size);;

    for (size_t i = 1; i < size; ++i) {
        std::string specifier = strings[i];
        size_t parenthesisStart = specifier.find("(");
        size_t plusSignStart = specifier.find("+", parenthesisStart);

        std::string mangledName;

        if (parenthesisStart != std::string::npos && plusSignStart != std::string::npos) {
            mangledName = specifier.substr(parenthesisStart + 1,
                    plusSignStart - parenthesisStart - 1);
        } else {
            hDebug() << i - 1 << "--" << specifier;
            continue;
        }

        int status;
        char *demangledName = abi::__cxa_demangle(mangledName.data(), 0, 0, &status);

        if (status == 0) {
            hDebug() << i - 1 << "--" << demangledName;
            free(demangledName);
        } else {
            hDebug() << i - 1 << "--" << strings[i];
        }
    }

    free(strings);

    hDebug() << "";
}
