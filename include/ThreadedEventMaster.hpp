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


#ifndef __MURM_THREADED_EVENT_MASTER_HPP__
#define __MURM_THREADED_EVENT_MASTER_HPP__

#include "ThreadedEventManager.hpp"
#include "omp.h"


namespace murm {


// Forward declaration
template <typename EventT>
class ThreadedEventMasterType;

typedef ThreadedEventMasterType<Event> ThreadedEventMaster;


// The threaded event master takes care of managing the individual threads and the
// corresponding EventManagers.
template <typename EventT = Event>
class ThreadedEventMasterType {
private:
    int num_threads_ {1};
    std::vector<ThreadedEventManagerType<EventT> *> event_managers_;

public:
    // Get a reference to the global singleton ThreadedEventMaster
    static ThreadedEventMasterType &getDefaultThreadedEventMaster() {
        // Here's the global singleton ThreadedEventMaster:
        // (it's only created if used)
        static ThreadedEventMasterType global_eventmaster;

        return global_eventmaster;
    }

    // Constructor
    ThreadedEventMasterType() = default;

    // Destructor
    ~ThreadedEventMasterType() {
        for (int i = 1; i < event_managers_.size(); ++i) {
            delete event_managers_[i];
        }
    }

    int getMaxNumThreads() const { return omp_get_max_threads(); }
    
    void createThreads(int num_threads = omp_get_max_threads()) {
        assert(num_threads <= omp_get_max_threads());
        num_threads_ = num_threads;
        
        omp_set_num_threads(num_threads_);
        
        // Create the EventManagers (one per thread)
        // (use the default EventManager for thread 0)
        event_managers_.push_back(&ThreadedEventManagerType<EventT>::getDefaultThreadedEventManager());
        for (int i = 1; i < num_threads_; ++i) {
            event_managers_.push_back(new ThreadedEventManagerType<EventT>);
        }
    }

    int getNumThreads() { return num_threads_; }

    ThreadedEventManagerType<EventT> *getEventManagerForThread(int thread_num) {
        assert(thread_num < num_threads_);
        return event_managers_[thread_num];
    }

    // Initialize all EventManagers
    void init() {
        bool done = false;
        while (done == false) {
            uint64_t total_count = 0;
            for (auto event_manager : event_managers_) {
                total_count += event_manager->init();
            }
            done = (total_count == 0);
        }
    }

    // Run all EventManagers on separate threads up to and including the specified time
    void run_to(uint64_t stop_time) {
        #pragma omp parallel for
        for (int j=0; j<num_threads_; ++j) {
            int thread_num = omp_get_thread_num();
            event_managers_[thread_num]->run_to(stop_time);
        }
    }
};


} // namespace murm

#endif // __MURM_THREADED_EVENT_MASTER_HPP__
