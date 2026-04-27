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
// Command line options:
// -p to override a parameter (or multiple parameters if wildcards are used)
//    Example:  -p 'sim.name.wild*.param=value'
// -pin to input parameter overrides from a parameter file
// -pout to output a parameter file (e.g., to reproduce the run)
// -lp to list all params and quit
// -Lp to list all params with descriptions and quit
// 
// Component and parameter names must not contain "*", "?", "=", or ".".
// (Add valid name check.)
// 
// If the first name in the override specifier is not "sim", "sim." is implied.
// 
// (The sim object is allowed to have some params. E.g., # of threads.)
// 
// Wild cards may be specified in an override:
// * within a name can represent zero or more characters.
// ? within a name represents exactly one character
// ** instead of a name represents zero or more levels in the tree.
// 
// 
// Parameter overrides (e.g., from commandline or param file) build a parameter overlay tree
// (before construction), consisting of nodes and leaves.
// 
// Each override adds a single leaf to the overlay tree, and may add one or more intermediate
// nodes.
// 
// Each leaf has a parameter override ordering number, incremented for each leaf created.
// 
// The "sim" object starts with the top-level node in the param overlay tree.
// 
// As each component is constructed, it does the following:
//
// - it obtains a list of overlay nodes from the parent. The parent constructs the list as
//   follows: it checks the child nodes of each of its own nodes. If the child node matches the
//   (child) component's name (including wildcards), then the child node is added to the
//   list. If the child node is "**", it is added to the list plus its child nodes are
//   recursively checked as well.
//   Also, if it has a "**" node in its own list, then that node is also added to the new list.
//
// - it goes through this new list of overlay nodes and collects all of the param leaves from
//   each. The param leaves are all sorted (in reverse order) by parameter override ordering
//   number.
//
// As each parameter is constructed, it requests an override, if any, from its owner component.
// The component iterates through the sorted list of leaves, and returns the first match
// (remember, they're in reverse order, so the last match will be encountered first).
// If a match is returned, the param value is updated.
// 

#ifndef __MURM_PARAM_OVERLAY_HPP__
#define __MURM_PARAM_OVERLAY_HPP__

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cassert>
#include "string_utils.hpp"

typedef unsigned int uint;

namespace murm {

// A leaf in the parameter overlay tree
class ParamOverride
{
private:
    std::string param_name_;  // May contain wildcards
    std::string valstr_;
    uint ordering_num_{0};
public:
    // Constructor
    ParamOverride(const std::string &param_name, const std::string &valstr, uint ordering_num) :
        param_name_(param_name),
        valstr_(valstr),
        ordering_num_(ordering_num)
    {}

    const std::string &getParamName() const { return param_name_; }
    const std::string &getValueStr() const { return valstr_; }
    uint getOrderingNum() const { return ordering_num_; }
};

// A node in the parameter overlay tree
class ParamOverlayNode
{
private:
    std::string name_;  // May contain wildcards

    std::vector<ParamOverride> param_overrides_;

    std::map<std::string, ParamOverlayNode> child_node_map_;

    void setName_(const std::string &name) { name_ = name; }
    
    void addOverride_(const std::string &paramname, const std::string &valstr, uint ordering_num,
                            const std::vector<std::string> &path, uint offset)
    {
        assert(path.size() > offset);
        assert(path[offset] == name_);

        if (path.size() == offset + 1) {
            // OK, we're at the end of the path, the param override goes here
            param_overrides_.emplace_back(paramname, valstr, ordering_num);
        } else {
            // Find or create the next node in the path
            uint next_offset = offset + 1;
            ParamOverlayNode &child_node = child_node_map_[path[next_offset]];
            // Check if we just created this child node
            if (child_node.getName() == "") {
                child_node.setName_(path[next_offset]);
            }
            child_node.addOverride_(paramname, valstr, ordering_num, path, next_offset);
        }
    }

public:
    // Constructors
    ParamOverlayNode(const std::string &name) :
        name_(name)
    {}

    ParamOverlayNode()
    {}

    void addOverride(const std::string &paramname, const std::string &valstr, uint ordering_num,
                          const std::vector<std::string> &path)
    {
        // start at element 0 in res_vec
        addOverride_(paramname, valstr, ordering_num, path, 0);
    }
    
    const std::vector<ParamOverride> &getOverrides() const { return param_overrides_; }

    const std::string &getName() const { return name_; }

    const std::map<std::string, ParamOverlayNode> &getChildNodeMap() const { return child_node_map_; }
};


// Parameter overlay generator
class ParamOverlayGen
{
private:
    // Top-level parameter overlay node
    ParamOverlayNode overlay_top_{"sim"};

    uint next_ordering_num_ {0};


    // Split a string
    //std::vector<std::string> &split_(const std::string &s, char delim,
    //                                 std::vector<std::string> &res_vec)
    //{
    //   std::stringstream ss(s);
    //   std::string sval;
    //   while (std::getline(ss, sval, delim)) {
    //      res_vec.push_back(sval);
    //   }
    //   return res_vec;
    //}

public:
    void addOverride(const std::string &pover) {
        std::vector<std::string> res_vec;

        // Get the path and the value
        //string_utils::splitString(pover, '=', res_vec);
        //if (res_vec.size() != 2) {
        //   std::cerr << "Invalid parameter specification: " << pover << std::endl;
        //   assert(0);
        //}
        //std::string path = res_vec[0];
        //std::string valstr = res_vec[1];

        // Get the path and the value
        if (pover.find('=') == std::string::npos) {
            // '=' not found
            std::cerr << "Invalid parameter specification: " << pover << std::endl;
            assert(0);
        }
        std::string path;
        std::string valstr;
        string_utils::splitStringOnFirst(pover, '=', path, valstr);

        //std::cout << "Override parsed: path = '" << path << "', valstr = '" << valstr << "'"
        //          << std::endl;

        // Split the path into nodes and param name (may include wildcards)
        res_vec.clear();
        // If the path doesn't explicitly start with "sim.", prepend it
        if (path.compare(0,4,"sim.") != 0) {
            res_vec.push_back("sim");
        }
        string_utils::splitString(path, '.', res_vec);
        std::string paramname = res_vec.back();
        res_vec.pop_back();

        //std::cout << "Override parsed:  paramname = \"" << paramname
        //          << "\", new value = \"" << valstr << "\"" << std::endl;
        //std::cout << "      path = ";
        //for(auto &node : res_vec) {
        //   std::cout << "\"" << node << "\" ";
        //}
        //std::cout << std::endl;
        
        overlay_top_.addOverride(paramname, valstr, next_ordering_num_, res_vec);
        ++next_ordering_num_;
    }

    const ParamOverlayNode *getParamOverlay() const { return &overlay_top_; }
};


} // namespace murm

#endif // __MURM_PARAM_OVERLAY_HPP__
