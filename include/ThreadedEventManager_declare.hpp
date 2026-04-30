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


#ifndef __MURM_THREADED_EVENT_MANAGER_DECLARE_HPP__
#define __MURM_THREADED_EVENT_MANAGER_DECLARE_HPP__

#include <map>
#include "EventManager_declare.hpp"


namespace murm {

// Forward declaration
template <typename EventT> class BetweenThreadQueue;


// A threaded event manager class.
// For convenience, a global singleton ThreadedEventManager is provided (for use as the default
// ThreadedEventManager)
//
// EventT must be a type that:
// - Has a constructor with arguments (double time, DelegateType &delegate, ParamTypes&&... params).
// - Defines the less than (<) and greater than (>) operators.
// - Has a public method called doEvent with the following signature:
//   void doEvent()
template <typename EventT = Event>
class ThreadedEventManagerType : public EventManagerType<EventT> {
private:
    using EventManagerType<EventT>::cur_time_;

    typedef std::map<EventManagerType<EventT> *, BetweenThreadQueue<EventT> *> SourceToBTQpMap;
    SourceToBTQpMap source_to_btq_map_;

    typedef std::vector<BetweenThreadQueue<EventT> *> IncomingBTQVector;
    IncomingBTQVector incoming_btq_vec_;

    typedef std::vector<BetweenThreadQueue<EventT> *> OutgoingBTQVector;
    OutgoingBTQVector outgoing_btq_vec_;

    EventGenerator<EventT, BetweenThreadQueue<EventT> *> make_btqSourceInit_event_;
    EventGenerator<EventT, BetweenThreadQueue<EventT> *> make_sendPing_event_;
    EventGenerator<EventT> make_pseudoEvent_;
    EventGenerator<EventT, BetweenThreadQueue<EventT> *> make_btqDestInit_event_;
    EventGenerator<EventT, BetweenThreadQueue<EventT> *> make_getRemoteEvents_event_;
    
    // "Connect up" a new outgoing BetweenThreadQueue
    void addOutgoingBTQ_(BetweenThreadQueue<EventT> *new_btq) {
        outgoing_btq_vec_.emplace_back(new_btq);
        
        // And set the source init Event for this BTQ
        append_init(make_btqSourceInit_event_(0ULL, new_btq));
    }

public:
    typedef EventT EventType;

    using EventManagerType<EventT>::emplace;
    using EventManagerType<EventT>::append;
    using EventManagerType<EventT>::emplace_init;
    using EventManagerType<EventT>::append_init;
    using EventManagerType<EventT>::run;
    using EventManagerType<EventT>::run_to;
    using EventManagerType<EventT>::stop;
    using EventManagerType<EventT>::init;
    using EventManagerType<EventT>::getTime;

    // Get a reference to the global singleton ThreadedEventManager
    static ThreadedEventManagerType &getDefaultThreadedEventManager() {
        // Here's the global singleton ThreadedEventManager:
        // (it's only created if used)
        static ThreadedEventManagerType global_eventmanager;

        return global_eventmanager;
    }

    // Constructor
    ThreadedEventManagerType()
    {
        make_btqSourceInit_event_.bindToMethod(this, &ThreadedEventManagerType<EventT>::btqSourceInit);
        make_sendPing_event_.bindToMethod(this, &ThreadedEventManagerType<EventT>::sendPing);
        make_pseudoEvent_.bindToMethod(this, &ThreadedEventManagerType<EventT>::pseudoEvent);
        make_btqDestInit_event_.bindToMethod(this, &ThreadedEventManagerType<EventT>::btqDestInit);
        make_getRemoteEvents_event_.bindToMethod(this, &ThreadedEventManagerType<EventT>::getRemoteEvents);
    }

    // Get the BetweenThreadQueue to here from the given source ThreadedEventManager
    // (or create it if it does not already exist)
    BetweenThreadQueue<EventT> *getBetweenThreadQueueFrom(ThreadedEventManagerType<EventT> *source_tem);

    void btqSourceInit(BetweenThreadQueue<EventT> *btq);

    // Send a "ping" pseudoevent to keep the "rolling roadblock" advancing
    void sendPing(BetweenThreadQueue<EventT> *btq);

    void pseudoEvent() {
        // Does nothing
    }

    void btqDestInit(BetweenThreadQueue<EventT> *btq);

    void getRemoteEvents(BetweenThreadQueue<EventT> *btq);

    // Reset the ThreadedEventManager back to the beginning (time = 0.0 and clear all events)
    // (Useful for testing, mostly)
    void reset() {
        EventManager::reset();
        source_to_btq_map_.clear();
        incoming_btq_vec_.clear();
        outgoing_btq_vec_.clear();
    }
};


} // namespace murm

#endif // __MURM_THREADED_EVENT_MANAGER_DECLARE_HPP__
