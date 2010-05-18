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

#include <algorithm>
#include <cassert>
#include <exception>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <bencode/object.hh>
#include <bencode/path.hh>
#include <bencode/reader.hh>
#include <bencode/releasememoryvisitor.hh>
#include <bencode/serializationvisitor.hh>
#include <bencode/trivialobject.hh>
#include <bencode/typedefs.hh>

#include <debug/debug.hh>
#include <util/sha1hash.hh>

#include "torrentmodel.hh"

using namespace Hypergrace;
using namespace Bt;


class RelinkAzureusKeys : public Bencode::ObjectVisitor
{
public:
    void visit(Bencode::BencodeDictionary *dict)
    {
        const char utf8[] = { '.', 'u', 't', 'f', '-', '8' };
        auto &d = dict->get<Bencode::Dictionary>();

        std::vector<std::string> azureusKeys;

        // Extract all azureus keys from the dictionary.
        for (auto it = d.begin(); it != d.end(); ++it) {
            std::string key = (*it).first;

            // But before doing anything visit the value first.
            (*it).second->accept(*this);

            if (key.size() < sizeof(utf8))
                continue;

            if (key.compare(key.size() - sizeof(utf8), sizeof(utf8), utf8, sizeof(utf8)) == 0)
                azureusKeys.push_back(key);
        }

        std::vector<std::string> keysToErase;

        for (auto it = azureusKeys.begin(); it != azureusKeys.end(); ++it) {
            std::string azureusKey = *it;
            std::string siblingKey = azureusKey.substr(0, azureusKey.size() - sizeof(utf8));

            auto azureusKeyIt = d.find(azureusKey);
            auto siblingKeyIt = d.find(siblingKey);

            if (siblingKeyIt == d.end())
                continue;

            // Ensure that both keys are associated with the String
            // values. Exception will be thrown if one of the keys has
            // a non-string value.
            try {
                (*azureusKeyIt).second->get<Bencode::String>();
                (*siblingKeyIt).second->get<Bencode::String>();
            } catch (...) {
                continue;
            }

            // Relink the azureus key to the sibling.
            delete (*siblingKeyIt).second;
            (*siblingKeyIt).second = (*azureusKeyIt).second;

            // We will erase the azureus key since we are no longer
            // need it.
            keysToErase.push_back(azureusKey);

            hDebug() << "Relinked" << azureusKey << "->" << siblingKey;
        }

        std::for_each(
            keysToErase.begin(), keysToErase.end(),
            [&d](const std::string &k) { d.erase(k); }
        );
    }

    void visit(Bencode::BencodeList *) {}
    void visit(Bencode::BencodeInteger *) {}
    void visit(Bencode::BencodeString *) {}
};

TorrentModel::TorrentModel(Bencode::Object *metadata) :
    metadata_(metadata),
    torrentSize_(0)
{
    assert(metadata_ != 0);

    // Store file list and info hash in cache for faster access
    try {
        constructAnnounceList();
        constructFileList();
        constructInfoHash();
    } catch (std::exception &e) {
        hDebug() << e.what();

        // Clean up after ourselves
        Bencode::ReleaseMemoryVisitor visitor;
        metadata_->accept(visitor);

        throw std::runtime_error("Failed to initialize torrent model");
    }
}

TorrentModel::~TorrentModel()
{
    Bencode::ReleaseMemoryVisitor visitor;
    metadata_->accept(visitor);
}

const std::string &TorrentModel::announceUri() const
{
    return Bencode::Path(metadata_, "announce").resolve<Bencode::String>();
}

const AnnounceList &TorrentModel::announceList() const
{
    return announceList_;
}

std::string TorrentModel::comment() const
{
    return Bencode::Path(metadata_, "comment").resolve<Bencode::String>("");
}

std::string TorrentModel::creator() const
{
    return Bencode::Path(metadata_, "created by").resolve<Bencode::String>("");
}

std::string TorrentModel::encoding() const
{
    return Bencode::Path(metadata_, "encoding").resolve<Bencode::String>("");
}

const InfoHash &TorrentModel::hash() const
{
    return infoHash_;
}

