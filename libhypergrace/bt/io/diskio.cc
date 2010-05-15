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

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <mutex>
#include <thread>

#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>

#include <thread/event.hh>
#include <util/sha1hash.hh>

#include "diskio.hh"

using namespace Hypergrace;
using namespace Hypergrace::Bt;


class DiskIo::Private
{
public:
    struct WriteBlocksRequest {
        WriteBlocksRequest() {}

        WriteBlocksRequest(WriteBlocksRequest &&other)
        {
            *this = std::move(other);
        }

        void operator =(WriteBlocksRequest &&other)
        {
            if (this == &other)
                return;

            bundle = other.bundle;
            writeList = std::move(other.writeList);
            onWriteSuccess = other.onWriteSuccess;
            onWriteFailure = other.onWriteFailure;
        }

        const TorrentBundle *bundle;
        DiskIo::WriteList writeList;
        DiskIo::WriteSuccessDelegate onWriteSuccess;
        DiskIo::WriteFailureDelegate onWriteFailure;
    };

    struct WriteDataRequest {
        std::string filename;
        unsigned long long offset;
        std::string data;
        DiskIo::WriteSuccessDelegate onWriteSuccess;
        DiskIo::WriteFailureDelegate onWriteFailure;
    };

    struct ReadRequest {
        ReadRequest() {}

        ReadRequest(ReadRequest &&other)
        {
            *this = std::move(other);
        }

        void operator =(ReadRequest &&other)
        {
            if (this == &other)
                return;

            bundle = other.bundle;
            readList = std::move(other.readList);
            onReadSuccess = other.onReadSuccess;
            onReadFailure = other.onReadFailure;
        }

        const TorrentBundle *bundle;
        DiskIo::ReadList readList;
        DiskIo::ReadSuccessDelegate onReadSuccess;
        DiskIo::ReadFailureDelegate onReadFailure;
    };

    struct VerifyRequest {
        const TorrentBundle *bundle;
        unsigned int piece;
        DiskIo::VerifySuccessDelegate onVerifySuccess;
        DiskIo::VerifyFailureDelegate onVerifyFailure;
    };

    struct LocationDescriptor
    {
        LocationDescriptor() {}
        LocationDescriptor(LocationDescriptor &&other) { *this = std::move(other); }

        void operator =(LocationDescriptor &&other)
        {
            if (this == &other)
                return;

            underlyingFiles = std::move(other.underlyingFiles);
            hintOffset = other.hintOffset;
            hintIndex = other.hintIndex;
        }

        struct IoDescriptor
        {
            unsigned long long offset;
            const FileDescriptor *file;
        };

        std::deque<IoDescriptor> underlyingFiles;

        unsigned long long hintOffset;
        unsigned int hintIndex;
    };

    struct OpenFileDescriptor {
        int fd;
        Util::Time accessTime;
    };

public:
    Private() :
        stop_(false),
        ioThread_(Delegate::make(this, &Private::ioLoop))
    {
    }

