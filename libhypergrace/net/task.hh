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

#ifndef NET_TASK_HH_
#define NET_TASK_HH_

namespace Hypergrace { namespace Net { class Reactor; }}


namespace Hypergrace {
namespace Net {

class Task
{
public:
    virtual ~Task();

private:
    friend class Reactor;

    virtual void start();
    virtual void execute() = 0;
    virtual void stop();
};

} /* namespace Net */
} /* namespace Bencode */

#endif /* NET_TASK_HH_ */
