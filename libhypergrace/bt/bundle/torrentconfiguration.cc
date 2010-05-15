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

#include <exception>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <bencode/object.hh>
#include <bencode/path.hh>
#include <bencode/reader.hh>
#include <bencode/releasememoryvisitor.hh>
#include <bencode/serializationvisitor.hh>
#include <bencode/typedefs.hh>
#include <bencode/trivialobject.hh>

#include <bt/peerwire/commandtask.hh>

#include <debug/debug.hh>
#include <util/sha1hash.hh>

#include "torrentconfiguration.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


TorrentConfiguration::TorrentConfiguration() :
    configTree_(new Bencode::BencodeDictionary())
{
    auto &d = configTree_->get<Bencode::Dictionary>();

    d["storage-dir"] = new Bencode::BencodeString();
    d["connection-limit"] = new Bencode::BencodeInteger(60);
    d["dload-rate"] = new Bencode::BencodeInteger(-1);
    d["uload-rate"] = new Bencode::BencodeInteger(-1);
    d["uload-slots"] = new Bencode::BencodeInteger(6);
    d["prealloc-storage"] = new Bencode::BencodeInteger(1);
}

TorrentConfiguration::~TorrentConfiguration()
{
    Bencode::ReleaseMemoryVisitor freemem;
    configTree_->accept(freemem);
}

void TorrentConfiguration::setUnmaskedFiles(std::set<std::string> &&files)
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    fileCache_ = std::move(files);

    onFileMaskChanged(fileCache_);
}

void TorrentConfiguration::limitDownloadRate(int rate)
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    Bencode::Path(configTree_, "dload-rate").resolve<Bencode::Integer>() = rate;
    onDownloadRateLimitChanged(rate);
}

void TorrentConfiguration::limitUploadRate(int rate)
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    Bencode::Path(configTree_, "uload-rate").resolve<Bencode::Integer>() = rate;
    onUploadRateLimitChanged(rate);
}

void TorrentConfiguration::limitConnections(int limit)
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    Bencode::Path(configTree_, "connection-limit").resolve<Bencode::Integer>() = limit;
    onConnectionLimitChanged(limit);
}

void TorrentConfiguration::setUploadSlotCount(unsigned int count)
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    Bencode::Path(configTree_, "uload-slots").resolve<Bencode::Integer>() = count;
    onUploadSlotCountChanged(count);
}

void TorrentConfiguration::setStorageDirectory(const std::string &path)
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    Bencode::Path(configTree_, "storage-dir").resolve<Bencode::String>() = path;
    onStorageDirectoryChanged(path);
}

void TorrentConfiguration::setPreallocateStorage(bool preallocate)
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    Bencode::Path(configTree_, "prealloc-storage").resolve<Bencode::Integer>() = preallocate;
}

std::set<std::string> TorrentConfiguration::unmaskedFiles() const
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    return fileCache_;
}

int TorrentConfiguration::downloadRateLimit() const
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    return Bencode::Path(configTree_, "dload-rate").resolve<Bencode::Integer>();
}

int TorrentConfiguration::uploadRateLimit() const
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    return Bencode::Path(configTree_, "uload-rate").resolve<Bencode::Integer>();
}

int TorrentConfiguration::connectionLimit() const
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    return Bencode::Path(configTree_, "connection-limit").resolve<Bencode::Integer>();
}

unsigned int TorrentConfiguration::uploadSlotCount() const
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    return Bencode::Path(configTree_, "uload-slots").resolve<Bencode::Integer>();
}

const std::string &TorrentConfiguration::storageDirectory() const
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    return Bencode::Path(configTree_, "storage-dir").resolve<Bencode::String>();
}

bool TorrentConfiguration::preallocateStorage() const
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    return Bencode::Path(configTree_, "prealloc-storage").resolve<Bencode::Integer>();
}

std::string TorrentConfiguration::toString()
{
    std::lock_guard<std::recursive_mutex> l(anchor_);

    // Convert the set containing files that we've been downloading
    // into BencodeList and insert the list into the config tree so
    // it can be saved together with the rest of the config tree
    Bencode::Object *filesObject = new Bencode::BencodeList();
    auto &files = filesObject->get<Bencode::List>();

    for (auto file = fileCache_.begin(); file != fileCache_.end(); ++file)
        files.push_back(new Bencode::BencodeString(*file));

    configTree_->get<Bencode::Dictionary>()["sched-files"] = filesObject;

    // Save the config tree to config file
    std::ostringstream out;
    Bencode::SerializationVisitor serialize(out);
    configTree_->accept(serialize);

    // Delete the temporary file list we have created earlier
    configTree_->get<Bencode::Dictionary>().erase("sched-files");

    Bencode::ReleaseMemoryVisitor freemem;
    filesObject->accept(freemem);

    return out.str();
}

TorrentConfiguration *TorrentConfiguration::fromString(const std::string &s)
{
    std::istringstream in(s, std::ios_base::binary | std::ios_base::in);
    Bencode::Object *configTree = 0;

    // Parse the string and ensure that it's a valid format
    try {
        configTree = Bencode::Reader::parse(in);

        if (configTree == 0)
            throw std::runtime_error("Configuration data is damaged");

        // Ensure that mandatory dictionary keys do exist and their
        // values have the right type
        Bencode::Path(configTree, "uload-rate").resolve<Bencode::Integer>();
        Bencode::Path(configTree, "uload-slots").resolve<Bencode::Integer>();
        Bencode::Path(configTree, "dload-rate").resolve<Bencode::Integer>();
        Bencode::Path(configTree, "connection-limit").resolve<Bencode::Integer>();
        Bencode::Path(configTree, "storage-dir").resolve<Bencode::String>();
        Bencode::Path(configTree, "sched-files").resolve<Bencode::List>();
        Bencode::Path(configTree, "prealloc-storage").resolve<Bencode::Integer>();
    } catch (std::exception &e) {
        hDebug() << e.what();
        hSevere() << "Failed to deserialize a torrent configuration from string.";
        return 0;
    }

    TorrentConfiguration *conf = new TorrentConfiguration();

    // Build a file cache
    try {
        auto &files = Bencode::Path(configTree, "sched-files").resolve<Bencode::List>();

        for (auto file = files.begin(); file != files.end(); ++file)
            conf->fileCache_.insert((*file)->get<Bencode::String>());
    } catch (std::exception &e) {
        hDebug() << e.what();
        hSevere() << "Cannot rebuild a file cache because the file list has an invalid "
                     "entries";
        return 0;
    }

    Bencode::ReleaseMemoryVisitor freemem;

    // We don't need sched-files value anymore because all file list
    // is transferred to the cache
    Bencode::Object *filesObject = configTree->get<Bencode::Dictionary>()["sched-files"];
    configTree->get<Bencode::Dictionary>().erase("sched-files");
    filesObject->accept(freemem);

    // Don't forget to free memory for a default configuration tree
    conf->configTree_->accept(freemem);
    conf->configTree_ = configTree;
    return conf;
}
