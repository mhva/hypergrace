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

#include <linux/falloc.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <sstream>

#include <debug/debug.hh>

#include "filesystem.hh"

using namespace Hypergrace;
using namespace Hypergrace::Util;


bool FileSystem::createPath(const std::string &path, int mode)
{
    auto subpathList = splitPath(path);

    if (subpathList.empty()) {
        hDebug() << path << "is an invalid path";
        return false;
    }

    auto end = (path[path.size() - 1] != '/') ? subpathList.end() - 1 : subpathList.end();

    std::string currentPath;

    for (auto it = subpathList.begin(); it != end; ++it) {
        currentPath += "/";
        currentPath += *it;

        struct stat st;
        if (stat(currentPath.data(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                continue;
            } else {
                hDebug() << "Failed to create path" << path << "( Inferior file" << *it << ")";
                return false;
            }
        } else if (errno == ENOENT) {
            if (mkdir(currentPath.data(), mode) == -1) {
                hDebug() << "Failed to create directory at" << currentPath
                         << "(" << strerror(errno) << ")";
                return false;
            }
        } else {
            hDebug() << "Failed to create path" << path << "(" << strerror(errno) << ")";
            return false;
        }
    }

    return true;
}

bool FileSystem::createFile(const std::string &filename, int mode)
{
    int fd = open(filename.data(), O_CREAT | O_WRONLY, mode);

    if (fd != -1) {
        close(fd);
        return true;
    } else {
        hDebug() << "Failed to create file" << filename << "(" << strerror(errno) << ")";
        return false;
    }
}

bool FileSystem::preallocateFile(const std::string &filename, long long desiredSize)
{
    int fd = open(filename.data(), O_WRONLY);

    if (fd == -1) {
        hDebug() << "Failed to open file" << filename << "for preallocation"
                 << "(" << strerror(errno) << ")";
        return false;
    }

    if (fallocate(fd, 0, 0, desiredSize) == 0) {
        return true;
    } else {
        hDebug() << "Failed to preallocate file (" << strerror(errno) << ")";
        return false;
    }
}

bool FileSystem::fileExists(const std::string &filename)
{
    struct stat st;

    if (stat(filename.data(), &st) == 0) {
        return true;
    } else if (errno == ENOENT) {
        return false;
    } else {
        hDebug() << "Failed to check file's existence" << filename
                 << "(" << strerror(errno) << ")";
        return false;
    }
}

long long FileSystem::fileSize(const std::string &filename)
{
    struct stat st;

    if (stat(filename.data(), &st) == 0) {
        return st.st_size;
    } else {
        hDebug() << "Failed to stat file" << filename << "(" << strerror(errno) << ")";
        return -1;
    }
}

std::vector<std::string> FileSystem::splitPath(const std::string &path)
{
    std::vector<std::string> components;
    std::istringstream pathStream(path);
    std::string component;

    while (std::getline(pathStream, component, '/')) {
        if (component.empty())
            continue;

        components.push_back(component);
        component.clear();
    }

    return components;
}
