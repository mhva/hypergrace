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

#ifndef NET_PACKETFRAMEWORK_HH_
#define NET_PACKETFRAMEWORK_HH_

#include <arpa/inet.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>

#include <debug/debug.hh>
#include <net/packet.hh>


namespace Hypergrace {
namespace Net {
namespace Details {

    template<typename Tuple, size_t field>
    struct AccumulateSize
    {
        inline AccumulateSize(size_t &accumulator, const Tuple &tuple)
        {
            accumulator += std::get<field>(tuple).size();
            AccumulateSize<Tuple, field - 1>(accumulator, tuple);
        }
    };

    template<typename Tuple>
    struct AccumulateSize<Tuple, 0>
    {
        inline AccumulateSize(size_t &accumulator, const Tuple &tuple)
        {
            accumulator += std::get<0>(tuple).size();
        }
    };

    template<typename Tuple, size_t slide = std::tuple_size<Tuple>::value>
    struct MatchString
    {
        MatchString(bool &result, Tuple &fields, const std::string &string, size_t offset)
        {
            ssize_t fieldSize = std::get<std::tuple_size<Tuple>::value - slide>(fields)
                                        .match(fields, string, offset);
            if (fieldSize >= 0)
                MatchString<Tuple, slide - 1>(result, fields, string, offset + fieldSize);
        }
    };

    template<typename Tuple>
    struct MatchString<Tuple, 0>
    {
        MatchString(bool &result, Tuple &, const std::string &, size_t) { result = true; }
    };

    template<typename Tuple, size_t field = std::tuple_size<Tuple>::value - 1>
    struct Serialize
    {
        inline Serialize(std::string &accumulator, const Tuple &fields)
        {
            Serialize<Tuple, field - 1>(accumulator, fields);
            accumulator.append(std::get<field>(fields).data());
        }
    };

    template<typename Tuple>
    struct Serialize<Tuple, 0>
    {
        inline Serialize(std::string &accumulator, const Tuple &fields)
        {
            accumulator.append(std::get<0>(fields).data());
        }
    };

    template<typename Tuple>
    std::string serializeFields(const Tuple &fields)
    {
        std::string accumulator;

        Serialize<Tuple>(accumulator, fields);

        return accumulator;
    }

    template<typename Tuple>
    bool matchString(const std::string &string, Tuple &fields, size_t offset)
    {
        bool result = false;

        MatchString<Tuple>(result, fields, string, offset);

        return result;
    }

    template<typename Tuple>
    static size_t fieldsSize(const Tuple &fields)
    {
        size_t size = 0;

        AccumulateSize<Tuple, std::tuple_size<Tuple>::value - 1>(size, fields);

        return size;
    }

    template<typename IntegerType>
    class BigEndianFilter
    {
    public:
        inline static IntegerType transformFromInput(uint16_t v) { return ntohs(v); }
        inline static IntegerType transformFromInput(uint32_t v) { return ntohl(v); }
        inline static IntegerType transformFromInput(int16_t v) { return ntohs(v); }
        inline static IntegerType transformFromInput(int32_t v) { return ntohl(v); }
        inline static IntegerType transformForOutput(uint16_t v) { return htons(v); }
        inline static IntegerType transformForOutput(uint32_t v) { return htonl(v); }
        inline static IntegerType transformForOutput(int16_t v) { return htons(v); }
        inline static IntegerType transformForOutput(int32_t v) { return htonl(v); }
    };

    template<typename IntegerType>
    class NoFilter
    {
    public:
        inline static IntegerType transformFromInput(IntegerType v) { return v; }
        inline static IntegerType transformForOutput(IntegerType v) { return v; }
    };

    template<typename T>
    class Matcher
    {
    public:
        typedef T ValueType;

    public:
        Matcher() {}
        Matcher(const T &v) : value_(v) {}

        inline const T &cref() const { return value_; }
        inline T &ref() { return value_; }

    protected:
        T value_;
    };

} /* namespace Details */


template<typename IntegerType, typename Filter>
class IntegerMatcher : public Details::Matcher<IntegerType>
{
private:
    typedef Details::Matcher<IntegerType> BaseClass;

public:
    typedef typename BaseClass::ValueType ValueType;

public:
    IntegerMatcher() { this->value_ = 0; }
    IntegerMatcher(typename BaseClass::ValueType v) : BaseClass(v) {}

    inline std::string data() const
    {
        ValueType result = Filter::transformForOutput(this->value_);
        return std::string((char *)&result, sizeof(ValueType));
    }

    inline static size_t size() { return staticSize; }

    template<typename Tuple>
    ssize_t match(const Tuple &fields, const std::string &data, unsigned int offset)
    {
        if (data.size() >= offset + sizeof(IntegerType)) {
            this->value_ = Filter::transformFromInput(*(IntegerType *)&data.data()[offset]);
            return sizeof(IntegerType);
        } else {
            return -1;
        }
    }

