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

#ifndef BT_BUNDLE_TORRENTMODEL_HH_
#define BT_BUNDLE_TORRENTMODEL_HH_

#include <string>

#include <bt/types.hh>
#include <delegate/delegate.hh>
#include <util/sha1hash.hh>

namespace Hypergrace { namespace Bencode { class Object; }}
namespace Hypergrace { namespace Bt { class FileRegistry; }}


namespace Hypergrace {
namespace Bt {

class TorrentModel
{
public:
    TorrentModel(const TorrentModel &) = delete;
    ~TorrentModel();

    const std::string &announceUri() const;
    const AnnounceList &announceList() const;
    std::string comment() const;
    std::string creator() const;
    std::string encoding() const;
    const InfoHash &hash() const;
    const std::string &name() const;

    long long creationDate() const;
    unsigned int pieceCount() const;
    unsigned int pieceSize() const;
    unsigned int lastPieceSize() const;
    Util::Sha1Hash::Hash pieceHash(unsigned int) const;

    unsigned long long torrentSize() const;

    const FileList &fileList() const;

public:
    std::string toString() const;
    static TorrentModel *fromString(const std::string &);

    TorrentModel &operator =(const TorrentModel &) = delete;

private:
    explicit TorrentModel(Bencode::Object *);

    void constructAnnounceList();
    void constructFileList();
    void constructInfoHash();

private:
    Bencode::Object *metadata_;

    InfoHash infoHash_;
    AnnounceList announceList_;

    FileList fileList_;
    unsigned long long torrentSize_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_BUNDLE_TORRENTMODEL_HH_ */
