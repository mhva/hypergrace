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

#include <cmath>

#include <debug/debug.hh>

#include "rating.hh"

using namespace Hypergrace::Util;


Rating::Rating() :
    upvotes_(0),
    downvotes_(0)
{
}

Rating::Rating(unsigned int upvotes, unsigned int downvotes) :
    upvotes_(upvotes),
    downvotes_(downvotes)
{
}

void Rating::upvote()
{
    ++upvotes_;
}

void Rating::downvote()
{
    ++downvotes_;
}

void Rating::setUpvotes(unsigned int upvotes)
{
    upvotes_ = upvotes;
}

void Rating::setDownvotes(unsigned int downvotes)
{
    downvotes_ = downvotes;
}

float Rating::rating() const
{
    // Reference: http://www.evanmiller.org/how-not-to-sort-by-average-rating.html

    const unsigned int n = upvotes_ + downvotes_;

    if (n == 0)
        return 0.0;

    const float zs = 1.959963971;
    const float phat = 1.0 * upvotes_ / n;

    return (phat + zs * zs / (2 * n) - zs * sqrt((phat * (1 - phat) + zs * zs / (4 * n)) / n))
                /
           (1 + zs * zs / n);
}

unsigned int Rating::upvotes() const
{
    return upvotes_;
}

unsigned int Rating::downvotes() const
{
    return downvotes_;
}

bool Rating::operator <(const Rating &right) const
{
    return *this != right && rating() < right.rating();
}

bool Rating::operator ==(const Rating &right) const
{
    return upvotes_ == right.upvotes_ && downvotes_ == right.downvotes_;
}

bool Rating::operator !=(const Rating &right) const
{
    return !(*this == right);
}

bool Rating::operator >(const Rating &right) const
{
    return *this != right && rating() > right.rating();
}

Hypergrace::Debug::Debug &
operator <<(Hypergrace::Debug::Debug &stream, const Hypergrace::Util::Rating &rating)
{
    return stream << rating.rating();
}
