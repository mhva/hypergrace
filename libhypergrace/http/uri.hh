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

#ifndef HTTP_URL_HH_
#define HTTP_URL_HH_

#include <map>
#include <string>

namespace Hypergrace {
namespace Http {

class Uri
{
public:
    explicit Uri(const std::string &);
    ~Uri();

    bool valid();

    Uri &setParameter(const std::string &, const std::string &);
    Uri &setParameter(const std::string &, int);
    Uri &setParameter(const std::string &, long long);
    Uri &setParameter(const std::string &, unsigned int);
    Uri &setParameter(const std::string &, unsigned long long);

    const std::string &host() const;
    const std::string &port() const;
    const std::string &scheme() const;

    std::string absoluteUri() const;
    std::string absolutePath() const;

    static std::string escape(const std::string &);

private:
    std::string stuffParameters() const;

private:
    std::string host_;
    std::string port_;
    std::string scheme_;
    std::string path_;
    std::map<const std::string, std::string> parameterMap_;
};

} /* namespace Http */
} /* namespace Hypergrace */

#endif /* HTTP_URL_HH_ */
