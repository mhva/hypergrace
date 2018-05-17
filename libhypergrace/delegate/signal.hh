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

#ifndef DELEGATE_SIGNAL_HH_
#define DELEGATE_SIGNAL_HH_

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>
#include <utility>

#include <delegate/delegate.hh>
#include <delegate/signalbase.hh>

namespace Hypergrace {
namespace Delegate {

template<typename... Args>
class Signal : public SignalBase
{
public:
    typedef void ResultType;
    typedef Signal<Args...> SignalType;
    typedef Delegate<ResultType (Args...)> SlotType;

private:
    struct ConnectionDescriptor
    {
        SlotType slot;
        bool *liveness;
    };

public:
    Signal() = default;
    Signal(const Signal<Args...> &) = delete;

    ~Signal()
    {
        for (auto it = connections_.begin(); it != connections_.end(); ++it) {
            ConnectionDescriptor &descriptor = *it;
            if (descriptor.liveness)
                *descriptor.liveness = false;
            else
                delete descriptor.liveness;
        }
    }

    inline void connect(ResultType (*function)(Args...))
    {
        connections_.push_back(ConnectionDescriptor { SlotType(function), new bool(true) });
    }

    template<typename Class>
    inline void connect(Class *base, ResultType (Class::*method)(Args...))
    {
        registerSlot(base, SlotType(base, method));
    }

    template<typename Class>
    inline void connect(Class *base, ResultType (Class::*method)(Args...) const)
    {
        registerSlot(base, SlotType(base, method));
    }

    template<typename Class>
    inline void connect(std::shared_ptr<Class> base, ResultType (Class::*method)(Args...))
    {
    }

    inline void operator ()(Args... args) const
    {
        // Iterating through vector by using direct addressing yields
        // almost 2 times better performance than the version that
        // involves iterators
        size_t size = connections_.size();
        register ConnectionDescriptor *descriptors = (size > 0) ? &connections_[0] : 0;

        for (register size_t i = 0; i < size; ++i) {
            // Check whether connection is still valid
            if (descriptors[i].liveness) {
                descriptors[i].slot(args...);
            } else {
                delete descriptors[i].liveness;
                connections_.erase(connections_.begin() + i);

                descriptors = &connections_[0];

                --size;
                --i;
            }
        }
    }

    void operator =(const Signal<Args...> &) = delete;

private:
    void registerSlot(Connectable *connectable, const SlotType &slot)
    {
        ConnectionDescriptor descriptor = { slot, new bool(true) };

        SignalBase::trackConnectionLiveness(connectable, descriptor.liveness);

        connections_.push_back(descriptor);
    }

private:
    mutable std::vector<ConnectionDescriptor> connections_;
};

} /* namespace Delegate */
} /* namespace Hypergrace */

#endif /* DELEGATE_SIGNAL_HH_ */
