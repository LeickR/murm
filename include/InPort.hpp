// MIT License
//
// Copyright (c) 2026 Leick Robinson (Virtual Science Workshop)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef __MURM_INPORT_HPP__
#define __MURM_INPORT_HPP__

#include "marcmo/SRDelegate.hpp"
#include "Component.hpp"
#include "EventManager.hpp"

namespace murm {

// InPort class
// (can be bound to a method or connected to a more deeply nested InPort)
// ParamTypes are the types of data that are passed through the port
template<typename... ParamTypes>
class InPort {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;
    typedef generic::delegate<void(int, ParamTypes...)> IndexedDelegateType;

private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    // The delegate (for an InPort bound to a method)
    DelegateType del_;
    // The original delegate for an indexed InPort:
    IndexedDelegateType orig_del_;
    int idx_ {-1};

    bool connected_ {false};  // true if this is connected to an OutPort
    bool dangling_ {false}; // true if this is intentionally left dangling (unconnected)

    // Support for InPort→InPort chaining (hierarchical components)
    InPort<ParamTypes...> *chain_next_ {nullptr};
    uint64_t chain_delay_ {0};

public:
    // Constructor
    InPort(Component *owner) :
        owner_(owner),
        event_manager_(owner->getEventManager())
    {}

    // Copy constructor
    InPort(const InPort&) = default;

    EventManager *getEventManager() { return event_manager_; }

    // Bind this InPort to a method:
    //
    //    my_inport.bindToMethod(obj_ptr, &MyClass::method);
    //
    template <typename T>
    void bindToMethod(T *obj_ptr, void (T::*method)(ParamTypes...)) {
        del_ = generic::delegate<void(ParamTypes...)>::from(*obj_ptr, method);
    }

    // Make this an indexed InPort and bind to a method:
    //
    //    my_inport.bindToIndexedMethod(idx, obj_ptr, &MyClass::method);
    //
    //void setDelegateAndIndex(int idx, const IndexedDelegateType &new_del) {
    //   idx_ = idx;
    //   orig_del_ = new_del;
    //   BIND_TO_METHOD(this, InPort::indexedCall);
    //}

     template <typename T>
     void bindToIndexedMethod(int idx, T *obj_ptr,
                                      void (T::*method)(int, ParamTypes...))
     {
          idx_ = idx;
          orig_del_ = generic::delegate<void(int, ParamTypes...)>::from(*obj_ptr, method);
          bindToMethod(this, InPort::indexedCall);
     }

    // Adds the InPort index to the call parameters and invokes the original delegate:
    void indexedCall(ParamTypes... params) {
        orig_del_(idx_, params...);
    }

    // Mark if this InPort is intentionally left dangling (unconnected)
    void setDangling(bool dangling = true) {
        // Make sure that it's not already connected
        if (dangling) { assert(connected_ == false); }
        dangling_ = dangling;
    }
    bool isDangling() { return dangling_; }


    // Used when connecting to an OutPort
    // (don't call this directly, use "murm::connect(in_port, out_port);" instead)
    DelegateType &getDelegate() { return del_; }

    // Used for InPort→InPort chaining (hierarchical components)
    // (don't call directly, use "murm::connect(inport_a, inport_b);" instead)
    void setChainNext(InPort<ParamTypes...> *next, uint64_t delay) {
        assert(chain_next_ == nullptr);
        chain_next_ = next;
        chain_delay_ = delay;
    }

    // Follow the chain to find the ultimate InPort (the one bound to a method)
    // and accumulate the total chain delay.
    InPort<ParamTypes...> &getChainEnd(uint64_t &accumulated_delay) {
        accumulated_delay = 0;
        InPort<ParamTypes...> *current = this;
        while (current->chain_next_ != nullptr) {
            accumulated_delay += current->chain_delay_;
            current = current->chain_next_;
        }
        return *current;
    }

    // Mark all InPorts in the chain (from this one to the end) as connected.
    void markChainConnected() {
        InPort<ParamTypes...> *current = this;
        while (current != nullptr) {
            current->setConnected();
            current = current->chain_next_;
        }
    }

    // Mark when this InPort has been connected
    void setConnected(bool connected = true) {
        // Make sure that it's not already connected or set as dangling
        if (connected) {
            assert(connected_ == false);
            assert(dangling_ == false);
        }
        connected_ = connected;
    }
    bool isConnected() { return connected_; }
};

} // namespace murm

// Have to put this at the end so that both InPort and OutPort are defined for the connect function
// (and InPort is defined for the OutPort).
//#include "InPortOutPort_connect.hpp"
#include "OutPort.hpp"

#endif // __MURM_INPORT_HPP__
