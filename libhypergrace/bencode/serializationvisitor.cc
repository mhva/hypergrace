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

#include "trivialobject.hh"
#include "serializationvisitor.hh"

using namespace Hypergrace;
using namespace Bencode;


SerializationVisitor::SerializationVisitor(std::ostream &receiver) :
    receiver_(receiver)
{
}

void SerializationVisitor::visit(BencodeDictionary *dict)
{
    auto &d = dict->get<Dictionary>();
    auto end = d.end();

    receiver_ << 'd';

    for (auto it = d.begin(); it != end; ++it) {
        receiver_ << (*it).first.size() << ':' << (*it).first;
        (*it).second->accept(*this);
    }

    receiver_ << 'e';
}

void SerializationVisitor::visit(BencodeInteger *integer)
{
    receiver_ << 'i' << integer->get<Integer>() << 'e';
}

void SerializationVisitor::visit(BencodeList *list)
{
    auto &l = list->get<List>();
    auto end = l.end();

    receiver_ << 'l';

    for (auto it = l.begin(); it != end; ++it)
        (*it)->accept(*this);

    receiver_ << 'e';
}

void SerializationVisitor::visit(BencodeString *string)
{
    auto &s = string->get<String>();
    receiver_ << s.size() << ':' << s;
}
