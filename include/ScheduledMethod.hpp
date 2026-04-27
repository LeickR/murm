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


#ifndef __MURM_SCHEDULED_METHOD_HPP__
#define __MURM_SCHEDULED_METHOD_HPP__

#include "marcmo/SRDelegate.hpp"
#include "Event.hpp"
#include "Component.hpp"
#include "EventManager.hpp"

namespace murm {

// The parameter type list should match the parameters of the bound method
template<typename... ParamTypes>
class ScheduledMethod {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;
private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    DelegateType del_;
    uint32_t order_ {0};
public:
    // Constructor
    ScheduledMethod(Component *owner, uint32_t order = 0);
    
    // Bind this ScheduledMethod to a method:
    //
    //    ScheduledMethod scheduled_foo(this);
    //    scheduled_foo.bindToMethod(this, &MyClass::foo);
    //
    template <typename T>
    void bindToMethod(T *obj_ptr, void (T::*method)(ParamTypes...)) {
        del_ = generic::delegate<void(ParamTypes...)>::from(*obj_ptr, method);
    }
    
    template <typename... ParamU>
    void operator()(uint64_t time, ParamU&&... params);

    // Append an event with LAST ordering
    // (typically, you shouldn't have to use this method)
    //void appendLast(uint64_t time, ParamTypes&&... params);
};

} // namespace murm

// This needs to be included after ScheduledMethod declaration, above, but before the method
// definitions below.

namespace murm {

template<typename... ParamTypes>
ScheduledMethod<ParamTypes...>::ScheduledMethod(Component *owner, uint32_t order) :
    owner_(owner),
    event_manager_(owner->getEventManager()),
    order_(order)
{}

template<typename... ParamTypes>
template <typename... ParamU>
void ScheduledMethod<ParamTypes...>::operator()(uint64_t time, ParamU&&... params) {
    event_manager_->emplace(order_, event_manager_->getTime() + time, del_, std::forward<ParamTypes>(params)...);
}

//template<typename... ParamTypes>
//void ScheduledMethod<ParamTypes...>::appendLast(double time, ParamTypes&&... params) {
//   EventManager::EventType event(time, del_, std::forward<ParamTypes>(params)...);
//   event.setLast();
//   event_manager_->append(event);
//}

} // namespace murm

#endif // __MURM_SCHEDULED_METHOD_HPP__
