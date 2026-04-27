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


#ifndef __MURM_BETWEEN_THREAD_QUEUE_HPP__
#define __MURM_BETWEEN_THREAD_QUEUE_HPP__

#include <cassert>
#include "ThreadedEventManager_declare.hpp"
#include "BetweenThreadQueue_declare.hpp"

// Include after declarations
#include "ThreadedEventManager.hpp"


namespace murm {


// Emplace construction of a new Event
// NOTE:  this should only be called by the source EventManager
template <typename EventT>
template <typename DelegateType, typename... ParamTypes>
void BetweenThreadQueue<EventT>::emplace(uint64_t t, DelegateType &delegate, ParamTypes&&... params)
{
    uint64_t cur_time = source_tem_->getTime();
    // Grab a copy of the write pointer (memory ordering is relaxed because the source EM
    // "owns" this pointer - it's the only one allowed to write to it).
    EventPkg *next_to_write = next_to_write_.load(std::memory_order_relaxed);
    EventPkg *next_next_to_write = nextPtr_(next_to_write);
    // Grab a copy of the read pointer
    // (to avoid unintentional overwrite of data still being read by the dest EM, the memory
    // fence ensures that we'll get the current pointer before any of the writes below)
    EventPkg *next_to_read = next_to_read_.load(std::memory_order_acquire);
    while (next_next_to_write == next_to_read) {
        // The queue is full, spin until there's room
        next_to_read = next_to_read_.load(std::memory_order_acquire);
    }

    //std::cout << "BetweenThreadQueue from " << (void *)(source_tem_) << " to " << (void *)(dest_tem_)
    //          << ", EventPkg next_to_write = " << (void *)(next_to_write) << std::endl;

    // Write the event (plus the current time)
    new (next_to_write) EventPkg(cur_time, t, delegate, std::forward<ParamTypes>(params)...);
    // And, finally, update the pointer
    // (the memory fence ensures that all of the writes above will happen first)
    next_to_write_.store(next_next_to_write, std::memory_order_release);
}
    
// Append a new Event
// NOTE:  this should only be called by the source EventManager
template <typename EventT>
template <typename EventU>
void BetweenThreadQueue<EventT>::append(EventU&& event)
{
    uint64_t cur_time = source_tem_->getTime();
    // Grab a copy of the write pointer (memory ordering is relaxed because the source EM
    // "owns" this pointer - it's the only one allowed to write to it).
    EventPkg *next_to_write = next_to_write_.load(std::memory_order_relaxed);
    EventPkg *next_next_to_write = nextPtr_(next_to_write);
    // Grab a copy of the read pointer
    // (to avoid unintentional overwrite of data still being read by the dest EM, the memory
    // fence ensures that we'll get the current pointer before any of the writes below)
    EventPkg *next_to_read = next_to_read_.load(std::memory_order_acquire);
    while (next_next_to_write == next_to_read) {
        // The queue is full, spin until there's room
        next_to_read = next_to_read_.load(std::memory_order_acquire);
    }
    // Write the event (plus the current time)
    new (next_to_write) EventPkg(cur_time, std::forward<EventU>(event));
    // And, finally, update the pointer
    // (the memory fence ensures that all of the writes above will happen first)
    next_to_write_.store(next_next_to_write, std::memory_order_release);
}

// Get new Events
// NOTE:  this should only be called by the dest EventManager
template <typename EventT>
void BetweenThreadQueue<EventT>::moveEventsToDest()
{
    // Grab a copy of the read pointer (memory ordering is relaxed because the dest EM
    // "owns" this pointer - it's the only one allowed to write to it).
    EventPkg *next_to_read = next_to_read_.load(std::memory_order_relaxed);
    // Grab a copy of the write pointer
    // (the memory fence ensures that we won't try to read a partially written Event)
    EventPkg *next_to_write = next_to_write_.load(std::memory_order_acquire);
    if (next_to_read != next_to_write) {
        uint64_t latest_send_time = 0;
        while (next_to_read != next_to_write) {
            dest_tem_->append(next_to_read->event);
            latest_send_time = next_to_read->send_time;
            next_to_read = nextPtr_(next_to_read);
        }
        // If we've moved events to the dest EventManager, update the read pointer
        // (the memory fence ensures that the events won't get overwritten before we've finished
        // reading them)
        next_to_read_.store(next_to_read, std::memory_order_release);

        // Now, update the dest EventManager's "rolling roadblock" (for this BTQ)
        uint64_t new_roadblock = latest_send_time + time_window_size_;
        assert(new_roadblock >= current_roadblock_time_);

        if (save_roadblock_data_) {
            uint64_t srctime = source_tem_->getTime();
            uint64_t dsttime = dest_tem_->getTime();
            road_block_data_.emplace_back(srctime, dsttime,
                (int64_t)current_roadblock_time_ - (int64_t)dsttime,
                (int64_t)new_roadblock - (int64_t)dsttime);
        }
        
        current_roadblock_time_ = new_roadblock;
    }
}


} // namespace murm

#endif // __MURM_BETWEEN_THREAD_QUEUE_HPP__
