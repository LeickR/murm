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


#ifndef __MURM_EVENT_MANAGER_DECLARE_HPP__
#define __MURM_EVENT_MANAGER_DECLARE_HPP__

#include <queue>
#include <sstream>
#include "Event.hpp"
#include "EventGenerator.hpp"

namespace murm {

// Forward declarations
template <typename EventT> class EventManagerType;
//template <typename EventT> class ThreadedEventManagerType;
//template <typename EventT> 
//static ThreadedEventManagerType<EventT>& ThreadedEventManagerType<EventT>::getDefaultThreadedEventManager();

typedef EventManagerType<Event> EventManager;

// The main event manager class.
// For convenience, a global singleton EventManager is provided (for use as the default
// EventManager)
//
// EventT must be a type that:
// - Has a constructor with arguments (double time, DelegateType &delegate, ParamTypes&&... params).
// - Defines the less than (<) and greater than (>) operators.
// - Has a public method called doEvent with the following signature:
//   void doEvent()
template <typename EventT = Event>
class EventManagerType {
protected:
    // EventQueue (the earliest event will be at the top)
    typedef std::priority_queue<EventT, std::vector<EventT>, std::greater<EventT>> EventQueue;

    // FIFO queue for initialization events
    typedef std::queue<EventT> InitQueue;
    
    uint64_t cur_time_ {0};
    bool stopping_ {false};
    EventQueue equeue_;
    InitQueue init_queue_;
    
    EventGenerator<EventT> make_stop_event_;

    // The max number of events at the same time that are allowed (to catch infinite loops)
    static const uint64_t MAX_SIMULTANEOUS_EVENTS = 1000000;

    uint64_t simultaneous_event_count_ {0};

    // Used for debugging:
    int event_manager_num_ {0};
    static int getAndIncrementEventManagerNum_() {
        static int next_event_manager_num = 0;
        
        int evnum = next_event_manager_num;
        ++next_event_manager_num;
        return evnum;
    }

public:
    typedef EventT EventType;

    // Get a reference to the global singleton EventManager
    // (The underlying object is actually a ThreadedEventManager which can be used in either
    // threaded or unthreaded simulations)
    static EventManagerType& getDefaultEventManager();

    // Constructor
    EventManagerType()
    {
        event_manager_num_ = getAndIncrementEventManagerNum_();
#ifdef DEBUG_EVENTMANAGER
        std::cout << "Constructing EventManager " << event_manager_num_ << " (in debug mode)" << std::endl;
#endif
        make_stop_event_.bindToMethod(this, &EventManagerType<EventT>::stop);
    }

    // Destructor
    virtual ~EventManagerType() {}

    // Emplace construction of a new Event
    template <typename DelegateType, typename... ParamTypes>
    void emplace(uint64_t t, DelegateType &delegate, ParamTypes&&... params)
    {
        //std::cout << "emplace():  Emplace new event, equeue_.size() before = " << equeue_.size() << ", equeue_.empty() = " << equeue_.empty() << std::endl;
        equeue_.emplace(t, delegate, std::forward<ParamTypes>(params)...);
        //std::cout << "emplace():                     equeue_.size() after  = " << equeue_.size() << ", equeue_.empty() = " << equeue_.empty() << std::endl;
    }

    // Emplace construction of a new Event with specified ordering
    template <typename DelegateType, typename... ParamTypes>
    void emplace(uint32_t order, uint64_t t, DelegateType &delegate, ParamTypes&&... params)
    {
        EventT event(t, delegate, std::forward<ParamTypes>(params)...);
        event.setOrder(order);
        equeue_.push(std::move(event));
    }

    // Append a new Event
    template <typename EventU>
    void append(EventU&& event)
    {
        //std::cout << "emplace():  Append new event, equeue_.size() before = " << equeue_.size() << ", equeue_.empty() = " << equeue_.empty() << std::endl;
        equeue_.push(std::forward<EventU>(event));
        //std::cout << "emplace():                    equeue_.size() after  = " << equeue_.size() << ", equeue_.empty() = " << equeue_.empty() << std::endl;
    }