    ~Private()
    {
    }

private:
    LocationDescriptor describeLocation(
            const TorrentBundle &bundle,
            unsigned long long where,
            unsigned int size,
            unsigned long long hintOffset,
            unsigned int hintIndex)
    {
        LocationDescriptor location;
        const FileList &fileList = bundle.model().fileList();

        size_t left = 0;
        size_t right = fileList.size();
        size_t index = (size_t)-1;

        if (where == hintOffset) {
            index = hintIndex;
        } else {
            while (left < right) {
                size_t middle = left + (right - left) / 2;

                if (where < fileList[middle].absOffset) {
                    right = middle;
                } else if (where >= fileList[middle].absOffset &&
                           where >= fileList[middle].absOffset + fileList[middle].size) {
                    left = middle;
                } else {
                    index = middle;
                    break;
                }
            }
        }

        assert(index != (size_t)-1);

        // A rare case when we landed upon an empty file.
        // Find the first adjacent non-empty file.
        while (fileList[index].size == 0) {
            assert(index < fileList.size());
            ++index;
        }

        LocationDescriptor::IoDescriptor firstIoDescriptor;
        firstIoDescriptor.offset = where - fileList[index].absOffset;
        firstIoDescriptor.file = &fileList[index];

        location.underlyingFiles.push_back(firstIoDescriptor);

        // If the given block doesn't overlap with other files then we can
        // safely shutdown search at this point.
        if (size <= fileList[index].size - firstIoDescriptor.offset) {
            // Improve performance by giving a hint where next request
            // might start. This allows us to completely avoid binary
            // search if prediction is correct.

            location.hintIndex = index;
            location.hintIndex += (size == fileList[index].size - firstIoDescriptor.offset);
            location.hintOffset = where + size;

            return location;
        }

        // The block spans across multiple files and thus these files are
        // also must be accounted in the LocationDescriptor struct.
        unsigned int remainingSize;
        remainingSize = size - (fileList[index].size - firstIoDescriptor.offset);

        ++index;

        assert(index < fileList.size());

        // Insert all adjacent files into the LocationDescriptor struct.
        // No checks on index bounds are performed because callers of
        // this method are supposedly trusted.
        while (remainingSize > 0 && remainingSize >= fileList[index].size) {
            // Don't include empty files.
            if (fileList[index].size > 0) {
                LocationDescriptor::IoDescriptor ioDescriptor;
                ioDescriptor.offset = 0;
                ioDescriptor.file = &fileList[index];

                location.underlyingFiles.push_back(ioDescriptor);
            }

            remainingSize -= fileList[index].size;
            ++index;

            assert(!(remainingSize > 0 && index >= fileList.size()));
        }

        // Check whether the given block fully spans across the last
        // file in the file chain or not.
        if (remainingSize != 0) {
            LocationDescriptor::IoDescriptor ioDescriptor;
            ioDescriptor.offset = 0;
            ioDescriptor.file = &fileList[index];

            location.underlyingFiles.push_back(ioDescriptor);

            location.hintIndex = index;
            location.hintOffset = where + size;
        } else {
            location.hintIndex = index + 1;
            location.hintOffset = where + size;
        }

        return location;
    }

    void satisfyRequest(const WriteBlocksRequest &request)
    {
        unsigned int pieceSize = request.bundle->model().pieceSize();
        const std::string &path = request.bundle->configuration().storageDirectory();

        unsigned long long hintedOffset = 0;
        unsigned int hintedIndex = 0;

        for (auto it = request.writeList.begin(); it !=request.writeList.end(); ++it) {
            LocationDescriptor location = describeLocation(
                    *request.bundle,
                    ((unsigned long long)std::get<0>(*it)) * pieceSize + std::get<1>(*it),
                    std::get<2>(*it).size(),
                    hintedOffset, hintedIndex);

            assert(location.underlyingFiles.size() > 0);

            // Save hints, these allow describeLocation() method to
            // work more efficiently on contiguous blocks.
            hintedOffset = location.hintOffset;
            hintedIndex = location.hintIndex;

            unsigned int dataOffset = 0;

            for (auto ioIt = location.underlyingFiles.begin();
                 ioIt != location.underlyingFiles.end();
                 ++ioIt)
            {
                unsigned long long dataRemain = std::get<2>(*it).size() - dataOffset;
                unsigned long long possibleAmount = (*ioIt).file->size - (*ioIt).offset;

                unsigned int writeAmount = std::min(dataRemain, possibleAmount);

                ssize_t wrote = write(path + (*ioIt).file->filename, (*ioIt).offset,
                        std::get<2>(*it).data() + dataOffset, writeAmount);

                //hDebug() << "Wrote" << wrote << "bytes at offset" << (*ioIt).offset
                //         << "in file" << (*ioIt).file->filename << "of" << std::get<0>(*it);

                if (wrote >= 0 && (unsigned int)wrote == writeAmount) {
                    dataOffset += writeAmount;
                } else {
                    request.onWriteFailure();
                    return;
                }
            }
        }

        request.onWriteSuccess();
    }

    void satisfyRequest(const WriteDataRequest &request)
    {
        ssize_t wrote = write(request.filename, request.offset, request.data.data(),
                request.data.size());

        if (wrote >= 0 && (size_t)wrote == request.data.size()) {
            request.onWriteSuccess();
        } else {
            request.onWriteFailure();
        }
    }

