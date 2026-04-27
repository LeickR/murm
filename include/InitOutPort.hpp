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


#ifndef __MURM_INIT_OUTPORT_HPP__
#define __MURM_INIT_OUTPORT_HPP__

#include <cassert>
#include "marcmo/SRDelegate.hpp"
#include "Component.hpp"
#include "EventManager.hpp"
#include "connect.hpp"

namespace murm {

// InitOutPort class
// (can be connected to an InitInPort or connected to a more deeply nested InitOutPort)
// ParamTypes are the types of data that are passed through the port
template<typename... ParamTypes>
class InitOutPort {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;

private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    // The delegate (copied over when the chain of connection ends in an InitInPort bound to a method)
    DelegateType del_;

    bool immediate_ {false}; // If true, the call via the delegate will be made immediately
                                     // instead of through the EventManager's init queue

    bool connected_ {false}; // true if this is connected to an InitInPort
    bool dangling_ {false}; // true if this is intentionally left dangling (unconnected)

public:
    // Constructor
    InitOutPort(Component *owner, bool immediate = false) :
        owner_(owner),
        event_manager_(owner->getEventManager()),
        immediate_(immediate)
    {}

    // The InitOutPort is a functor:
    void operator()(ParamTypes... params) {
        assert(connected_ == true);
        if (immediate_) {
            // Make the call immediately
            del_(params...);
        } else {
            // Create an event (Event(0.0, del_, params...)) and enqueue it to the EventHandler
            event_manager_->emplace_init(del_, std::forward<ParamTypes>(params)...);
        }
    }

    // Mark if this InitOutPort is intentionally left dangling (unconnected)
    void setDangling(bool dangling = true) {
        // Make sure that it's not already connected
        if (dangling) { assert(connected_ == false); }
        dangling_ = dangling;
    }
    bool isDangling() { return dangling_; }

    // Used when connecting to an InitInPort
    // (don't call these directly, use "murm::connect(in_port, out_port);" instead)
    void setDelegate(const DelegateType &new_del) { del_ = new_del; }

    // Mark when this InitOutPort has been connected
    void setConnected(bool connected = true) {
        // Make sure that it's not already connected or set as dangling
        if (connected) {
            assert(connected_ == false);
            assert(dangling_ == false);
        }
        connected_ = connected;
    }
    bool isConnected() { return connected_; }

    // Set whether this is an immediate connection
    void setImmediate(bool immediate = true) {
        immediate_ = immediate;
    }
    bool isImmediate() { return immediate_; }
};

} // namespace murm

// Have to put this at the end so that both InitInPort and InitOutPort are defined for the connect function
#include "InitInPortInitOutPort_connect.hpp"

#endif // __MURM_INIT_OUTPORT_HPP__
