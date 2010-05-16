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

#include <fstream>
#include <iostream>

#include <debug/debug.hh>

#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>

#include "bundleunmarshaller.hh"

using namespace Hypergrace::Bt;


BundleUnmarshaller::BundleUnmarshaller(const std::string &bundleDirectory) :
    BundleBuilder(bundleDirectory)
{
}

BundleUnmarshaller::~BundleUnmarshaller()
{
}

bool BundleUnmarshaller::read(const std::string &filename, std::string &buffer)
{
    std::ifstream in(filename, std::ios_base::binary | std::ios_base::in);

    if (!in.is_open())
        return false;

    in.seekg(0, std::ios::end);

    size_t size = in.tellg();

    in.seekg(0, std::ios::beg);

    char tmp[size];

    in.read(tmp, size);

    if (!in.bad() && in.eof()) {
        buffer.assign(tmp, size);
        return true;
    } else {
        return false;
    }
}

void BundleUnmarshaller::build()
{
    std::string configurationBuffer;
    std::string modelBuffer;
    std::string stateBuffer;

    std::string confFile = bundleDir_ + "/" + TorrentBundle::configurationFilename();
    std::string modelFile = bundleDir_ + "/" + TorrentBundle::modelFilename();
    std::string stateFile = bundleDir_ + "/" + TorrentBundle::stateFilename();

    if (!read(confFile, configurationBuffer)) {
        hSevere() << "Failed to read torrent's configuration from file" << confFile;
        onBuildFailed();
        delete this;
        return;
    }

    if (!read(modelFile, modelBuffer)) {
        hSevere() << "Failed to read torrent's model from file" << modelFile;
        onBuildFailed();
        delete this;
        return;
    }

    if (!read(stateFile, stateBuffer)) {
        hSevere() << "Failed to read torrent's state from file" << stateFile;
        onBuildFailed();
        delete this;
        return;
    }

    TorrentConfiguration *conf = TorrentConfiguration::fromString(configurationBuffer);

    if (conf == 0) {
        hSevere() << "Failed to restore torrent's configuration from file"
                  << bundleDir_ + "0.hgconf," << "the data might be corrupted";
        onBuildFailed();
        delete this;
        return;
    }

    TorrentModel *model = TorrentModel::fromString(modelBuffer);

    if (model == 0) {
        hSevere() << "Failed to restore torrent's model from file"
                  << bundleDir_ + "0.hgmodel," << "the data might be corrupted";
        onBuildFailed();
        delete this;
        return;
    }

    TorrentState *state = TorrentState::fromString(stateBuffer);

    if (state == 0) {
        hSevere() << "Failed to restore torrent's state from file"
                  << bundleDir_ + "0.hgstate," << "the data might be corrupted";
        onBuildFailed();
        delete this;
        return;
    }

    onBundleReady(new TorrentBundle(bundleDir_, model, state, conf));

    delete this;
}
