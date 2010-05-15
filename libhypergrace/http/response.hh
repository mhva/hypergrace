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

#ifndef HTTP_RESPONSE_HH_
#define HTTP_RESPONSE_HH_

#include <string>


namespace Hypergrace {
namespace Http {

class Response
{
public:
    enum State {
        Invalid = 0,
        MoreData,
        Complete
    };

    explicit Response(std::string);

    State state() const;

    int statusCode() const;
    int contentLength() const;
    std::string messageBody() const;

private:
    State state_;

    int statusCode_;
    int contentLength_;
    std::string messageBody_;
};

} /* namespace Http */
} /* namespace Hypergrace */

#endif /* HTTP_RESPONSE_HH_ */
