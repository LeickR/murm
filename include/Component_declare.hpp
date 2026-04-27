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


#ifndef __MURM_COMPONENT_DECLARE_HPP__
#define __MURM_COMPONENT_DECLARE_HPP__

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include "EventManager.hpp"
#include "ParamBase.hpp"
#include "ParamOverlay.hpp"
//#include "Stat.hpp"
#include "string_utils.hpp"

namespace murm {

// forward declaration
class TopLevelComponent;
class StatSampler;

class Component {
private:
    std::string type_name_; // what "type" of Component is this?
    std::string name_; // this Component's name (e.g., "bar")
    std::string full_name_; // this Component's fully qualified name (e.g., "sim.foo.bar")
    Component *parent_ {nullptr}; // this Component's parent
    std::vector<Component *> children_; // this Component's children

    // the EventManager used by this component (for its Ports, etc.)
    EventManager *event_manager_ {nullptr};
    // the EventManager used by this component's children (if different)
    // [This is used, for example, during construction of multicore/multithreading
    // implementations]
    EventManager *event_manager_for_children_ {nullptr};

    std::vector<const ParamOverlayNode *> param_overlay_nodes_;
    std::vector<ParamOverride> param_overrides_;

    std::vector<ParamBase *> params_;

    std::map<std::string, StatSampler*> stats_;

    
    // Special constructor for constructing the singleton top-level "sim" component
    // If an EventManager for the simulation is not specified, the default singleton
    // EventManager will be used.
    Component(EventManager *event_manager = &EventManager::getDefaultEventManager()) :
        type_name_("sim"),
        name_("sim"),
        full_name_(name_),
        event_manager_(event_manager)
    {}

    friend class murm::TopLevelComponent;

    void checkChildNodes_(const ParamOverlayNode *node, Component *child) {
        for (const auto &key_childnode : node->getChildNodeMap()) {
            // If the child node is the special "wild path" ("**"), pass it down and
            // recursively check its child nodes
            if (key_childnode.first == "**") {
                child->addParamOverlayNode(&(key_childnode.second));
                checkChildNodes_(&(key_childnode.second), child);
            }
            // If child name matches (including wildcards), pass down the child node
            else if (string_utils::wildMatch(key_childnode.first, child->getName())) {
                child->addParamOverlayNode(&(key_childnode.second));
            }
        }
    }

    // Add a child, and pass down information (e.g., the parameter overlay nodes)
    void addChild_(Component *child) {
        children_.push_back(child);

        // Pass down the child's parameter overlay nodes
        // Iterate over the child nodes of each my nodes:
        for (auto mynode : param_overlay_nodes_) {
            // Check the child nodes and pass all that match down to the child component
            checkChildNodes_(mynode, child);
            // Also, if this node is the special "wild path" ("**"), pass it down as well
            if (mynode->getName() == "**") {
                child->addParamOverlayNode(mynode);
            }
        }
    }

protected:
    void setEventManagerForChildren(EventManager *event_manager_for_children) {
        event_manager_for_children_ = event_manager_for_children;
    }

    void clearEventManagerForChildren() {
        event_manager_for_children_ = nullptr;
    }

public:
    // An iterator class that can be used to iterate over a Component tree (or subtree).
    // This is a pre-order, depth-first iterator.
    class iterator {
    private:
        Component *top_; // the top of the tree or subtree (returned by "begin()")
        Component *curr_; // the current location
        typedef std::vector<Component*> ChildComponentVector;
        typedef std::pair<ChildComponentVector*, ChildComponentVector::iterator> ChildIterationPair;
        std::vector<ChildIterationPair> child_iteration_stack_;
    public:
        // Constructors
        // (this one's used by Component::begin() and Component::end())
        iterator(Component *top_of_subtree, Component *loc) {
            top_ = top_of_subtree;
            curr_ = loc;
        }

        // Copy constructor
        iterator(const iterator &it) :
            top_(it.top_),
            curr_(it.curr_),
            child_iteration_stack_(it.child_iteration_stack_)
        {}


        // Operators
        // (comparison)
        bool operator==(const iterator &rhs) const { return ((top_ == rhs.top_) && (curr_ == rhs.curr_)); }
        bool operator!=(const iterator &rhs) const { return (operator==(rhs) == false); }

        // (dereference)
        Component &operator*() { return (*curr_); }
        const Component &operator*() const { return (*curr_); }

        Component *operator->() { return curr_; }
        const Component *operator->() const { return curr_; }

        // (assignment)
        iterator &operator=(const iterator &it) {
            top_ = it.top_;
            curr_ = it.curr_;
            child_iteration_stack_ = it.child_iteration_stack_;

            return (*this);
        }

        // (prefix increment)
        iterator &operator++() {
            if (curr_->children_.empty()) {
                // No children...
                bool done = false;
                while (!done) {
                    // Proceed to the next sibling, if any
                    if (child_iteration_stack_.empty()) {
                        // We're at the end of the tree
                        curr_ = nullptr;
                        done = true;
                    } else {
                        ChildIterationPair &curr_it_pair = child_iteration_stack_.back();
                        ChildComponentVector *curr_child_vec = curr_it_pair.first;
                        ChildComponentVector::iterator &curr_child_it = curr_it_pair.second;
                        ++curr_child_it;
                        if (curr_child_it == curr_child_vec->end()) {
                            // No more siblings - go up one level and try again
                            child_iteration_stack_.pop_back();
                        } else {
                            // We've found the next sibling
                            curr_ = *curr_child_it;
                            done = true;
                        }
                    }
                }
            } else {
                // curr_ does have children, so start iterating through them
                child_iteration_stack_.emplace_back(&(curr_->children_), curr_->children_.begin());
                curr_ = curr_->children_.front();
            }
            return (*this);
        }

