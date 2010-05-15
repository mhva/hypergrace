/*
   Copyright (C) 2010 Anton Mihalyov <anton@glyphsense.com>

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

#ifndef BT_IO_DISKIO_HH_
#define BT_IO_DISKIO_HH_

#include <deque>
#include <tuple>

#include <delegate/delegate.hh>
#include <util/shared.hh>

namespace Hypergrace { namespace Bt { class TorrentBundle; }}


namespace Hypergrace {
namespace Bt {

class DiskIo
{
public:
    typedef std::deque<std::tuple<unsigned int, unsigned int, unsigned int> > ReadList;
    typedef std::deque<std::tuple<unsigned int, unsigned int, std::string> > WriteList;
    typedef Delegate::Delegate<void ()> WriteSuccessDelegate;
    typedef Delegate::Delegate<void ()> WriteFailureDelegate;
    typedef Delegate::Delegate<void (std::string)> ReadSuccessDelegate;
    typedef Delegate::Delegate<void ()> ReadFailureDelegate;
    typedef Delegate::Delegate<void (unsigned int)> VerifySuccessDelegate;
    typedef Delegate::Delegate<void (unsigned int)> VerifyFailureDelegate;

    DiskIo();
    ~DiskIo();

    void writeBlocks(const TorrentBundle &, WriteList &&,
            const WriteSuccessDelegate &, const WriteFailureDelegate &);

    void writeData(const std::string &, unsigned long long, const std::string &,
            const WriteSuccessDelegate &, const WriteFailureDelegate &);

    void readBlocks(const TorrentBundle &, ReadList &&,
            const ReadSuccessDelegate &, const ReadFailureDelegate &);

    void readBlock(const TorrentBundle &, unsigned int, unsigned int, unsigned int,
            const ReadSuccessDelegate &, const ReadFailureDelegate &);

    void verifyPiece(const TorrentBundle &, unsigned int,
            const VerifySuccessDelegate &, const VerifyFailureDelegate &);

    static DiskIo *self();

private:
    HG_DECLARE_PRIVATE
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_IO_DISKIO_HH_ */
