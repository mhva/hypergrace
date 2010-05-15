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

#ifndef UTIL_RATING_HH_
#define UTIL_RATING_HH_

namespace Hypergrace { namespace Debug { class Debug; } }

namespace Hypergrace {
namespace Util {

class Rating
{
public:
    Rating();
    Rating(unsigned int, unsigned int);

    void upvote();
    void downvote();

    void setUpvotes(unsigned int);
    void setDownvotes(unsigned int);

    float rating() const;

    unsigned int upvotes() const;
    unsigned int downvotes() const;

public:
    bool operator <(const Rating &) const;
    bool operator ==(const Rating &) const;
    bool operator !=(const Rating &) const;
    bool operator >(const Rating &) const;

private:
    unsigned int upvotes_;
    unsigned int downvotes_;
};

} /* namespace Util */
} /* namespace Hypergrace */

Hypergrace::Debug::Debug &
operator <<(Hypergrace::Debug::Debug &, const Hypergrace::Util::Rating &);

#endif /* UTIL_RATING_HH_ */
