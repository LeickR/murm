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


#ifndef __MURM_EVENT_HPP__
#define __MURM_EVENT_HPP__

#ifndef MURM_EVENT_MAX_LOCAL_DATA_SIZE_BYTES
#define MURM_EVENT_MAX_LOCAL_DATA_SIZE_BYTES  32
#endif


#include <array>
#include <tuple>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <type_traits>
#include "marcmo/SRDelegate.hpp"

namespace murm {


// Convenient constants used to order Events scheduled at the same time.
// (e.g., used to implement "tick, update" scheduling)
const int32_t EVENT_FIRST = INT32_MIN;
const int32_t EVENT_NEXT_TO_FIRST = INT32_MIN + 1;
const int32_t EVENT_NEXT_TO_LAST = INT32_MAX - 1;
const int32_t EVENT_LAST = INT32_MAX;


// Event class
// max_local_bytes defines the largest tuples that are stored locally (rather than on the heap)
template <size_t max_local_bytes = 32>
class EventType {
private:
    typedef generic::delegate<void()> v2vDelegate;
    
    // Borrowed the idea for the Executors and Deleters from the Impossibly Fast Delegate
    typedef void (*PointerToExecutor)(v2vDelegate &d, void *hdata, void *ldata);
    PointerToExecutor fpEventExecutor_;
    
    typedef void (*PointerToParamDeleter)(void *, void *);
    PointerToParamDeleter fpParamDeleter_;
    
    v2vDelegate del_;           // the delegate (cast to v2vDelegate for storage)
    uint64_t time_ {0};         // the scheduled time for the event
    void *heap_data_ {nullptr}; // pointer to large tuple on the heap (null if local)
    int32_t order_ {0};         // ordering for events at the same time
    
    static const size_t max_local_uint64s = (max_local_bytes + 7)/8;
    std::array<uint64_t, max_local_uint64s> data_;  // local tuple buffer
    
    
    // Helper classes for converting tuple to parameter pack
    // Indices template class
    template <std::size_t... indices>
    struct Indices {
        static void printMe() {
            int dummy[sizeof...(indices)] = { (std::cout << indices << " ", 0)... };
            std::cout << std::endl;
        }
    };
    
    // Helper template declaration
    template <std::size_t next_index, typename IndicesT, typename... Types>
    struct TypesToIndicesHelper;
    
    // If there are no types left, terminate the recursion
    template <std::size_t next_index, std::size_t... indices>
    struct TypesToIndicesHelper<next_index, Indices<indices...>>
    {
        typedef Indices<indices...> GetIndices;
    };
    
    // As long as there are types left, continue the recursion
    template <std::size_t next_index, std::size_t... indices, typename Type, typename... Types>
    struct TypesToIndicesHelper<next_index, Indices<indices...>, Type, Types...>
    {
        typedef typename TypesToIndicesHelper<next_index+1, Indices<indices..., next_index>,
					  Types...>::GetIndices GetIndices;
    };
    
    // Converts a pack of types to a same-sized pack of integers (e.g., <0, 1, 2, etc.>)
    template <typename... Types>
    struct TypesToIndices {
        typedef typename TypesToIndicesHelper<0, Indices<>, Types...>::GetIndices GetIndices;
    };
    
    template <typename IndicesT>
    struct EventHelper;
    
    template <std::size_t... indices>
    struct EventHelper<Indices<indices...>>
    {
        template <typename DelegateType, typename... ParamTypes>
        static void invokeDelegate(DelegateType &delegate, std::tuple<ParamTypes...> *param_tuple)
        {
            delegate(std::move(std::get<indices>(*param_tuple))...);
        }
    };
    
    
    // "Executor" methods for the event
    template<typename DelegateType, bool on_heap, typename... ParamTypes>
    static void eventExecutor_(v2vDelegate &d, void *hdata, void *ldata) {
        DelegateType &delegate = reinterpret_cast<DelegateType&>(d);
        void *tuple_data = (on_heap)?(hdata):(ldata);
        std::tuple<typename std::remove_reference<ParamTypes>::type...> *param_tuple =
            static_cast<std::tuple<typename std::remove_reference<ParamTypes>::type...> *>(tuple_data);
        
        // Use the helper class above to convert the tuple to a parameter pack
        EventHelper<typename TypesToIndices<ParamTypes...>::GetIndices>::invokeDelegate(delegate,
										    param_tuple);
    }
    