    void satisfyRequest(const ReadRequest &request)
    {
        std::string buffer;

        unsigned int pieceSize = request.bundle->model().pieceSize();
        const std::string &path = request.bundle->configuration().storageDirectory();

        unsigned long long hintedOffset = 0;
        unsigned int hintedIndex = 0;

        for (auto it = request.readList.begin(); it != request.readList.end(); ++it) {

            Private::LocationDescriptor location = describeLocation(
                    *request.bundle,
                    ((unsigned long long)std::get<0>(*it)) * pieceSize + std::get<1>(*it),
                    std::get<2>(*it),
                    hintedOffset, hintedIndex);

            assert(location.underlyingFiles.size() > 0);

            hintedOffset = location.hintOffset;
            hintedIndex = location.hintIndex;

            unsigned int remainingSize = std::get<2>(*it);

            for (auto ioIt = location.underlyingFiles.begin();
                 ioIt != location.underlyingFiles.end();
                 ++ioIt)
            {
                assert(remainingSize > 0);

                unsigned long long possibleAmount = (*ioIt).file->size - (*ioIt).offset;

                unsigned int readAmount =
                        std::min<unsigned long long>(remainingSize, possibleAmount);

                ssize_t got = read(path + (*ioIt).file->filename, (*ioIt).offset,
                        buffer, readAmount);

                if (got >= 0 && (unsigned int)got == readAmount) {
                    remainingSize -= readAmount;
                } else {
                    request.onReadFailure();
                    return;
                }
            }
        }

        request.onReadSuccess(buffer);
    }

    void satisfyRequest(const VerifyRequest &request)
    {
        Util::Sha1Hash hash;

        const TorrentModel &model = request.bundle->model();
        const std::string &path = request.bundle->configuration().storageDirectory();

        unsigned int offset = request.piece * model.pieceSize();
        unsigned int remainingSize = 0;

        if (request.piece < model.pieceCount() - 1) {
            remainingSize = model.pieceSize();
        } else {
            unsigned int modulo = model.torrentSize() % model.pieceSize();
            remainingSize = (modulo != 0) ? modulo : model.pieceSize();
        }

        Private::LocationDescriptor location =
            describeLocation(*request.bundle, offset, remainingSize, 0, 0);

        if (location.underlyingFiles.size() == 0) {
            request.onVerifyFailure(request.piece);
            return;
        }

        for (auto ioIt = location.underlyingFiles.begin();
             ioIt != location.underlyingFiles.end();
             ++ioIt)
        {
            assert(remainingSize > 0);

            std::string buffer;

            unsigned long long possibleAmount = (*ioIt).file->size - (*ioIt).offset;
            unsigned int readAmount = std::min<unsigned long long>(remainingSize, possibleAmount);

            ssize_t got = read(path + (*ioIt).file->filename, (*ioIt).offset, buffer, readAmount);

            //hDebug() << "Read" << got << "bytes at offset" << (*ioIt).offset
            //         << "in file" << (*ioIt).file->filename << "of" << request.piece;

            if (got >= 0 && (unsigned int)got == readAmount) {
                hash.update(buffer);
                remainingSize -= readAmount;
            } else {
                request.onVerifyFailure(request.piece);
                return;
            }
        }

        if (hash.final() == model.pieceHash(request.piece)) {
            request.onVerifySuccess(request.piece);
        } else {
            request.onVerifyFailure(request.piece);
        }
    }

    OpenFileDescriptor &open(const std::string &filename)
    {
        // TODO: Move all platform specific stuff to the Util::Filesystem
        // class.
        auto file = openFiles_.find(filename);

        // Use cache to store recently opened files to avoid repeatedly
        // opening/closing them
        if (file != openFiles_.end()) {
            return (*file).second;
        } else {
            OpenFileDescriptor fileDescriptor;

            fileDescriptor.fd = ::open(filename.c_str(), O_CREAT | O_NOATIME | O_RDWR);
            fileDescriptor.accessTime = Util::Time::monotonicTime();

            file = openFiles_.insert(std::make_pair(filename, fileDescriptor)).first;
        }

        return (*file).second;
    }

