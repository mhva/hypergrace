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

#ifndef NET_DNSRESOLVER_HH_
#define NET_DNSRESOLVER_HH_

#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Hypergrace {
namespace Net {

class HostResolver
{
public:
    typedef Delegate::Delegate<void (sockaddr *, int)> Callback;

    HostResolver();
    ~HostResolver();

    void resolve(const std::string &, Callback) const;

    void setKeepAliveTime(size_t);
    void setMaximumThreads(size_t);

private:
    void resolverThread();

private:
    size_t keepAlive_;
    size_t maxThreads_;
    std::vector<std::string> queue_;
    std::mutex queueLock_;
};

} /* namespace Net */
} /* namespace Hypergrace */

#endif /* NET_DNSRESOLVER_HH_ */
