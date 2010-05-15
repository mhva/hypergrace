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


TorrentBundle::TorrentBundle(
        const std::string &bundleDir,
        TorrentModel *model,
        TorrentState *state,
        TorrentConfiguration *conf) :
    conf_(conf),
    model_(model),
    state_(state),
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

TorrentConfiguration &TorrentBundle::configuration() const
{
    return *conf_;
}

TorrentModel &TorrentBundle::model() const
{
    return *model_;
}

TorrentState &TorrentBundle::state() const
{
    return *state_;
}

const std::string &TorrentBundle::bundleDirectory() const
{
    return bundleDir_;
}

const char *TorrentBundle::configurationFilename()
{
    return "0.hgconf";
}

const char *TorrentBundle::modelFilename()
{
    return "0.hgmodel";
}

const char *TorrentBundle::stateFilename()
{
    return "0.hgstate";
}
