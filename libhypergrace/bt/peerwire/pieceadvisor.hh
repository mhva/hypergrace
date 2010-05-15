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

#ifndef BT_IO_PIECEADVISOR_HH_
#define BT_IO_PIECEADVISOR_HH_

#include <vector>
#include <util/bitfield.hh>


namespace Hypergrace {
namespace Bt {

class PieceAdvisor
{
public:
    explicit PieceAdvisor(const TorrentBundle &);

    void reference(unsigned int);
    void reference(const Util::Bitfield &);
    void unreference(unsigned int);
    void unreference(const Util::Bitfield &);

    void markDirty(unsigned int);
    void markClean(unsigned int);

    bool isDirty(unsigned int) const;

    unsigned int recommend();

private:
    void refillCache();

private:
    const TorrentBundle &bundle_;

    std::vector<unsigned int> cache_;
    std::vector<unsigned int> references_;
    Util::Bitfield dirtyPieces_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_IO_PIECEADVISOR_HH_ */
