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

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>

#include <debug/debug.hh>

#include "torrentstate.hh"
#include "torrentmodel.hh"
#include "torrentconfiguration.hh"
#include "torrentbundle.hh"

using namespace Hypergrace::Bt;


TorrentBundle::TorrentBundle(const std::string &bundleDir,
                             TorrentModel *model,
                             TorrentState *state,
                             TorrentConfiguration *conf) :
    model_(model),
    state_(state),
    conf_(conf),
    bundleDir_(bundleDir)
{
    assert(model_ != 0 && state_ != 0 && conf_ != 0);
}

TorrentBundle::~TorrentBundle()
{
    delete conf_;
    delete state_;
    delete model_;
}

template<typename T>
static bool serialize(const std::string &filename, T &object)
{
    // TODO: make a backup if file exists.
    std::ofstream out(filename, std::ios_base::binary | std::ios_base::out);
    std::string contents = object.toString();

    out.write(contents.data(), contents.size());

    if (!out.fail()) {
        return true;
    } else {
        remove(filename.c_str());
        return false;
    }
}

bool TorrentBundle::save() const
{
    // FIXME: If something fails bundle will be left in the inconsistent
    // state.
    std::string modelFilename = bundleDir_ + "/0.hgmodel";
    std::string stateFilename = bundleDir_ + "/0.hgstate";
    std::string confFilename = bundleDir_ + "/0.hgconf";

    if (!serialize(stateFilename, *state_)) {
        hSevere() << "Failed to serialize torrent's state to" << stateFilename;
        return false;
    }

    if (!serialize(confFilename, *conf_)) {
        hSevere() << "Failed to serialize torrent configuration to" << confFilename;
        return false;
    }

    // If model doesn't exist we'll create one and if it does we can
    // skip serializing it.
    std::ifstream modelFile(modelFilename, std::ios_base::in);

    if (!modelFile.is_open()) {
        if (!serialize(modelFilename, *model_)) {
            hSevere() << "Failed to serialize torrent model to" << modelFilename;
            return false;
        }
    } else {
        modelFile.close();
    }

    return true;
}

TorrentModel &TorrentBundle::model() const
{
    return *model_;
}

TorrentState &TorrentBundle::state() const
{
    return *state_;
}

TorrentConfiguration &TorrentBundle::configuration() const
{
    return *conf_;
}
