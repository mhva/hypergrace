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

#ifndef BT_PEERWIRE_INPUTMIDDLEWARE_HH_
#define BT_PEERWIRE_INPUTMIDDLEWARE_HH_

#include <memory>
#include <bt/peerwire/message.hh>

namespace Hypergrace { namespace Bt { class DataCollector; }}
namespace Hypergrace { namespace Net { class Socket; }}


namespace Hypergrace {
namespace Bt {

class InputMiddleware
{
public:
    typedef std::shared_ptr<InputMiddleware> Pointer;

    InputMiddleware(Pointer);
    virtual ~InputMiddleware();

    virtual void processMessage(Net::Socket &, ChokeMessage &);
    virtual void processMessage(Net::Socket &, UnchokeMessage &);
    virtual void processMessage(Net::Socket &, InterestedMessage &);
    virtual void processMessage(Net::Socket &, NotInterestedMessage &);
    virtual void processMessage(Net::Socket &, HaveMessage &);
    virtual void processMessage(Net::Socket &, BitfieldMessage &);
    virtual void processMessage(Net::Socket &, PieceMessage &);
    virtual void processMessage(Net::Socket &, RequestMessage &);
    virtual void processMessage(Net::Socket &, CancelMessage &);

protected:
    void delegateMessage(Net::Socket &, ChokeMessage &);
    void delegateMessage(Net::Socket &, UnchokeMessage &);
    void delegateMessage(Net::Socket &, InterestedMessage &);
    void delegateMessage(Net::Socket &, NotInterestedMessage &);
    void delegateMessage(Net::Socket &, HaveMessage &);
    void delegateMessage(Net::Socket &, BitfieldMessage &);
    void delegateMessage(Net::Socket &, PieceMessage &);
    void delegateMessage(Net::Socket &, RequestMessage &);
    void delegateMessage(Net::Socket &, CancelMessage &);

protected:
    InputMiddleware();

private:
    Pointer delegate_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_INPUTMIDDLEWARE_HH_ */
