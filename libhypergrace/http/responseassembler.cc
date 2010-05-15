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
#include <net/socket.hh>

#include "inputmiddleware.hh"
#include "response.hh"
#include "responseassembler.hh"

using namespace Hypergrace::Http;


ResponseAssembler::ResponseAssembler(Http::InputMiddleware *delegate) :
    httpDelegate_(delegate),
    assembledResponse_(false)
{
}

ResponseAssembler::ResponseAssembler(Http::InputMiddleware::Pointer delegate) :
    httpDelegate_(delegate)
{
}

void ResponseAssembler::receive(Net::Socket &socket, std::string &data)
{
    if (buffer_.empty())
        buffer_ = data;
    else
        buffer_.append(data);

    Response response(buffer_);

    if (response.state() == Response::Complete) {
        httpDelegate_->processResponse(socket, response);
        assembledResponse_ = true;
    }

    // We don't issue error handler here (if there's error) because it
    // will be called from the shutdown() method.
    if (response.state() == Response::Complete || response.state() == Response::Invalid)
        socket.close();
}

void ResponseAssembler::shutdown(Net::Socket &socket)
{
    if (assembledResponse_)
        return;

    Response response(buffer_);

    if (response.state() != Response::Invalid)
        httpDelegate_->processResponse(socket, response);
    else
        httpDelegate_->handleError(socket);
}
