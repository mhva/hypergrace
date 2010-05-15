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

#ifndef ANY_HH_
#define ANY_HH_

#include <cxxabi.h>

#include <string>
#include <typeinfo>

namespace Hypergrace {
namespace Bencode {

/**
 * The Any class is a generic container that's able to store a single
 * copy-constructible object and fetch it in a type safe way.
 *
 * This class  is very  similar to the boost::any class  available in
 * the boost library.
 */
class Any
{
public:
    template<class ValueType>
    explicit Any(const ValueType &holdee) :
        holder_(new Holder<ValueType>(holdee))
    {
    }

    Any(const Any &c) :
        holder_(c.holder_->copy())
    {
    }

    virtual ~Any()
    {
        delete holder_;
    }

    /**
     * Returns an enclosed object.
     *
     * In  the  case when  requested  type,  ValueType, and  enclosed
     * object's type  are not  the same, the  std::bad_cast exception
     * will be raised.
     */
    template<class ValueType> ValueType &get() const
    {
        if (typeid(ValueType) == holder_->type()) {
            return static_cast<Holder<ValueType> *>(holder_)->holdee;
        } else {
            throw BadCastException(holder_->type(), typeid(ValueType));
        }
    }

private:
    class HolderBase
    {
    public:
        virtual HolderBase *copy() const = 0;
        virtual const std::type_info &type() const = 0;
    };

    template<class ValueType>
    class Holder : public HolderBase
    {
    public:
        Holder(const ValueType &h) :
            holdee(h)
        {
        }

        HolderBase *copy() const
        {
            return new Holder(holdee);
        }

        const std::type_info &type() const
        {
            return typeid(holdee);
        }

    public:
        ValueType holdee;
    };

    class BadCastException : public std::bad_cast
    {
    public:
        BadCastException(const std::type_info &from, const std::type_info &to) throw()
        {
            message.append("Attempt to cast from the stored type '").append(typeName(from))
                   .append("' to the '").append(typeName(to)).append("'");
        }

        ~BadCastException() throw()
        {
        }

        const char *what() const throw()
        {
            return message.c_str();
        }

    private:
        std::string typeName(const std::type_info &info)
        {
            int status = 0;
            char *buffer = abi::__cxa_demangle(info.name(), 0, 0, &status);

            if (buffer != 0) {
                std::string result(buffer);
                delete buffer;

                return result;
            } else {
                return info.name();
            }
        }

    private:
        std::string message;
    };

private:
    HolderBase *holder_;
};

} /* namespace Bencode */
} /* namespace Hypergrace */

#endif // ANY_HH_
