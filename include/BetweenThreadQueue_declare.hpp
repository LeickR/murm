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


#ifndef __MURM_BETWEEN_THREAD_QUEUE_DECLARE_HPP__
#define __MURM_BETWEEN_THREAD_QUEUE_DECLARE_HPP__

#include <atomic>
#include <array>
#include <vector>
#include "constants.hpp"

namespace murm {

// Forward declaration
template <typename EventT> class ThreadedEventManagerType;


// The Event queue "bridging" between EventManagers for different threads
template <typename EventT>
class BetweenThreadQueue
{
public:
    struct RoadblockDataPoint {
        uint64_t dest_time;
        int64_t src_lead; // (src_time - dest_time)
        int64_t old_rb_delta;
        int64_t new_rb_delta;

        RoadblockDataPoint(uint64_t st, uint64_t dt, int64_t old_del, int64_t new_del) :
            dest_time(dt),
            src_lead((int64_t)st - (int64_t)dt),
            old_rb_delta(old_del),
            new_rb_delta(new_del)
        {}
    };

private:
    // The Event plus the "send" time
    struct EventPkg {
        EventT event;
        uint64_t send_time {0};

        // Constructors
        EventPkg() {}

        template <typename EventU>
        EventPkg(uint64_t stime, EventU&& new_event) :
            event(std::forward<EventU>(new_event)),
            send_time(stime)
        {}

        template <typename DelegateType, typename... ParamTypes>
        EventPkg(uint64_t stime, uint64_t event_time, DelegateType &delegate, ParamTypes&&... params) :
            event(event_time, delegate, std::forward<ParamTypes>(params)...),
            send_time(stime)
        {}
    };

    std::atomic<EventPkg*> next_to_write_ {nullptr}; // Only updated by the source EM
    std::atomic<EventPkg*> next_to_read_ {nullptr};  // Only updated by the dest EM
    ThreadedEventManagerType<EventT> *source_tem_;
    ThreadedEventManagerType<EventT> *dest_tem_;
    int source_tem_num_;
    int dest_tem_num_;

    uint64_t min_link_delay_ {0};
    uint64_t time_window_size_ {0};
    uint64_t current_roadblock_time_ {0};
    bool link_delay_set_ {false};

    bool save_roadblock_data_ {false};

    std::vector<RoadblockDataPoint> road_block_data_;

    std::array<EventPkg, BTQ_SIZE> circular_event_queue_;

    // Get the next Event(Pkg) in the circular queue
    EventPkg *nextPtr_(EventPkg *event) {
        return (event == &(circular_event_queue_.back())) ?
            (&(circular_event_queue_.front())) : (++event);
    }

public:
    // Constructor
    BetweenThreadQueue(ThreadedEventManagerType<EventT> *source_tem,
                             ThreadedEventManagerType<EventT> *dest_tem) :
        source_tem_(source_tem),
        dest_tem_(dest_tem)
    {
        source_tem_num_ = source_tem->getEventManagerNum();
        dest_tem_num_ = dest_tem->getEventManagerNum();
#ifdef DEBUG_EVENTMANAGER
        std::cout << "Created BTQ[" << source_tem_num_ << "->" << dest_tem_num_ << "]" << std::endl;
#endif

        next_to_write_.store(&(circular_event_queue_.front()), std::memory_order_relaxed);
        next_to_read_.store(&(circular_event_queue_.front()), std::memory_order_relaxed);
        
        getBTQVector().push_back(this);
    }

    // Update the minimum link delay between the EventManagers
    // along with the time window size for the "rolling roadblock"
    // (to avoid roundoff errors, the time window size is slightly less than the minimum link delay)
    void updateDelay(uint64_t delay) {
        if (link_delay_set_) {
            if (delay < min_link_delay_) {
                min_link_delay_ = delay;
                time_window_size_ = delay;
            }
        } else {
            min_link_delay_ = delay;
            time_window_size_ = delay;
            link_delay_set_ = true;
        }
    }

    // Emplace construction of a new Event
    // NOTE:  this should only be called by the source EventManager
    template <typename DelegateType, typename... ParamTypes>
    void emplace(uint64_t t, DelegateType &delegate, ParamTypes&&... params);

    // Append a new Event
    // NOTE:  this should only be called by the source EventManager
    template <typename EventU>
    void append(EventU&& event);

    // Get new Events
    // NOTE:  this should only be called by the dest EventManager
    void moveEventsToDest();

    void setRoadblock(uint64_t rb_time) { current_roadblock_time_ = rb_time; }
    uint64_t getRoadblockTime() { return current_roadblock_time_; }

    uint64_t getTimeWindowSize() { return time_window_size_; }
    uint64_t getMinLinkDelay() { return min_link_delay_; }

    ThreadedEventManagerType<EventT> *getSourceTEM() { return source_tem_; }
    ThreadedEventManagerType<EventT> *getDestTEM() { return dest_tem_; }

    
    void setSaveRoadblockData(bool save = true) {
        save_roadblock_data_ = save;
    }

    std::vector<RoadblockDataPoint> &getRoadBlockData() {
        return road_block_data_;
    }

    // Return a vector of all BTQs
    // (Useful for debugging)
    static std::vector<BetweenThreadQueue *> &getBTQVector() {
        static std::vector<BetweenThreadQueue *> btq_vec;
        return btq_vec;
    }
};


} // namespace murm

#endif // __MURM_BETWEEN_THREAD_QUEUE_DECLARE_HPP__
