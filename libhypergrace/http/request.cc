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

#include <sstream>

#include <debug/debug.hh>

#include "uri.hh"
#include "request.hh"

using namespace Hypergrace::Http;


Request::Request(const Uri &uri) :
    uri_(uri)
{
}

Request::Request(const char *uri) :
    uri_(uri)
{
}

void Request::appendHeader(const char *, const std::string &)
{
}

void Request::setHeader(const char *, const std::string &)
{
}

std::string Request::serialize() const
{
    std::ostringstream result;
    std::string host = uri_.host();

    if (!uri_.port().empty())
        host += ":" + uri_.port();

    // FIXME: Port to curl?
    result << "GET " << uri_.absolutePath() << " HTTP/1.0\r\n"
           << "Host: " << host << "\r\n"
           << "User-Agent: Hypergrace/0.1\r\n"
           << "Content-Length: 0\r\n\r\n";

    return result.str();
}
