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

#include <debug/debug.hh>
#include "inputmiddleware.hh"

using namespace Hypergrace;
using namespace Net;


InputMiddleware::InputMiddleware()
{
}

InputMiddleware::InputMiddleware(Pointer delegate) :
    delegate_(delegate)
{
}

InputMiddleware::~InputMiddleware()
{
}

void InputMiddleware::receive(Socket &socket, std::string &data)
{
    delegateReceiveEvent(socket, data);
}

void InputMiddleware::shutdown(Socket &socket)
{
    delegateShutdownEvent(socket);
}

void InputMiddleware::delegateReceiveEvent(Socket &socket, std::string &data)
{
    if (delegate_) {
        delegate_->receive(socket, data);
    } else {
        hWarning() << "Tried to delegate Receive event but there's no middleware to handle it";
    }
}

void InputMiddleware::delegateShutdownEvent(Socket &socket)
{
    if (delegate_) {
        delegate_->shutdown(socket);
    } else {
        hWarning() << "Tried to delegate Shutdown event but there's no middleware to handle it";
    }
}
