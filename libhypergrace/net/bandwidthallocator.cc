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

#include "bandwidthallocator.hh"

using namespace Hypergrace::Net;


BandwidthAllocator::BandwidthAllocator() :
    tokens_(-1),
    maxTokens_(-1)
{
}

BandwidthAllocator::~BandwidthAllocator()
{
}

void BandwidthAllocator::limit(int limit)
{
    maxTokens_ = limit;
    renew();
}

int BandwidthAllocator::allocate(int size)
{
    int maxTokens = maxTokens_;

    if (maxTokens > 0) {
        int expected = tokens_;
        int alloc;

        do {
            alloc = (expected >= size) ? size : expected;
        } while (!tokens_.compare_exchange_weak(expected, expected - alloc));

        return alloc;
    } else if (maxTokens == 0) {
        return 0;
    } else {
        return size;
    }
}

void BandwidthAllocator::release(int size)
{
    while (true) {
        int expected = tokens_;
        int maxTokens = maxTokens_;
        int newValue = (expected + size <= maxTokens) ? expected + size : maxTokens;

        if (tokens_.compare_exchange_weak(expected, newValue)) {
            break;
        }
    }
}

void BandwidthAllocator::renew()
{
    while (true) {
        int expected = tokens_;

        if (tokens_.compare_exchange_weak(expected, maxTokens_)) {
            break;
        }
    }
}
