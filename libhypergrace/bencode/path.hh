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

#ifndef BENCODE_PATH_H_
#define BENCODE_PATH_H_

#include <exception>
#include <stdexcept>

#include "object.hh"
#include "typedefs.hh"
#include "trivialobject.hh"


namespace Hypergrace {
namespace Bencode {

/**
 * The Path class provides a  means of navigation through a hierarchy
 * of nested dictionaries.
 *
 * An existence  of particular  path can be  checked with  the method
 * @exists(). An  attempt to  @resolve() a path  that does  not exist
 * will raise  the std::runtime_error exception. Resolving  an object
 * that's  pointed  by  the path  using an  invalid type  will  cause
 * the std::bad_cast exception to be raised.
 */
class Path
{
public:
    /**
     * Constructs a Path object.
     *
     * A hierarchy  to traverse  is specified  by the  first argument
     * @dict. Argument pack, @args, must contain at least one or more
     * subpaths, all subpath arguments must be a string.
     */
    template<typename... Args>
    Path(const Object *dict, Args &... args)
    {
        try {
            target_ = traverse(*dict, args...);
        } catch (std::bad_cast &e) {
            target_ = 0;
        }
    }

    /**
     * Checks whether the path exists.
     */
    inline bool exists() const
    {
        return 0 != target_;
    }

    /**
     * Returns an object associated with particular path.
     */
    Object *object() const
    {
        return target_;
    }

    /**
     * Returns a  reference to the value of an object associated with
     * particular path.
     *
     * If  the path  doesn't exist  the std::runtime_error  exception
     * will be raised.
     *
     * If supplied  type -ValueType- does  not match  sought object's
     * type then the std::bad_cast exception will be raised.
     */
    template<typename ValueType> ValueType &resolve() const
    {
        if (exists()) {
            return target_->get<ValueType>();
        } else {
            throw std::runtime_error("Invalid path or path doesn't exist");
        }
    }

    /**
     * Returns a  reference to the value of an object associated with
     * particular path.
     *
     * If the  path doesn't exist  a reference to the  default value,
     * @defaultValue, will be returned.
     *
     * See documentation for @resolve() method to learn more  details
     * about raised exceptions.
     */
    template<typename ValueType>
    ValueType resolve(ValueType defaultValue) const
    {
        return (exists()) ? target_->get<ValueType>() : defaultValue;
    }

private:
    template<typename T>
    Object *traverse(const Object &wrapper, T &subpath)
    {
        auto &dict = wrapper.get<Dictionary>();
        auto it = dict.find(subpath);

        return (it != dict.end()) ? (*it).second : 0;
    }

    template<typename T, typename... Rest>
    Object *traverse(const Object &wrapper, T &subpath, Rest &... rest)
    {
        auto &dict = wrapper.get<Dictionary>();
        auto it = dict.find(subpath);

        return (it != dict.end()) ? traverse(*(*it).second, rest...) : 0;
    }

private:
    Object *target_;
};

} /* namespace Bencode */
} /* namespace Hypergrace */


#endif /* BENCODE_PATH_HH_ */
