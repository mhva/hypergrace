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

#include <ios>
#include <iostream>

#include <debug/debug.hh>

#include "object.hh"
#include "typedefs.hh"
#include "trivialobject.hh"
#include "releasememoryvisitor.hh"

#include "integerworker.hh"
#include "stringworker.hh"

using namespace Hypergrace;
using namespace Bencode;


bool StringWorker::matches(std::istream &stream) const
{
    int c = stream.peek();
    return c > '0' && c <= '9';
}

Object *StringWorker::process(std::istream &stream) const
{
    char prelude[11];

    stream.get(prelude, sizeof(prelude) - 1, ':');
    const std::streamsize read = stream.gcount();

    if (!stream.good() || stream.peek() != ':' || stream.eof())
    {
        if (stream.eof())
            hSevere() << "Unexpected end of stream while parsing a string prelude";
        else
            hSevere() << "String is too large";
        return 0;
    }

    stream.seekg(1, std::ios_base::cur);
    const auto length = IntegerWorker::extractInteger32(prelude);

    // Verify that the string prelude didn't contain any characters
    // except decimal numbers
    if (length.second == 0 || length.second != read)
    {
        hSevere() << "String prelude '" << prelude << "' can contain only "
                  << "decimal numbers and must be a 32-bit integer";
        return 0;
    }

    // TODO: Instead of setting a limit here it'd better if it was
    // defined in config.h.

    // Reject huge strings
    if (length.first > 36 * 1024 * 1024) {
        hSevere() << "String is too large";
        return 0;
    }

    Object *object = new BencodeString();
    std::string &string = object->get<String>();
    char buffer[32];
    unsigned int remaining = length.first;

    // Read input stream in small chunks
    while (remaining > sizeof(buffer)) {
        stream.read(buffer, sizeof(buffer));

        if (!stream.eof()) {
            string.append(buffer, sizeof(buffer));
            remaining -= sizeof(buffer);
        } else {
            ReleaseMemoryVisitor v;
            object->accept(v);
            return 0;
        }
    }

    stream.read(buffer, remaining);

    if (!stream.eof()) {
        string.append(buffer, remaining);
        return object;
    } else {
        ReleaseMemoryVisitor v;
        object->accept(v);
        return 0;
    }
}
