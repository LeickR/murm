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


#ifndef __MURM_COMPONENT_HPP__
#define __MURM_COMPONENT_HPP__

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include "Component_declare.hpp"
#include "Param.hpp"
#include "string_utils.hpp"

namespace murm {

class TopLevelComponent : public Component {
private:
    std::vector<ParamBase *> params_on_heap_;

public:
    // Constructor
    TopLevelComponent(EventManager *event_manager = &EventManager::getDefaultEventManager()) :
        Component(event_manager)
    {}

    // Destructor
    ~TopLevelComponent() {
        for (ParamBase *param : params_on_heap_) {
            delete param;
        }
    }

    // Add a parameter to this component.
    // Return a const reference to the new parameter.
    template <typename T>
    Param<T> &addParam(const std::string &name, const T &value, const std::string &descrip) {
        Param<T> *new_param = new Param<T>(this, name, value, descrip);
        params_on_heap_.push_back(new_param);
        return *new_param;
    }
};


// Get a pointer to the singleton top-level "sim" component
Component *Component::getToplevelComponent() {
    // Here's the global singleton top-level "sim" component:
    // (it's only created if used)
    static TopLevelComponent sim;

    return (&sim);
}


} // namespace murm

#endif // __MURM_COMPONENT_HPP__