const std::string &TorrentModel::name() const
{
    return Bencode::Path(metadata_, "info", "name").resolve<Bencode::String>();
}

long long TorrentModel::creationDate() const
{
    return Bencode::Path(metadata_, "creation date").resolve<Bencode::Integer>();
}

unsigned int TorrentModel::pieceCount() const
{
    return Bencode::Path(metadata_, "info", "pieces")
                .resolve<Bencode::String>()
                .size() / Util::Sha1Hash::Hash::static_size;
}

unsigned int TorrentModel::pieceSize() const
{
    return Bencode::Path(metadata_, "info", "piece length").resolve<Bencode::Integer>();
}

Util::Sha1Hash::Hash TorrentModel::pieceHash(unsigned int piece) const
{
    Util::Sha1Hash::Hash hash;
    std::string &pieces = Bencode::Path(metadata_, "info", "pieces").resolve<Bencode::String>();

    hash.fromString(pieces.data() + piece * Util::Sha1Hash::Hash::static_size,
            Util::Sha1Hash::Hash::static_size);

    return hash;
}

unsigned long long TorrentModel::torrentSize() const
{
    return torrentSize_;
}

const FileList &TorrentModel::fileList() const
{
    return fileList_;
}

void TorrentModel::constructAnnounceList()
{

    if (Bencode::Path(metadata_, "announce-list").exists() &&
        Bencode::Path(metadata_, "announce-list").resolve<Bencode::List>().size() > 0)
    {
        unsigned int trackersFound = 0;
        auto &beAnnounceList = Bencode::Path(metadata_, "announce-list").resolve<Bencode::List>();

        for (auto tier = beAnnounceList.begin(); tier != beAnnounceList.end(); ++tier) {
            auto &beUriList = (*tier)->get<Bencode::List>();

            if (beUriList.size() == 0)
                continue;

            announceList_.resize(announceList_.size() + 1);

            for (auto uri = beUriList.begin(); uri != beUriList.end(); ++uri)
                announceList_.back().push_back((*uri)->get<Bencode::String>());

            trackersFound += beUriList.size();
        }

        if (trackersFound > 0) {
            hcDebug(name()) << "Found" << trackersFound << "trackers in" << announceList_.size()
                            << "tier(s)";
        } else {
            hcDebug(name()) << "Announce list doesn't contain any tracker URIs inside";
        }
    } else {
        // TODO: Use context-dependent debug output
        hcDebug(name()) << "Announce list is not present or empty";
    }
}

void TorrentModel::constructFileList()
{
    using namespace Bencode;

    if (Path(metadata_, "info", "files").exists()) {
        unsigned long long offset = 0;
        auto &beFileList = Path(metadata_, "info", "files").resolve<List>();

        fileList_.reserve(beFileList.size());

        for (auto file = beFileList.begin(); file != beFileList.end(); ++file) {
            FileDescriptor fileDescriptor;
            auto &beSubpathList= Bencode::Path(*file, "path").resolve<Bencode::List>();

            // Construct the full filename from the list of path elements
            for (auto pe = beSubpathList.begin(); pe != beSubpathList.end(); ++pe) {
                // TODO: Replace incompatible filename characters
                fileDescriptor.filename.append(1, '/');
                fileDescriptor.filename.append((*pe)->get<Bencode::String>());
            }

            fileDescriptor.size = Bencode::Path(*file, "length").resolve<Bencode::Integer>();
            fileDescriptor.absOffset = offset;

            fileList_.push_back(fileDescriptor);

            offset += fileDescriptor.size;
            torrentSize_ += fileDescriptor.size;
        }
    } else {
        FileDescriptor fileDescriptor;

        fileDescriptor.filename.append(1, '/');
        fileDescriptor.filename.append(Path(metadata_, "info", "name").resolve<String>());
        fileDescriptor.size = Path(metadata_, "info", "length").resolve<Integer>();
        fileDescriptor.absOffset = 0;

        fileList_.push_back(fileDescriptor);
        torrentSize_ = fileDescriptor.size;
    }
}

