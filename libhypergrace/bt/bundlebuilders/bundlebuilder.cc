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

#include <thread>

#include <bt/bundle/torrentbundle.hh>
#include <debug/debug.hh>
#include <delegate/delegate.hh>

#include "bundlebuilder.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


static void warnSuccess(TorrentBundle *b)
{
    hWarning() << "Bundle will be deleted because no bundle handler has been registered";
    delete b;
}

static void warnFailure()
{
    hWarning() << "Failed to build a bundle";
}

BundleBuilder::BundleBuilder(const std::string &bundleDir) :
    onBundleReady(&warnSuccess),
    onBuildFailed(&warnFailure),
    bundleDir_(bundleDir),
    workerThread_(0)
{
}

BundleBuilder::~BundleBuilder()
{
    try {
        workerThread_->detach();
        delete workerThread_;
    } catch (...) {
        hSevere() << "Failed to detach a thread causing a little memory leak";
    }
}

void BundleBuilder::startBuilding()
{
    workerThread_ = new std::thread(Delegate::make(this, &BundleBuilder::build));
}