    ssize_t read(
            const std::string &filename,
            unsigned long long offset,
            std::string &buffer,
            unsigned int size)
    {
        // TODO: Move all platform specific stuff to the Util::Filesystem
        // class.
        OpenFileDescriptor &readSrc = open(filename);
        char readBuffer[1024 * 1024];

        buffer.reserve(buffer.size() + size);

        if (readSrc.fd == -1) {
            hWarning() << "Failed to open file" << filename << "for reading"
                       << "(" << strerror(errno) << ")";
            return -1;
        }

        readSrc.accessTime = Util::Time::monotonicTime();

        if (::lseek64(readSrc.fd, offset, SEEK_SET) == (off64_t)-1) {
            hWarning() << "Failed to reset file pointer to point at offset" << offset
                       << "in file" << filename << "(" << strerror(errno) << ")";
            return -1;
        }

        unsigned int totalRead = 0;

        while (totalRead < size) {
            unsigned int readAmount = std::min<unsigned int>(sizeof(readBuffer), size - totalRead);
            ssize_t got = ::read(readSrc.fd, readBuffer, readAmount);

            if (got > 0) {
                buffer.append(readBuffer, got);
                totalRead += got;
            } else if (got == 0) {
                break;
            } else {
                hWarning() << "Failed to read" << size << "bytes from file" << filename
                        << "(" << strerror(errno) << ")";
                return -1;
            }
        }

        return totalRead;
    }

    ssize_t write(
            const std::string &filename,
            unsigned long long offset,
            const char *buffer,
            unsigned int size)
    {
        // TODO: Move all platform specific stuff to the Util::Filesystem
        // class.
        OpenFileDescriptor &writeDst = open(filename);

        if (writeDst.fd == -1) {
            hWarning() << "Failed to open file" << filename << "for writing"
                       << "(" << strerror(errno) << ")";
            return -1;
        }

        writeDst.accessTime = Util::Time::monotonicTime();

        if (::lseek64(writeDst.fd, offset, SEEK_SET) == (off64_t)-1) {
            hWarning() << "Failed to reposition file pointer to offset" << offset
                       << "in file" << filename << "(" << strerror(errno) << ")";
            return -1;
        }

        ssize_t wrote = ::write(writeDst.fd, buffer, size);

        if (wrote == -1) {
            hWarning() << "Failed to write" << size << "bytes at offset" << offset << "in file"
                       << filename << "(" << strerror(errno) << ")";
            return -1;
        }

        return wrote;
    }

    void closeOld()
    {
        // TODO: Move all platform specific stuff to the Util::Filesystem
        // class.
        std::deque<std::map<std::string, OpenFileDescriptor>::iterator> toErase;

        for (auto file = openFiles_.begin(); file != openFiles_.end(); ++file) {
            if ((*file).second.accessTime < Util::Time(0, 1, 0))
                continue;

            if (::close((*file).second.fd) == -1)
                hDebug() << "An error occurred while closing file (" << strerror(errno) << ")";

            toErase.push_back(file);
        }

        for (auto it = toErase.begin(); it != toErase.end(); ++it)
            openFiles_.erase(*it);
    }

    void ioLoop()
    {
        while (!stop_) {
            std::deque<WriteBlocksRequest> blocksToWrite;
            std::deque<WriteDataRequest> dataToWrite;
            std::deque<ReadRequest> toRead;
            std::deque<VerifyRequest> toVerify;

            {
                // Don't hold lock for too long by copying data (stealing)
                // from the original request lists and processing the new
                // lists instead.
                std::lock_guard<std::mutex> l(anchor_);

                if (!writeBlocksRequests_.empty()) {
                    blocksToWrite = std::move(writeBlocksRequests_);
                    writeBlocksRequests_ = std::deque<WriteBlocksRequest>();
                }

                if (!writeDataRequests_.empty()) {
                    dataToWrite = std::move(writeDataRequests_);
                    writeDataRequests_ = std::deque<WriteDataRequest>();
                }

                if (!readRequests_.empty()) {
                    toRead = std::move(readRequests_);
                    readRequests_ = std::deque<ReadRequest>();
                }

                if (!verifyRequests_.empty()) {
                    toVerify = std::move(verifyRequests_);
                    verifyRequests_ = std::deque<VerifyRequest>();
                }
            }

            for (auto request = blocksToWrite.begin(); request != blocksToWrite.end(); ++request)
                satisfyRequest(*request);

            for (auto request = toRead.begin(); request != toRead.end(); ++request)
                satisfyRequest(*request);

            for (auto request = toVerify.begin(); request != toVerify.end(); ++request)
                satisfyRequest(*request);

            for (auto request = dataToWrite.begin(); request != dataToWrite.end(); ++request)
                satisfyRequest(*request);

            // Close files that we haven't accessed for some time
            if (lastCleanupTime_ >= Util::Time(0, 0, 30)) {
                closeOld();
                lastCleanupTime_ = Util::Time::monotonicTime();
            }

            if (writeBlocksRequests_.empty() && writeDataRequests_.empty() &&
                readRequests_.empty() && verifyRequests_.empty())
            {
                requestsAvailableEvent_.wait(10 * 1000);
            }
        }
    }

public:
    std::deque<WriteBlocksRequest> writeBlocksRequests_;
    std::deque<WriteDataRequest> writeDataRequests_;
    std::deque<ReadRequest> readRequests_;
    std::deque<VerifyRequest> verifyRequests_;