        // (postfix increment) [Try not to use this!!]
        iterator operator++(int) {
            iterator tmp(*this);
            operator++();
            return tmp;
        }
    }; // end of class iterator

    friend class iterator; // the iterator has access to Component's private members


    // begin() and end() methods for iterating through a Component tree
    // (with this Component as the root)
    iterator begin() { return iterator(this, this); }
    iterator end() { return iterator(this, nullptr); }


    // Component names cannot contain the characters *, ?, =, or .
    static bool isValidName(const std::string &name_to_test) {
        return ((name_to_test.find('*') == std::string::npos) &&
                  (name_to_test.find('?') == std::string::npos) &&
                  (name_to_test.find('=') == std::string::npos) &&
                  (name_to_test.find('.') == std::string::npos));
    }
    
    // Constructor
    Component(Component *parent, const std::string &type_name, const std::string &name) :
        type_name_(type_name),
        name_(name),
        parent_(parent)
    {
        assert(isValidName(name_));

        // If parent is null, then this is a top-tier component and its parent is
        // the singleton top-level "sim" component
        if (parent_ == nullptr) {
            parent_ = Component::getToplevelComponent();
        }

        // Prepend the parent's full name to get this component's full name
        full_name_ = parent_->getFullName() + '.' + name_;
        
        // Add this component to its parent's children.
        // The parent will also pass down information, such as the parameter overlay.
        parent_->addChild_(this);

        // Inherit the EventManager from the parent
        if (parent_->event_manager_for_children_ == nullptr) {
            event_manager_ = parent_->event_manager_;
        } else {
            event_manager_ = parent_->event_manager_for_children_;
        }

        //// Iterate over the parameter overlay nodes obtained from the parent and collect the
        //// param override "leaves"
        //for (auto node : param_overlay_nodes_) {
        //   for (auto const &leaf : node->getOverrides()) {
        //      param_overrides_.push_back(leaf);
        //   }
        //}
        //// And sort the overrides in reverse order of the ordering numbers
        //std::sort(param_overrides_.begin(), param_overrides_.end(),
        //          [](const ParamOverride &a, const ParamOverride &b)
        //            { return a.getOrderingNum() > b.getOrderingNum(); });
    }

    // DEPRECATED - do not use (but provided for compatibility with older code)
    Component(Component *parent, const std::string &name) :
        Component(parent, "", name)
    {}


    // We don't want Components to be moved after construction
    Component(const Component&) { assert(false); }
    Component(Component&&) { assert(false); }

    virtual ~Component() {}

    // Add a parameter overlay node (used during construction)
    void addParamOverlayNode(const ParamOverlayNode *node) {
        param_overlay_nodes_.push_back(node);

        // Collect the param override "leaves"
        for (const auto &leaf : node->getOverrides()) {
            param_overrides_.push_back(leaf);
        }
        // And sort the overrides in reverse order of the ordering numbers
        std::sort(param_overrides_.begin(), param_overrides_.end(),
                     [](const ParamOverride &a, const ParamOverride &b)
                        { return a.getOrderingNum() > b.getOrderingNum(); });
    }

    // Check for a matching parameter override
    const ParamOverride *getParamOverride(const std::string &param_name) {
        // Iterate through the sorted list of parameter overrides and return the first match, if
        // any.
        // (remember, they're in reverse order, so the last override specified on the command
        // line will be encountered first)
        for (const auto &param_override : param_overrides_) {
            // Check for match, including wildcards
            if (string_utils::wildMatch(param_override.getParamName(), param_name)) {
                return &param_override;
            }
        }
        return nullptr;
    }
    
    // Add a parameter to the parameter list (to enable global access)
    void addParam(ParamBase *param) {
        params_.push_back(param);
    }

    const std::vector<ParamBase *> &getParams() const { return params_; }

    // Add a stat to the stat list (to enable global access)
    void addStatSampler(StatSampler *stat, const std::string &stat_name) {
        stats_[stat_name] = stat;
    }

    // Get the stats map
    const std::map<std::string, StatSampler*> &getStats() const { return stats_; }

    // A convenience method if you only need one stat
    const StatSampler *getStat(const std::string &name) const {
        try {
            return stats_.at(name);
        } catch(const std::out_of_range&) {
            return nullptr;
        }
    }

    // Compute any statistics "snapshots" that might be needed
    virtual void computeStatSnapshot() {
        // by default, do nothing
    }

    
    EventManager *getEventManager() {
        return event_manager_;
    }

    // Get the current time
    uint64_t getTime() const {
        //std::cout << "Component::getTime(): event_manager_ = " << (void *)(event_manager_) << std::endl;
        return event_manager_->getTime();
    }

    // Get this Component's "type" name (what type of Component is this?)
    const std::string &getTypeName() const { return type_name_; }

    // Get this Component's name
    const std::string &getName() const { return name_; }

    // Get this Component's full name
    const std::string &getFullName() const { return full_name_; }

    
    // Get a pointer to the singleton top-level "sim" component
    static Component *getToplevelComponent();
};


} // namespace murm

#endif // __MURM_COMPONENT_DECLARE_HPP__
