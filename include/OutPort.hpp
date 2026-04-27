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


#ifndef __MURM_OUTPORT_HPP__
#define __MURM_OUTPORT_HPP__

#include <cassert>
#include "marcmo/SRDelegate.hpp"
#include "Component.hpp"
#include "EventManager.hpp"
#include "InPort.hpp"
#include "connect.hpp"
#include "BetweenThreadQueue.hpp"

namespace murm {

// OutPort class
// (can be connected to an InPort or connected to a more deeply nested OutPort)
// ParamTypes are the types of data that are passed through the port
template<typename... ParamTypes>
class OutPort {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;
    //typedef generic::delegate<void(double, DelegateType&, ParamTypes...)> EventCreatorType;

private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    BetweenThreadQueue<EventManager::EventType> *btqueue_ {nullptr};
    bool remote_ {false};  // True if the InPort on the "other end" belongs to a different EventManager
    
    // The delegate (copied over when the chain of connection ends in an InPort bound to a method)
    DelegateType del_;
    uint64_t delta_time_ {0}; // Total delay of the link (after connected to an InPort)

    //EventCreatorType event_creator_;

    bool connected_ {false};  // true if this is connected to an InPort
    bool dangling_ {false}; // true if this is intentionally left dangling (unconnected)

    // Support for OutPort→OutPort chaining (hierarchical components)
    OutPort<ParamTypes...> *chained_outport_ {nullptr};
    uint64_t chain_delay_ {0};

    // Internal connection helper — configures this OutPort and recurses through chained OutPorts,
    // but does NOT mark the InPort as connected (that is done once by the public connectToInPort).
    void connectToInPortInternal_(InPort<ParamTypes...> &inport, uint64_t delay) {
        assert(isDangling() == false);
        assert(isConnected() == false);

        setDelegate(inport.getDelegate());
        setDeltaTime(delay);

        if (event_manager_ == inport.getEventManager()) {
            // Same EventManager — no extra setup needed
        } else {
            remote_ = true;
            ThreadedEventManager *source_tem = dynamic_cast<ThreadedEventManager *>(event_manager_);
            ThreadedEventManager *dest_tem = dynamic_cast<ThreadedEventManager *>(inport.getEventManager());
            btqueue_ = dest_tem->getBetweenThreadQueueFrom(source_tem);
            btqueue_->updateDelay(delay);
        }

        setConnected();

        if (chained_outport_) {
            chained_outport_->connectToInPortInternal_(inport, delay + chain_delay_);
        }
    }

public:
    // Constructor
    OutPort(Component *owner) :
        owner_(owner),
        event_manager_(owner->getEventManager())
    {}

    // Copy constructor
    OutPort(const OutPort&) = default;

    EventManager *getEventManager() { return event_manager_; }

    // The OutPort is a functor:
    void operator()(ParamTypes... params) {
        assert(connected_ == true);
        uint64_t new_time = event_manager_->getTime() + delta_time_;
        // Create an event (Event(new_time, del_, params...)) and enqueue it to the EventHandler or BetweenThreadQueue
        //event_creator_(new_time, del_, std::forward<ParamTypes>(params)...);
        if (remote_) {
            btqueue_->emplace(new_time, del_, std::forward<ParamTypes>(params)...);
        } else {
            event_manager_->emplace(new_time, del_, std::forward<ParamTypes>(params)...);
        }
    }

    // Mark if this OutPort is intentionally left dangling (unconnected)
    void setDangling(bool dangling = true) {
        // Make sure that it's not already connected
        if (dangling) { assert(connected_ == false); }
        dangling_ = dangling;
    }
    bool isDangling() { return dangling_; }

    // Used when connecting to an InPort
    // (don't call these directly, use "murm::connect(in_port, out_port);" instead)
    void setDelegate(const DelegateType &new_del) { del_ = new_del; }
    void setDeltaTime(uint64_t total_delta) { delta_time_ = total_delta; }

    void connectToInPort(InPort<ParamTypes...> &inport, uint64_t delay) {
        assert(inport.isDangling() == false);
        assert(isDangling() == false);
        assert(inport.isConnected() == false);
        assert(isConnected() == false);

        // Follow the InPort chain to find the ultimate bound InPort,
        // accumulating any additional delay along the way.
        uint64_t inport_chain_delay = 0;
        InPort<ParamTypes...> &effective_inport = inport.getChainEnd(inport_chain_delay);
        uint64_t total_delay = delay + inport_chain_delay;

        connectToInPortInternal_(effective_inport, total_delay);

        // Mark all InPorts in the chain (including inport) as connected.
        inport.markChainConnected();
    }

    // Used for OutPort→OutPort chaining (hierarchical components)
    // (don't call directly, use "murm::connect(inner_outport, outer_outport);" instead)
    void setChainedOutPort(OutPort<ParamTypes...> *chained, uint64_t chain_delay) {
        assert(chained_outport_ == nullptr);
        chained_outport_ = chained;
        chain_delay_ = chain_delay;
    }
    
    // Mark when this OutPort has been connected
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
#include "InPortOutPort_connect.hpp"

#endif // __MURM_OUTPORT_HPP__