void TorrentModel::constructInfoHash()
{
    std::ostringstream resultStream;
    Bencode::SerializationVisitor visitor(resultStream);

    Bencode::Path(metadata_, "info").object()->accept(visitor);

    infoHash_ = Util::Sha1Hash::oneshot(resultStream.str());
}

static void tryValidateMetadata(const Bencode::Object *metadata)
{
    // Verify root dictionary
    Bencode::Path(metadata, "announce").resolve<Bencode::String>();
    Bencode::Path(metadata, "creation date").resolve<Bencode::Integer>(0);
    Bencode::Path(metadata, "comment").resolve<Bencode::String>("");
    Bencode::Path(metadata, "created by").resolve<Bencode::String>("");
    Bencode::Path(metadata, "encoding").resolve<Bencode::String>("");

    // Verify announce list
    if (Bencode::Path(metadata, "announce-list").exists()) {
        auto &announceList = Bencode::Path(metadata, "announce-list").resolve<Bencode::List>();

        for (auto tier = announceList.begin(); tier != announceList.end(); ++tier) {
            auto &urlList = (*tier)->get<Bencode::List>();

            for (auto url = urlList.begin(); url != urlList.end(); ++url)
                (*url)->get<Bencode::String>();
        }
    }

    // Verify info dictionary
    Bencode::Path(metadata, "info").resolve<Bencode::Dictionary>();
    Bencode::Path(metadata, "info", "private").resolve<Bencode::Integer>(0);
    Bencode::Path(metadata, "info", "name").resolve<Bencode::String>();

    auto pieceSize = Bencode::Path(metadata, "info", "piece length").resolve<Bencode::Integer>();

    if (pieceSize < 1)
        throw std::runtime_error("Piece size must be greater that 0");
    else if (pieceSize > 32 * 1024 * 1024)
        throw std::runtime_error("Piece size is too large");

    if (Bencode::Path(metadata, "info", "files").exists()) {
        // Got a multi-file torrent
        auto &fileList = Bencode::Path(metadata, "info", "files").resolve<Bencode::List>();

        for (auto file = fileList.begin(); file != fileList.end(); ++file) {
            Bencode::Object *fileDescriptor = *file;

            Bencode::Path(fileDescriptor, "md5sum").resolve<Bencode::String>("");
            if (Bencode::Path(fileDescriptor, "length").resolve<Bencode::Integer>() < 0)
                throw std::runtime_error("File size cannot be negative");

            auto &subpathList = Bencode::Path(fileDescriptor, "path").resolve<Bencode::List>();

            // Check if all path elements are strings
            for (auto subpath = subpathList.begin(); subpath != subpathList.end(); ++subpath)
                (*subpath)->get<Bencode::String>();

            // TODO: Verify there are no filename collisions
        }
    } else {
        // Got a single-file torrent
        if (Bencode::Path(metadata, "info", "length").resolve<Bencode::Integer>() < 0)
            throw std::runtime_error("File size cannot be negative");

        Bencode::Path(metadata, "info", "md5sum").resolve<Bencode::String>("");
    }

    size_t pieceCount = Bencode::Path(metadata, "info", "pieces") \
                                .resolve<Bencode::String>() \
                                .size();

    if (pieceCount == 0 || pieceCount % Util::Sha1Hash::Hash::static_size != 0)
        throw std::runtime_error("Pieces string has an invalid length");
}

std::string TorrentModel::toString() const
{
    std::ostringstream out(std::ios_base::binary | std::ios_base::out);

    Bencode::SerializationVisitor visitor(out);
    metadata_->accept(visitor);

    return out.str();
}

TorrentModel *TorrentModel::fromString(const std::string &string)
{
    try {
        std::istringstream in(string, std::ios_base::binary | std::ios_base::in);
        Bencode::Object *metadata = Bencode::Reader::parse(in);

        if (metadata == 0)
            throw std::runtime_error("Torrent metadata has malformed bencode structure");

        tryValidateMetadata(metadata);

        RelinkAzureusKeys relinker;
        metadata->accept(relinker);

        return new TorrentModel(metadata);
    } catch (std::exception &e) {
        hDebug() << e.what();
        return 0;
    }
}