    // Special Executor version if no parameters
    template<typename DelegateType>
    static void eventExecutor_noparams_(v2vDelegate &d, void *hdata, void *ldata) {
        DelegateType &delegate = reinterpret_cast<DelegateType&>(d);
        (void) hdata;
        (void) ldata;
        
        delegate();
    }
    
    
    // Methods for deleting the tuple
    // On heap:
    template <typename... ParamTypes>
    static void paramDeleterHeap_(void *hdata, void *ldata) {
        (void) ldata;
        std::tuple<typename std::remove_reference<ParamTypes>::type...> *param_tuple =
            static_cast<std::tuple<typename std::remove_reference<ParamTypes>::type...> *>(hdata);
        delete param_tuple;
    }
    
    // Local:
    template <typename... ParamTypes>
    static void paramDeleterLocal_(void *hdata, void *ldata) {
        (void) hdata;
        std::tuple<typename std::remove_reference<ParamTypes>::type...> *param_tuple =
            static_cast<std::tuple<typename std::remove_reference<ParamTypes>::type...> *>(ldata);
        param_tuple->~tuple();
    }
    
    // Special Deleter if no parameters
    static void paramDeleterNone_(void *hdata, void *ldata) {
            (void) hdata;
            (void) ldata;
            // Nothing to do
    }
    
public:
    // Default constructor
    EventType() :
        fpEventExecutor_(nullptr),
        fpParamDeleter_(paramDeleterNone_)
    {}
    
    // Constructor for no params (default ordering)
    template <typename DelegateType>
    EventType(uint64_t t, DelegateType &delegate) :
        time_(t)
    {
        // Make sure the param count matches the delegate
        static_assert(DelegateType::numParams == 0,
		  "Delegate and Event parameter counts do not match");
        
        // Save the delegate.
        // The reinterpret_cast here is intentional type-erased storage: every
        // generic::delegate<void(...)> specialization shares the same layout
        // (object pointer + stub pointer + empty shared_ptr-based store), so
        // we store any signature in del_ and recover it via fpEventExecutor_.
        // We invoke the delegate's defaulted operator= rather than memcpy so
        // that any non-trivial member assignment (e.g. shared_ptr refcount)
        // remains correct.  The strict-aliasing warning here is a known false
        // positive for this pattern and is suppressed locally.
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstrict-aliasing"
        reinterpret_cast<DelegateType&>(del_) = delegate;
        #pragma GCC diagnostic pop
        
        // Save the appropriate event executor function
        fpEventExecutor_ = eventExecutor_noparams_<DelegateType>;
        
        // Save the null tuple deleter (no tuple to delete)
        fpParamDeleter_ = paramDeleterNone_;
    }
    
    // Constructor with params (default ordering)
    template <typename DelegateType, typename... ParamTypes>
    EventType(uint64_t t, DelegateType &delegate, ParamTypes&&... params) :
        time_(t)
    {
        static_assert(sizeof...(ParamTypes) > 0, "This constructor only for 1 or more params");
        // Make sure the param count matches the delegate
        static_assert(DelegateType::numParams == sizeof...(ParamTypes),
		  "Delegate and Event param counts do not match");
        
        // Save the delegate.  See the comment in the no-params constructor
        // above for why we use reinterpret_cast here and locally suppress the
        // strict-aliasing warning.
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstrict-aliasing"
        reinterpret_cast<DelegateType&>(del_) = delegate;
        #pragma GCC diagnostic pop
        
        // Save the params in a tuple
        typedef std::tuple<typename std::remove_reference<ParamTypes>::type...> ParamTuple;
        if (sizeof(ParamTuple) <= sizeof(std::array<uint64_t, max_local_uint64s>))
        {
            // The tuple will fit in the local data
            ParamTuple *param_tuple = reinterpret_cast<ParamTuple*>(&data_);
            new(param_tuple) ParamTuple(std::forward<ParamTypes>(params)...);
            
            // Save the local tuple deleter
            fpParamDeleter_ = paramDeleterLocal_<ParamTypes...>;
            
            // Save the event executor function
            fpEventExecutor_ = eventExecutor_<DelegateType, false, ParamTypes...>;
        }
        else {
            // The parameter tuple is too big - needs to go on the heap
            heap_data_ =
	static_cast<void *>(new ParamTuple(std::forward<ParamTypes>(params)...));
            
            // Save the heap tuple deleter
            fpParamDeleter_ = paramDeleterHeap_<ParamTypes...>;
            
            // Save the appropriate event executor function
            fpEventExecutor_ = eventExecutor_<DelegateType, true, ParamTypes...>;
        }
    }

