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

#ifndef BT_BUNDLE_TORRENTBUNDLE_HH_
#define BT_BUNDLE_TORRENTBUNDLE_HH_

#include <string>

namespace Hypergrace { namespace Bt { class TorrentConfiguration; }}
namespace Hypergrace { namespace Bt { class TorrentModel; }}
namespace Hypergrace { namespace Bt { class TorrentState; }}


namespace Hypergrace {
namespace Bt {

class TorrentBundle
{
public:
    TorrentBundle(const std::string &, TorrentModel *, TorrentState *, TorrentConfiguration *);
    ~TorrentBundle();

    bool save() const;

    TorrentModel &model() const;
    TorrentState &state() const;
    TorrentConfiguration &configuration() const;

private:
    TorrentModel *model_;
    TorrentState *state_;
    TorrentConfiguration *conf_;

    std::string bundleDir_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_BUNDLE_TORRENTBUNDLE_HH_ */
