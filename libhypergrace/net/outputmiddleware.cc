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
#include "outputmiddleware.hh"

using namespace Hypergrace;
using namespace Net;


OutputMiddleware::OutputMiddleware()
{
}

OutputMiddleware::OutputMiddleware(Pointer delegate) :
    delegate_(delegate)
{
}

OutputMiddleware::~OutputMiddleware()
{
}

void OutputMiddleware::send(Socket &socket, Packet *packet)
{
    delegateSendEvent(socket, packet);
}

void OutputMiddleware::write(Socket &)
{
}

void OutputMiddleware::shutdown(Socket &socket)
{
    delegateShutdownEvent(socket);
}

void OutputMiddleware::delegateSendEvent(Socket &socket, Packet *packet)
{
    if (delegate_)
        delegate_->send(socket, packet);
}

void OutputMiddleware::delegateWriteEvent(Socket &socket)
{
    if (delegate_)
        delegate_->write(socket);
}

void OutputMiddleware::delegateShutdownEvent(Socket &socket)
{
    if (delegate_)
        delegate_->shutdown(socket);
}
