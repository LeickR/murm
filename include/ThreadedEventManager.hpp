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


#ifndef __MURM_THREADED_EVENT_MANAGER_HPP__
#define __MURM_THREADED_EVENT_MANAGER_HPP__

#include <map>
#include "EventManager_declare.hpp"
#include "ThreadedEventManager_declare.hpp"
#include "BetweenThreadQueue_declare.hpp"

// Include after declarations
#include "EventManager.hpp"
#include "BetweenThreadQueue.hpp"


namespace murm {

// Forward declaration
//template <typename EventT>
//class ThreadedEventManagerType;

typedef ThreadedEventManagerType<Event> ThreadedEventManager;


// Get the BetweenThreadQueue to here from the given source ThreadedEventManager
// (or create it if it does not already exist)
template <typename EventT>
BetweenThreadQueue<EventT> *ThreadedEventManagerType<EventT>::
getBetweenThreadQueueFrom(ThreadedEventManagerType<EventT> *source_tem)
{
    // Check and see if we need to create the BetweenThreadQueue
    if (source_to_btq_map_.count(source_tem) == 0) {
        // Create the new BTQ
        BetweenThreadQueue<EventT> *new_btq = new BetweenThreadQueue<EventT>(source_tem, this);
        incoming_btq_vec_.push_back(new_btq);
        source_to_btq_map_[source_tem] = new_btq;
        source_tem->addOutgoingBTQ_(new_btq);
        
        // And set the dest init Event for this BTQ
        append_init(make_btqDestInit_event_(0ULL, new_btq));
    }
    return source_to_btq_map_[source_tem];
}

template <typename EventT>
void ThreadedEventManagerType<EventT>::btqSourceInit(BetweenThreadQueue<EventT> *btq)
{
    // Set initial "roadblock"
    btq->setRoadblock(cur_time_ + btq->getTimeWindowSize());
    
    // Schedule the first "sendPing" Event at (time_delay)/8
    uint64_t first_ping_time = cur_time_ + btq->getTimeWindowSize()/8;
    append(make_sendPing_event_(first_ping_time, btq));
}

// Send a "ping" pseudoevent to keep the "rolling roadblock" advancing
template <typename EventT>
void ThreadedEventManagerType<EventT>::sendPing(BetweenThreadQueue<EventT> *btq) {
    // Send the pseudoevent
    btq->append(make_pseudoEvent_(cur_time_ + btq->getMinLinkDelay()));
    
    // Schedule the next "sendPing" Event
    uint64_t next_ping_time = cur_time_ + btq->getTimeWindowSize()/8;
    append(make_sendPing_event_(next_ping_time, btq));
}

template <typename EventT>
void ThreadedEventManagerType<EventT>::btqDestInit(BetweenThreadQueue<EventT> *btq)
{
    // Schedule first "getRemoteEvents" event at (time_delay)/2 [set as "last event"]
    uint64_t first_get_time = cur_time_ + btq->getTimeWindowSize()/2;
    EventT get_remote_events_event = make_getRemoteEvents_event_(first_get_time, btq);
    get_remote_events_event.setLast();
    append(get_remote_events_event);
}

template <typename EventT>
void ThreadedEventManagerType<EventT>::getRemoteEvents(BetweenThreadQueue<EventT> *btq)
{
    // Get any new events from this BTQ
    btq->moveEventsToDest();
    
    // Schedule next "getRemoteEvents" event, either (time_delay)/2 from now, or at the
    // "roadblock", whichever is sooner
    
    // [Note that if we're far enough ahead of the other thread that we're already at
    // the "roadblock", this will cause us to essentially "spin" at the roadblock until
    // new (remote) events show up.]
    uint64_t next_get_time = std::min(cur_time_ + btq->getTimeWindowSize()/2,
                                               btq->getRoadblockTime());
    EventT get_remote_events_event = make_getRemoteEvents_event_(next_get_time, btq);
    get_remote_events_event.setLast();
    append(get_remote_events_event);
}

} // namespace murm

#endif // __MURM_THREADED_EVENT_MANAGER_HPP__
