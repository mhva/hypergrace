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

#ifndef BT_BUNDLE_TORRENTCONFIGURATION_HH_
#define BT_BUNDLE_TORRENTCONFIGURATION_HH_

#include <mutex>
#include <set>
#include <string>

#include <bt/types.hh>
#include <delegate/signal.hh>

namespace Hypergrace { namespace Bencode { class Object; }}
namespace Hypergrace { namespace Bt { class CommandTask; }}
namespace Hypergrace { namespace Bt { class GlobalTorrentRegistry; }}


namespace Hypergrace {
namespace Bt {

class TorrentConfiguration
{
public:
    TorrentConfiguration();
    ~TorrentConfiguration();

    void setUnmaskedFiles(std::set<size_t> &&);
    void limitDownloadRate(int);
    void limitUploadRate(int);
    void limitConnections(int);
    void setUploadSlotCount(unsigned int);
    void setStorageDirectory(const std::string &);
    void setPreallocateStorage(bool);

    std::set<size_t> unmaskedFiles() const;
    int downloadRateLimit() const;
    int uploadRateLimit() const;
    int connectionLimit() const;
    unsigned int uploadSlotCount() const;
    const std::string &storageDirectory() const;
    bool preallocateStorage() const;

    static TorrentConfiguration *fromString(const std::string &);
    std::string toString() const;

public: /* signals */
    Delegate::Signal<> onFileMaskChanged;
    Delegate::Signal<> onUploadRateLimitChanged;
    Delegate::Signal<> onDownloadRateLimitChanged;
    Delegate::Signal<> onConnectionLimitChanged;
    Delegate::Signal<> onUploadSlotCountChanged;
    Delegate::Signal<> onStorageDirectoryChanged;

private:
    TorrentConfiguration(Bencode::Object *);

private:
    Bencode::Object *configTree_;

    std::set<size_t> unmaskedFiles_;
    mutable std::recursive_mutex anchor_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_BUNDLE_TORRENTCONFIGURATION_HH_ */
