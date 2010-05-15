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

#ifndef HTTP_HTTPINPUTMIDDLEWARE_CC_
#define HTTP_HTTPINPUTMIDDLEWARE_CC_

#include <memory>

namespace Hypergrace {
    namespace Net { class Socket; }
    namespace Http { class Response; }
}

namespace Hypergrace {
namespace Http {

class InputMiddleware
{
public:
    typedef std::shared_ptr<InputMiddleware> Pointer;

    virtual ~InputMiddleware();

    virtual void processResponse(Net::Socket &, Response &) = 0;
    virtual void handleError(Net::Socket &);

protected:
    InputMiddleware();
    explicit InputMiddleware(Http::InputMiddleware *);
    explicit InputMiddleware(Pointer);

private:
    Pointer delegate_;
};

} /* namespace Http */
} /* namespace Hypergrace */

#endif /* HTTP_HTTPINPUTMIDDLEWARE_CC_ */
