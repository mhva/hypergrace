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

#include "object.hh"
#include "releasememoryvisitor.hh"
#include "typedefs.hh"
#include "trivialobject.hh"

#include "workerchain.hh"
#include "dictionaryworker.hh"
#include "integerworker.hh"
#include "stringworker.hh"
#include "listworker.hh"

using namespace Hypergrace;
using namespace Bencode;


bool ListWorker::matches(std::istream &stream) const
{
    return stream.peek() == 'l';
}

Object *ListWorker::process(std::istream &stream) const
{
    Object *listObject = new BencodeList();
    auto &list = listObject->get<List>();

    stream.seekg(1, std::ios_base::cur);

    while (!stream.fail() && stream.peek() != 'e') {
        Object *object = WorkerChain::extractObject(stream,
                            StringWorker(),
                            IntegerWorker(),
                            DictionaryWorker(),
                            ListWorker());

        if (object != 0) {
            list.push_back(object);
        } else {
            ReleaseMemoryVisitor v;
            listObject->accept(v);
            return 0;
        }
    }

    if (!stream.fail() && stream.peek() == 'e') {
        stream.seekg(1, std::ios_base::cur);
        return listObject;
    } else {
        ReleaseMemoryVisitor v;
        listObject->accept(v);
        return 0;
    }
}
