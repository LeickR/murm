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

// 
// Param<ParamT>
// ParamT must have istream extraction operator and ostream insertion operator.
// {this, "name", "default value", "description"}
//
// Param has public methods that include:
// - conversion operator to ParamT
// - explicit get() method (for cases where compiler might have difficulties with the
//   conversion operator)
// - set() method to change the value stored in the parameter (should be used sparingly and
//   clearly described in the parameter description)
// 

#ifndef __MURM_PARAM_HPP__
#define __MURM_PARAM_HPP__

#include <string>
#include <sstream>
#include <cassert>
#include "Component_declare.hpp"
#include "ParamBase.hpp"

namespace murm {

template <typename ParamT>
class Param : public ParamBase
{
private:
    ParamT value_;


    // Finish common construction
    void initialize_() {
        // Params follow the same name validity rules as Components
        assert(Component::isValidName(name_));

        if (owner_) {
            // Prepend the owner's full name to get the Param's full name
            full_name_ = owner_->getFullName() + '.' + name_;
            
            // Check the owner for a parameter override
            const ParamOverride *param_override = owner_->getParamOverride(name_);
            
            // Add this to the owner's parameter list (to enable global access)
            owner_->addParam(this);
            
            //std::cout << "   Param " << name_ << ", default value = " << default_valstring_
            //          << " (" << value_ << ")";
            
            if (param_override) {
                std::istringstream ivalstream(param_override->getValueStr());
                ivalstream >> value_;
                override_ = true;
            }
            
            // Set the value string
            std::ostringstream ovalstream;
            ovalstream << value_;
            ovalstream.flush();
            valstring_ = ovalstream.str();
            
            //if (override_) { std::cout << ", override value = " << value_; }
            //std::cout << " [owner = " << owner_->getName() << "]" << std::endl;
        }
    }

public:
    // Constructors
    //Param(Component *owner, const std::string &name, const std::string &default_valstring,
    //      const std::string &descrip) :
    //   ParamBase(owner, name, default_valstring, descrip)
    //{
    //   // Set the initial default value
    //   std::istringstream valstream(default_valstring);
    //   valstream >> value_;
    //
    //   initialize_();
    //}

    Param(Component *owner, const std::string &name, const ParamT &default_value,
            const std::string &descrip) :
        ParamBase(owner, name, "", descrip),
        value_(default_value)
    {
        // Set the default_valstring
        std::ostringstream valstream;
        valstream << value_;
        valstream.flush();
        default_valstring_ = valstream.str();

        initialize_();
    }

    // Conversion operators to ParamT
    operator const ParamT&() const { return value_; }
    // We only allow "rvalue" conversion
    //operator ParamT&() { return value_; }

    const ParamT &get() const { return value_; }

    void set(const ParamT &newval) {
        value_ = newval;

        // Set the value string
        std::ostringstream ovalstream;
        ovalstream << value_;
        ovalstream.flush();
        valstring_ = ovalstream.str();
    }
};


} // namespace murm

#endif // __MURM_PARAM_HPP__