    std::map<std::string, OpenFileDescriptor> openFiles_;
    Util::Time lastCleanupTime_;

    Thread::Event requestsAvailableEvent_;

    volatile bool stop_;
    std::thread ioThread_;

    std::mutex anchor_;
};

DiskIo::DiskIo() :
    d(new Private())
{
}

DiskIo::~DiskIo()
{
    d->stop_ = true;
    d->requestsAvailableEvent_.signal();
    d->ioThread_.join();

    delete d;
}

void DiskIo::writeBlocks(
        const TorrentBundle &bundle,
        WriteList &&writeList,
        const WriteSuccessDelegate &onSuccess,
        const WriteFailureDelegate &onFailure)
{
    std::lock_guard<std::mutex> l(d->anchor_);

    Private::WriteBlocksRequest writeRequest;
    writeRequest.bundle = &bundle;
    writeRequest.writeList = std::move(writeList);
    writeRequest.onWriteSuccess = onSuccess;
    writeRequest.onWriteFailure = onFailure;

    d->writeBlocksRequests_.push_back(std::move(writeRequest));
    d->requestsAvailableEvent_.signal();
}

void DiskIo::writeData(
        const std::string &filename,
        unsigned long long offset,
        const std::string &data,
        const WriteSuccessDelegate &onSuccess,
        const WriteFailureDelegate &onFailure)
{
    Private::WriteDataRequest writeRequest;
    writeRequest.filename = filename;
    writeRequest.offset = offset;
    writeRequest.data = data;
    writeRequest.onWriteSuccess = onSuccess;
    writeRequest.onWriteFailure = onFailure;

    std::lock_guard<std::mutex> l(d->anchor_);

    d->writeDataRequests_.push_back(std::move(writeRequest));
    d->requestsAvailableEvent_.signal();
}

void DiskIo::readBlocks(
        const TorrentBundle &bundle,
        ReadList &&readList,
        const ReadSuccessDelegate &onSuccess,
        const ReadFailureDelegate &onFailure)
{
    std::lock_guard<std::mutex> l(d->anchor_);

    Private::ReadRequest readRequest;
    readRequest.bundle = &bundle;
    readRequest.readList = std::move(readList);
    readRequest.onReadSuccess = onSuccess;
    readRequest.onReadFailure = onFailure;

    d->readRequests_.push_back(std::move(readRequest));
    d->requestsAvailableEvent_.signal();
}

void DiskIo::readBlock(
        const TorrentBundle &bundle,
        unsigned int piece,
        unsigned int offset,
        unsigned int size,
        const ReadSuccessDelegate &onSuccess,
        const ReadFailureDelegate &onFailure)
{
    std::lock_guard<std::mutex> l(d->anchor_);

    Private::ReadRequest readRequest;
    readRequest.bundle = &bundle;
    readRequest.readList.push_back(std::make_tuple(piece, offset, size));
    readRequest.onReadSuccess = onSuccess;
    readRequest.onReadFailure = onFailure;

    d->readRequests_.push_back(std::move(readRequest));
    d->requestsAvailableEvent_.signal();
}

void DiskIo::verifyPiece(
        const TorrentBundle &bundle,
        unsigned int piece,
        const VerifySuccessDelegate &onSuccess,
        const VerifyFailureDelegate &onFailure)
{
    std::lock_guard<std::mutex> l(d->anchor_);

    Private::VerifyRequest verifyRequest;
    verifyRequest.bundle = &bundle;
    verifyRequest.piece = piece;
    verifyRequest.onVerifySuccess = onSuccess;
    verifyRequest.onVerifyFailure = onFailure;

    d->verifyRequests_.push_back(std::move(verifyRequest));
    d->requestsAvailableEvent_.signal();
}
