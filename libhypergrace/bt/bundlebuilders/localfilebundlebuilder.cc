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

#include <fstream>
#include <iostream>
#include <sstream>

#include <debug/debug.hh>

#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/bundle/torrentbundle.hh>

#include "localfilebundlebuilder.hh"

using namespace Hypergrace::Bt;


LocalFileBundleBuilder::LocalFileBundleBuilder(const std::string &bundleDir,
                                               const std::string &torrentFile) :
    BundleBuilder(bundleDir),
    torrentFile_(torrentFile)
{
}

void LocalFileBundleBuilder::build()
{
    std::ifstream in(torrentFile_, std::ios_base::binary | std::ios_base::in);

    if (!in.is_open()) {
        hSevere() << "Failed to open the torrent file [" << torrentFile_ << "]";
        onBuildFailed();
        delete this;
        return;
    }

    std::string contents;
    contents.reserve(70 * 1024);
    char buffer[10240];

    while (!in.fail()) {
        in.read(buffer, sizeof(buffer));
        contents.append(buffer, sizeof(buffer));
    }

    if (!in.eof()) {
        hSevere() << "Failed to read the torrent file [" << torrentFile_ << "]";
        onBuildFailed();
        delete this;
        return;
    }

    TorrentModel *model = TorrentModel::fromString(contents);

    if (model == 0) {
        hSevere() << "Failed to parse the torrent file [" << torrentFile_ << "]";
        onBuildFailed();
        delete this;
        return;
    }

    TorrentState *state = new TorrentState(model->pieceCount());
    TorrentConfiguration *conf = new TorrentConfiguration();

    onBundleReady(new TorrentBundle(bundleDir_, model, state, conf));

    // This builder instance is no longer needed.
    delete this;
}
