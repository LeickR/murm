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


#ifndef __MURM_INIT_METHOD_HPP__
#define __MURM_INIT_METHOD_HPP__

#include "marcmo/SRDelegate.hpp"
#include "Event.hpp"
#include "Component.hpp"
#include "EventManager.hpp"

namespace murm {

// The parameter type list should match the parameters of the bound method
template<typename... ParamTypes>
class InitMethod {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;
private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    DelegateType del_;
public:
    // Constructor
    InitMethod(Component *owner) :
        owner_(owner),
        event_manager_(owner->getEventManager())
    {}
    
    // Bind this InitMethod to a method:
    //
    //    InitMethod init_foo(this);
    //    init_foo.bindToMethod(this, &MyClass::foo);
    //
    template <typename T>
    void bindToMethod(T *obj_ptr, void (T::*method)(ParamTypes...)) {
        del_ = generic::delegate<void(ParamTypes...)>::from(*obj_ptr, method);
    }
    
    void operator()(ParamTypes&&... params) {
        event_manager_->emplace_init(del_, std::forward<ParamTypes>(params)...);
    }
};


} // namespace murm

#endif // __MURM_INIT_METHOD_HPP__