    inline void operator =(ValueType v) { this->value_ = v; }

public:
    const static size_t staticSize = sizeof(IntegerType);
};

class ByteMatcher
    : public IntegerMatcher<unsigned char, Details::NoFilter<unsigned char> >
{
private:
    typedef IntegerMatcher<ValueType, Details::NoFilter<ValueType> > BaseClass;

public:
    ByteMatcher() {}
    ByteMatcher(ValueType v) : BaseClass(v) {}

    inline void operator =(ValueType v) { this->value_ = v; }
};

template<typename IntegerType>
class BigEndianIntegerMatcher
    : public IntegerMatcher<IntegerType, Details::BigEndianFilter<IntegerType> >
{
private:
    typedef IntegerMatcher<IntegerType, Details::BigEndianFilter<IntegerType> > BaseClass;

public:
    typedef typename BaseClass::ValueType ValueType;

public:
    BigEndianIntegerMatcher() {}
    BigEndianIntegerMatcher(ValueType v) : BaseClass(v) {}

    inline void operator =(ValueType v) { this->value_ = v; }
};

template<size_t length>
class StaticLengthStringMatcher : public Details::Matcher<std::string>
{
private:
    typedef Details::Matcher<ValueType> BaseClass;

public:
    StaticLengthStringMatcher() {}
    StaticLengthStringMatcher(const ValueType &v) : BaseClass(v) {}

    inline std::string data() const { return this->value_; }

    inline static size_t size()
    {
        static_assert(length > 0, "Zero-length strings are not allowed");
        return length;
    }

    template<typename Tuple>
    ssize_t match(const Tuple &fields, const std::string &data, unsigned int offset)
    {
        if (data.size() >= offset + length) {
            value_ = data.substr(offset, length);
            return length;
        } else {
            return -1;
        }
    }

    inline void operator =(ValueType v) { this->value_ = v; }

public:
    const static size_t staticSize = length;
};

template<unsigned int field>
class ConcreteLengthStringMatcher : public Details::Matcher<std::string>
{
private:
    typedef Details::Matcher<ValueType> BaseClass;

public:
    ConcreteLengthStringMatcher() {}
    ConcreteLengthStringMatcher(const ValueType &v) : BaseClass(v) {}

    inline std::string data() const { return this->value_; }
    inline size_t size() const { return value_.size(); }

    template<typename Tuple>
    ssize_t match(const Tuple &fields, const std::string &data, unsigned int offset)
    {
        size_t length = std::get<field>(fields).cref();

        if (length > 0 && data.size() >= offset + length) {
            value_ = data.substr(offset, length);
            return value_.size();
        } else {
            return -1;
        }
    }

    inline void operator =(const ValueType &v) { this->value_ = v; }
};

template<unsigned int field>
class VariableLengthStringMatcher : public Details::Matcher<std::string>
{
private:
    typedef Details::Matcher<ValueType> BaseClass;

public:
    VariableLengthStringMatcher() {}
    VariableLengthStringMatcher(const ValueType &v) : BaseClass(v) {}

    inline std::string data() const { return this->value_; }
    inline size_t size() const { return value_.size(); }

    template<typename Tuple>
    ssize_t match(const Tuple &fields, const std::string &data, unsigned int offset)
    {
        ssize_t length = std::get<field>(fields).cref() - Details::fieldsSize(fields) + \
                         std::get<field>(fields).size();

        if (length > 0 && data.size() >= (size_t)offset + length) {
            value_ = data.substr(offset, length);
            return value_.size();
        } else {
            return -1;
        }
    }

    inline void operator =(const ValueType &v) { this->value_ = v; }
};

template<typename... Matchers>
class SimplePacket : public Packet
{
public:
    typedef std::tuple<Matchers...> TupleType;

public:
    SimplePacket()
    {
    }

    SimplePacket(const typename Matchers::ValueType &... initializers)
        : fields_(initializers...)
    {
    }

    template<typename Tuple>
    SimplePacket(const Tuple &initializers) : fields_(initializers)
    {
    }

public:
    inline std::string serialize() const
    {
        return Details::serializeFields(fields_);
    }

    inline size_t size() const
    {
        return Details::fieldsSize(fields_);
    }

    template<unsigned int i>
    inline size_t size() const
    {
        return std::get<i>(fields_).size();
    }

    template<unsigned int i> inline
    auto field() const -> const typename std::tuple_element<i, TupleType>::type::ValueType &
    {
        return std::get<i>(fields_).cref();
    }

    template<unsigned int i> inline
    void modify(const typename std::tuple_element<i, TupleType>::type::ValueType &v)
    {
        std::get<i>(fields_).ref() = v;
    }

    bool absorb(const std::string &string, size_t offset = 0)
    {
        return fieldsFromString(string, offset, fields_);
    }

    template<typename Tuple>
    inline void operator =(const Tuple &initializers)
    {
        fields_ = initializers;
    }

    //static SimplePacket<Matchers...> *fromString(const std::string &string, size_t offset = 0)
    //{
    //    std::tuple<Matchers...> fields;

    //    if (fieldsFromString(string, offset, fields))
    //        return new SimplePacket<Matchers...>(fields);
    //    else
    //        return 0;
    //}

protected:
    inline static bool fieldsFromString(const std::string &string,
                                        size_t offset,
                                        std::tuple<Matchers...> &fields)
    {
        return Details::matchString(string, fields, offset);
    }

private:
    std::tuple<Matchers...> fields_;
};

template<typename T>
struct YieldPacketSize {};

template<>
struct YieldPacketSize<SimplePacket<> >
{
    const static size_t value = 0;
};

template<typename T, typename... Args>
struct YieldPacketSize<SimplePacket<T, Args...> >
{
    const static size_t value = T::staticSize + YieldPacketSize<SimplePacket<Args...> >::value;
};

} /* namespace Net */
} /* namespace Hypergrace */

#endif /* NET_PACKETFRAMEWORK_HH_ */