    // Emplace construction of a new initialization Event
    template <typename DelegateType, typename... ParamTypes>
    void emplace_init(DelegateType &delegate, ParamTypes&&... params)
    {
        // For init Events, the "time" is ignored.  Init Events are processed in FIFO order.
        init_queue_.emplace(0ULL, delegate, std::forward<ParamTypes>(params)...);
    }

    // Append a new initialization Event
    template <typename EventU>
    void append_init(EventU&& event)
    {
        init_queue_.push(std::forward<EventU>(event));
    }

    // Run until there are no more events or we are stopped
    void run() {
        //std::cout << "run():  equeue_.size() = " << equeue_.size() << ", equeue_.empty() = " << equeue_.empty() << std::endl;
        while ((equeue_.empty() == false) && (stopping_ == false)) {
            //std::cout << "run():  Get next event..." << std::endl;
            // const_casts are ugly, but we need a non-const reference to the next event
            //EventT &event = const_cast<EventT&>(equeue_.top());

            // Unfortunately, we have to make a copy and pop the event now.  Otherwise, new events
            // might be scheduled that could change the ordering before the event is popped.
            EventT event = equeue_.top();
            //std::cout << "run():  Pop next event..., equeue_.size() before = " << equeue_.size() << ", equeue_.empty() = " << equeue_.empty() << std::endl;
            equeue_.pop();
            //std::cout << "run():                     equeue_.size() after  = " << equeue_.size() << ", equeue_.empty() = " << equeue_.empty() << std::endl;
            
            // assert(cur_time_ <= event.getTime());
            //std::cout << "run():  Execute next event..." << std::endl;
            uint64_t event_time = event.getTime();
#ifdef DEBUG_EVENTMANAGER
            std::stringstream ss;
            ss << "EventManager " << event_manager_num_ << ":  processing event at time " << event_time << "\n";
            std::cout << ss.str() << std::flush;
            if (cur_time_ == event_time) {
                ++simultaneous_event_count_;
                if (simultaneous_event_count_ > MAX_SIMULTANEOUS_EVENTS) {
                    std::cout << "EventManager " << event_manager_num_
                                 << ":  MAX_SIMULTANEOUS_EVENTS exceeded.  EventManager may be in an infinite loop at time "
                                 << cur_time_ << std::endl;
                    assert(0);
                }
            } else {
                simultaneous_event_count_ = 0;
            }
#endif
            cur_time_ = event_time;
            event.doEvent();
            //equeue_.pop();
        }
        stopping_ = false;
        //std::cout << "run() finished" << std::endl;
    }

    // Stop the simulation (set "stopping_" to true)
    void stop() {
        stopping_ = true;
    }

    //void foo(int dummy) {
    //   std::cout << "Called foo" << std::endl;
    //}

    // Run up to and including the specified time
    void run_to(uint64_t stop_time) {
        EventT stop_event = make_stop_event_(stop_time);
        stop_event.setLast();
        append(stop_event);
        run();
    }

    // Execute all of the initialization events in FIFO order
    // Returns the number of init events executed
    uint64_t init() {
        uint64_t count = 0;
        while (init_queue_.empty() == false) {
            EventT event = init_queue_.front();
            event.doEvent();
            init_queue_.pop();
            ++count;
        }
        return count;
    }

    // Get the current time
    uint64_t getTime() {
        return cur_time_;
    }

    // Reset the EventManager back to the beginning (time = 0.0 and clear all events)
    // (Useful for testing, mostly)
    void reset() {
        cur_time_ = 0;
        stopping_ = false;
        while (equeue_.empty() == false) {
            equeue_.pop();
        }
        while (init_queue_.empty() == false) {
            init_queue_.pop();
        }
    }

    // Get the EventManager number (used for debugging)
    int getEventManagerNum() { return event_manager_num_; }
};

} // namespace murm


#endif // __MURM_EVENT_MANAGER_DECLARE_HPP__
