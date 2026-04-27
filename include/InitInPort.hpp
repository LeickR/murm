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


#ifndef __MURM_INIT_INPORT_HPP__
#define __MURM_INIT_INPORT_HPP__

#include "marcmo/SRDelegate.hpp"
#include "Component.hpp"
#include "EventManager.hpp"

namespace murm {

// InitInPort class
// (can be bound to a method or connected to a more deeply nested InitInPort)
// ParamTypes are the types of data that are passed through the port
template<typename... ParamTypes>
class InitInPort {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;
    typedef generic::delegate<void(int, ParamTypes...)> IndexedDelegateType;

private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    // The delegate (for an InitInPort bound to a method)
    DelegateType del_;
    // The original delegate for an indexed InitInPort:
    IndexedDelegateType orig_del_;
    int idx_ {-1};
    
    bool connected_ {false};  // true if this is connected to an InitOutPort
    bool dangling_ {false}; // true if this is intentionally left dangling (unconnected)

public:
    // Constructor
    InitInPort(Component *owner) :
        owner_(owner),
        event_manager_(owner->getEventManager())
    {}

    // Bind this InitInPort to a method:
    //
    //    my_inport.bindToMethod(obj_ptr, &MyClass::method);
    //
    template <typename T>
    void bindToMethod(T *obj_ptr, void (T::*method)(ParamTypes...)) {
        del_ = generic::delegate<void(ParamTypes...)>::from(*obj_ptr, method);
    }

    // Make this an indexed InitInPort and bind to a method:
    //
    //    my_inport.bindToIndexedMethod(idx, obj_ptr, &MyClass::method);
    //
    template <typename T>
    void bindToIndexedMethod(int idx, T *obj_ptr,
                             void (T::*method)(int, ParamTypes...))
    {
        idx_ = idx;
        orig_del_ = generic::delegate<void(int, ParamTypes...)>::from(*obj_ptr, method);
        bindToMethod(this, &InitInPort::indexedCall);
    }

    // Adds the InitInPort index to the call parameters and invokes the original delegate:
    void indexedCall(ParamTypes... params) {
        orig_del_(idx_, params...);
    }

    // Mark if this InitInPort is intentionally left dangling (unconnected)
    void setDangling(bool dangling = true) {
        // Make sure that it's not already connected
        if (dangling) { assert(connected_ == false); }
        dangling_ = dangling;
    }
    bool isDangling() { return dangling_; }


    // Used when connecting to an InitOutPort
    // (don't call this directly, use "murm::connect(init_in_port, init_out_port);" instead)
    DelegateType &getDelegate() { return del_; }

    // Mark when this InitInPort has been connected
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

// Have to put this at the end so that both InitInPort and InitOutPort are defined for the connect function
#include "InitInPortInitOutPort_connect.hpp"

#endif // __MURM_INIT_INPORT_HPP__
