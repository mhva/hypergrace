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

#include "inputmiddleware.hh"

using namespace Hypergrace::Bt;


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

void InputMiddleware::processMessage(Net::Socket &c, ChokeMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::processMessage(Net::Socket &c, UnchokeMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::processMessage(Net::Socket &c, InterestedMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::processMessage(Net::Socket &c, NotInterestedMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::processMessage(Net::Socket &c, HaveMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::processMessage(Net::Socket &c, BitfieldMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::processMessage(Net::Socket &c, RequestMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::processMessage(Net::Socket &c, PieceMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::processMessage(Net::Socket &c, CancelMessage &p)
{
    delegateMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, ChokeMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, UnchokeMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, InterestedMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, NotInterestedMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, HaveMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, BitfieldMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, RequestMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, PieceMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}

void InputMiddleware::delegateMessage(Net::Socket &c, CancelMessage &p)
{
    if (delegate_)
        delegate_->processMessage(c, p);
}
