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

#include <string.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <sstream>

#include <debug/debug.hh>

#include "uri.hh"

using namespace Hypergrace::Http;


Uri::Uri(const std::string &baseUri)
{
    // Extract URI scheme
    const char magic[] = "://";
    size_t schemeEnd = baseUri.find(magic);

    {
        if (schemeEnd != std::string::npos)
            scheme_ = baseUri.substr(0, schemeEnd);
        else
            return;
    }

    // Extract host
    size_t hostEnd = std::string::npos;

    {
        size_t hostStart = schemeEnd + sizeof(magic) - 1;
        size_t colonPos = baseUri.find(':', hostStart);
        size_t slashPos = baseUri.find('/', hostStart);

        if (slashPos != std::string::npos) {
            if (colonPos != std::string::npos && colonPos < slashPos)
                hostEnd = colonPos;
            else
                hostEnd = slashPos;

            host_ = baseUri.substr(hostStart, hostEnd - hostStart);
        } else {
            return;
        }
    }

    // Extract port
    size_t portEnd = baseUri.find('/', hostEnd);

    if (baseUri[hostEnd] == ':') {
        port_ = baseUri.substr(hostEnd + 1, portEnd - hostEnd - 1);
    }

    // Extract absolute path
    path_ = baseUri.substr(portEnd);
}

Uri::~Uri()
{
}

bool Uri::valid()
{
    return !scheme_.empty() && !host_.empty() && !path_.empty();
}

template<typename T>
static inline std::string numericToString(T t)
{
    std::ostringstream resultStream;
    resultStream << t;
    return resultStream.str();
}

Uri &Uri::setParameter(const std::string &name, const std::string &value)
{
    parameterMap_[name] = value;
    return *this;
}

Uri &Uri::setParameter(const std::string &name, int value)
{
    parameterMap_[name] = numericToString(value);
    return *this;
}

Uri &Uri::setParameter(const std::string &name, long long value)
{
    parameterMap_[name] = numericToString(value);
    return *this;
}

Uri &Uri::setParameter(const std::string &name, unsigned int value)
{
    parameterMap_[name] = numericToString(value);
    return *this;
}

Uri &Uri::setParameter(const std::string &name, unsigned long long value)
{
    parameterMap_[name] = numericToString(value);
    return *this;
}

const std::string &Uri::host() const
{
    return host_;
}

const std::string &Uri::port() const
{
    return port_;
}

const std::string &Uri::scheme() const
{
    return scheme_;
}

std::string Uri::absoluteUri() const
{
    std::string params = stuffParameters();
    return scheme_ + "://" + host_ + ((!port_.empty()) ? port_ : "") + path_ + params;
}

std::string Uri::absolutePath() const
{
    return path_ + stuffParameters();
}

std::string Uri::stuffParameters() const
{
    if (parameterMap_.empty())
        return std::string();

    std::ostringstream stream;
    stream << "?";

    for (auto it = parameterMap_.begin(); it != parameterMap_.end(); ++it)
        stream << Uri::escape((*it).first) << "=" << Uri::escape((*it).second) << "&";

    std::string result = stream.str();
    result.erase(result.end() - 1, result.end());

    return result;
}

struct EscapeString : public std::binary_function<std::string &, char, void>
{
    void operator ()(std::string &storage, unsigned char c) const
    {
        const char *hexMap = "0123456789ABCDEF";

        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') || (strchr("-._~", c) != 0)) {
            storage.append(1, c);
        } else {
            char encoded[] = { '%', hexMap[c >> 4], hexMap[c & 15] };
            storage.append(encoded, sizeof(encoded));
        }
    }
};

std::string Uri::escape(const std::string &s)
{
    std::string escaped;
    std::for_each(s.begin(), s.end(),
                  std::bind1st(EscapeString(), std::ref(escaped)));
    return escaped;
}
