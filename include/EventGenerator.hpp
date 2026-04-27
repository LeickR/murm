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


#ifndef __MURM_EVENT_GENERATOR_HPP__
#define __MURM_EVENT_GENERATOR_HPP__

#include "marcmo/SRDelegate.hpp"
#include "Event.hpp"

namespace murm {

// Note:  In most cases, you should use ScheduledMethod instead of this class.
//
// EventGenerator class
// ParamTypes are the types of data that are passed to the event handler
template<typename EventT, typename... ParamTypes>
class EventGenerator {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;
private:
    // The delegate for the method to which this is bound
    DelegateType del_;
public:
    // Constructor
    EventGenerator() = default;
    
    // Bind this EventGenerator to a method:
    //
    //    my_evgen.bindToMethod(obj, &MyClass::method);
    //
    template <typename T>
    void bindToMethod(T *obj_ptr, void (T::*method)(ParamTypes...)) {
        del_ = generic::delegate<void(ParamTypes...)>::from(*obj_ptr, method);
    }

    template <typename... ParamU>
    EventT operator()(uint64_t time, ParamU&&... params) {
        return EventT(time, del_, std::forward<ParamU>(params)...);
    }
};

} // namespace murm

#endif // __MURM_EVENT_GENERATOR_HPP__
