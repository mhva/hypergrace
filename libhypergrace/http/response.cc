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

#include <iterator>
#include <map>
#include <sstream>
#include <vector>

#include <debug/debug.hh>

#include "response.hh"

using namespace Hypergrace::Http;


Response::Response(std::string text) :
    state_(Invalid),
    statusCode_(-1),
    contentLength_(-1)
{
    // Verify status line and extract status code
    if (text.size() < 12) {
        state_ = MoreData;
        return;
    }

    if (text.compare(0, 8, "HTTP/1.0") != 0 && text.compare(0, 8, "HTTP/1.1")) {
        state_ = Invalid;
        return;
    }

    statusCode_ = 100 * (text[9] - 0x30) + 10 * (text[10] - 0x30) + (text[11] - 0x30);

    if (statusCode_ < 100 || statusCode_ > 599) {
        state_ = Invalid;
        return;
    }

    size_t offset = text.find("\r\n");

    if (offset == 0) {
        state_ = MoreData;
        return;
    }

    while (offset != std::string::npos && text.compare(offset, 4, "\r\n\r\n") != 0) {
        // Skip "\r\n"
        offset += 2;

        if (text.compare(offset, sizeof("Content-Length:") - 1, "Content-Length:") == 0) {
            contentLength_ = atoi(text.data() + offset + sizeof("Content-Length:") - 1);
        }

        offset = text.find("\r\n", offset);
    }

    // Exit if we didn't find a message body
    if (offset == std::string::npos) {
        state_ = MoreData;
        return;
    }

    // Don't forget to skip the "\r\n\r\n" headers/message-body separator
    offset += 4;
    messageBody_ = text.substr(offset, text.size() - offset);

    if (contentLength_ > 0) {
        if (text.size() - offset == (unsigned int)contentLength_)
            state_ = Complete;
        else
            state_ = MoreData;
    } else {
        state_ = MoreData;
    }
}

Response::State Response::state() const
{
    return state_;
}

int Response::statusCode() const
{
    return statusCode_;
}

std::string Response::messageBody() const
{
    return messageBody_;
}
