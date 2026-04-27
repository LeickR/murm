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


#ifndef __MURM_PARAM_BASE_HPP__
#define __MURM_PARAM_BASE_HPP__

#include <string>
#include <vector>
#include <sstream>
#include <cassert>

namespace murm {

// Forward declarations
class Component;

class ParamBase
{
protected:
    Component *owner_;
    std::string name_;
    std::string full_name_;
    std::string default_valstring_;
    std::string valstring_;
    std::string descrip_;
    bool override_{false};

    // Constructor
    ParamBase(Component *owner, const std::string &name, const std::string &default_valstring,
                 const std::string &descrip) :
        owner_(owner),
        name_(name),
        default_valstring_(default_valstring),
        descrip_(descrip)
    {}

public:
    // Get this parameter's name
    const std::string &getName() const { return name_; }

    // Get this parameter's full name
    const std::string &getFullName() const { return full_name_; }

    // Get this parameter's default value string
    const std::string &getDefaultValStr() const { return default_valstring_; }

    // Get this parameter's value string
    const std::string &getValStr() const { return valstring_; }

    // Get this parameter's description
    const std::string &getDescrip() const { return descrip_; }
};


} // namespace murm

#endif // __MURM_PARAM_BASE_HPP__