    // Note: copy, move, or assignment makes the original event object "null",
    // in order to avoid unnecessary deep copying.
    
    // Copy constructor
    EventType(const EventType &orig) :
        fpEventExecutor_(orig.fpEventExecutor_),
        fpParamDeleter_(orig.fpParamDeleter_),
        del_(orig.del_),
        time_(orig.time_),
        heap_data_(orig.heap_data_),
        order_(orig.order_),
        data_(orig.data_)
    {
        EventType &unconst_orig = const_cast<EventType &>(orig);
        unconst_orig.fpEventExecutor_ = nullptr;
        unconst_orig.fpParamDeleter_ = paramDeleterNone_;
        unconst_orig.heap_data_ = nullptr;
    }
    
    // Move constructor
    EventType(EventType &&orig) :
        fpEventExecutor_(orig.fpEventExecutor_),
        fpParamDeleter_(orig.fpParamDeleter_),
        del_(orig.del_),
        time_(orig.time_),
        heap_data_(orig.heap_data_),
        order_(orig.order_),
        data_(orig.data_)
    {
        orig.fpEventExecutor_ = nullptr;
        orig.fpParamDeleter_ = paramDeleterNone_;
        orig.heap_data_ = nullptr;
    }
    
    // Assignment operator
    EventType &operator= (const EventType &orig)
    {
        fpEventExecutor_ = orig.fpEventExecutor_;
        fpParamDeleter_ = orig.fpParamDeleter_;
        del_ = orig.del_;
        time_ = orig.time_;
        heap_data_ = orig.heap_data_;
        order_ = orig.order_;
        data_ = orig.data_;
        
        EventType &unconst_orig = const_cast<EventType &>(orig);
        unconst_orig.fpEventExecutor_ = nullptr;
        unconst_orig.fpParamDeleter_ = paramDeleterNone_;
        unconst_orig.heap_data_ = nullptr;
        return *this;
    }

    // Destructor
    ~EventType() {
        // Delete the param tuple (if any)
        (*fpParamDeleter_)(heap_data_, &data_);
    }
    
    // operators
    bool operator<(const EventType<max_local_bytes> &rhs) const {
        return (time_ < rhs.time_) || ((time_ == rhs.time_) && (order_ < rhs.order_));
    }
    bool operator>(const EventType<max_local_bytes> &rhs) const {
        return (time_ > rhs.time_) || ((time_ == rhs.time_) && (order_ > rhs.order_));
    }
    
    // Set the event ordering (for events scheduled at the same time)
    void setFirst()       { order_ = EVENT_FIRST; }
    void setNextToFirst() { order_ = EVENT_NEXT_TO_FIRST; }
    void setNextToLast()  { order_ = EVENT_NEXT_TO_LAST; }
    void setLast()        { order_ = EVENT_LAST; }
    void setOrder(int32_t order) { order_ = order; }
    
    int32_t getOrder() {
        return order_;
    }
    
    // Do the event
    void doEvent() {
        assert(fpEventExecutor_ != nullptr);
        (*fpEventExecutor_)(del_, heap_data_, &data_);
        fpEventExecutor_ = nullptr; // To catch "double execution" errors
    }
    
    uint64_t getTime() const { return time_; }
};

// By default, Event uses a local data area of size MURM_EVENT_MAX_LOCAL_DATA_SIZE_BYTES
typedef EventType<MURM_EVENT_MAX_LOCAL_DATA_SIZE_BYTES> Event;

} // namespace murm

#endif // __MURM_EVENT_HPP__
