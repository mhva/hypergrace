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
#include <typeinfo>
#include <utility>

#include "any.hh"
#include "object.hh"
#include "releasememoryvisitor.hh"
#include "typedefs.hh"
#include "trivialobject.hh"

#include "workerchain.hh"
#include "integerworker.hh"
#include "listworker.hh"
#include "stringworker.hh"
#include "dictionaryworker.hh"

using namespace Hypergrace;
using namespace Bencode;


bool DictionaryWorker::matches(std::istream &stream) const
{
    return stream.peek() == 'd';
}

void releaseObject(Object *object)
{
    if (object == 0)
        return;

    ReleaseMemoryVisitor v;
    object->accept(v);
}

Object *DictionaryWorker::process(std::istream &stream) const
{
    Object *dictObject = new BencodeDictionary();
    Object *keyObject = 0;
    auto &map = dictObject->get<Dictionary>();

    stream.seekg(1, std::ios_base::cur);

    while (!stream.fail() && stream.peek() != 'e') {
        if (keyObject == 0) {
            // We've found a new dictionary entry and need to extract
            // a key
            keyObject = WorkerChain::extractObject(stream, StringWorker());

            if (keyObject == 0) {
                releaseObject(dictObject);
                return 0;
            }
        } else {
            // We already have a dictionary key and need to extract
            // the value associated with the key
            Object *object = WorkerChain::extractObject(stream,
                                StringWorker(),
                                IntegerWorker(),
                                DictionaryWorker(),
                                ListWorker());

            if (object != 0) {
                map[keyObject->get<String>()] = object;

                releaseObject(keyObject);
                keyObject = 0;
            } else {
                releaseObject(dictObject);
                releaseObject(keyObject);
                return 0;
            }
        }
    }

    if (!stream.fail() && stream.peek() == 'e' && keyObject == 0) {
        stream.seekg(1, std::ios_base::cur);
        return dictObject;
    } else {
        releaseObject(dictObject);
        return 0;
    }
}
