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


#ifndef __MURM_SIM_OPTION_HANDLER_HPP__
#define __MURM_SIM_OPTION_HANDLER_HPP__

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <set>
#include <string>
#include <fstream>
#include "../include/CommandOptionHandler.h"
#include "../include/Param.hpp"
#include "../include/Component.hpp"

// Class to assist with command-line object parsing:
class SimOptionHandler : public CommandOptionHandler
{
private:
    // Parameter overlay generator
    murm::ParamOverlayGen param_overlay_gen_;

    static constexpr const char *valid_options_ = "p:+, pin:+, pout:, lp, Lp, ls, Ls";

    void parseParam_(const std::string &pover) {
        std::cout << "Param override:  '" << pover << "'" << std::endl;
        param_overlay_gen_.addOverride(pover);
    }

public:
    // Constructor
    SimOptionHandler() :
        CommandOptionHandler(valid_options_)
    {}

    // Handle post-construction command-line actions (e.g., list params/stats).
    // Returns true if the simulation should quit without running.
    bool processCLA() {
        murm::Component *sim = murm::Component::getToplevelComponent();
        bool quit = false;

        if (opCheck("pout")) {
            std::string poutname = opValue("pout");
            std::ofstream pfout(poutname);
            for (auto &comp : *sim) {
                for (const auto &param : comp.getParams()) {
                    pfout << param->getFullName() << "=" << param->getValStr() << std::endl;
                }
            }
        }

        if (opCheck("Lp") || opCheck("lp")) {
            bool print_descrip = opCheck("Lp");
            std::cout << "\nAvailable parameters:" << std::endl;
            std::set<std::string> types_found;
            for (auto &comp : *sim) {
                const std::string &comp_type = comp.getTypeName();
                if (types_found.count(comp_type) == 0) {
                    types_found.insert(comp_type);
                    std::cout << comp_type << ":" << std::endl;
                    if (comp.getParams().empty()) {
                        std::cout << "<no parameters>" << std::endl;
                    } else {
                        for (const auto &param : comp.getParams()) {
                            std::cout << "    " << param->getName() << " (default value = "
                                      << param->getDefaultValStr() << ")" << std::endl;
                            if (print_descrip) {
                                std::cout << "        " << param->getDescrip() << std::endl;
                            }
                        }
                    }
                }
            }
            quit = true;
        }

        if (opCheck("Ls") || opCheck("ls")) {
            bool print_descrip = opCheck("Ls");
            std::cout << "\nAvailable stats:" << std::endl;
            std::set<std::string> types_found;
            for (auto &comp : *sim) {
                const std::string &comp_type = comp.getTypeName();
                if (types_found.count(comp_type) == 0) {
                    types_found.insert(comp_type);
                    std::cout << comp_type << ":" << std::endl;
                    const auto &statmap = comp.getStats();
                    if (statmap.empty()) {
                        std::cout << "<no stats>" << std::endl;
                    } else {
                        for (const auto &statpair : statmap) {
                            std::cout << "    " << statpair.second->getName() << std::endl;
                            if (print_descrip) {
                                std::cout << "        " << statpair.second->getDescrip() << std::endl;
                            }
                        }
                    }
                }
            }
            quit = true;
        }

        return quit;
    }

    void parseSimOptions(const int argc, char* argv[]) {
        try {
            parseOptions(argc, argv);
        }
        catch (CommandOptionHandler::UnknownOption uO) {
            std::cerr << "Don't understand option \"-" << uO.option << "\"" << std::endl;
            //usage();
            assert(0);
        }
        catch (CommandOptionHandler::UnknownArg uA) {
            std::cerr << "Don't understand command line argument \"" << uA.arg << "\"" << std::endl;
            //usage();
            assert(0);
        }
        // Hand off the parameter overlay that we parsed to the top-level "sim" object
        murm::Component::getToplevelComponent()
            ->addParamOverlayNode(param_overlay_gen_.getParamOverlay());
    }

protected:
    // Handle the "-p" and "-pin" options in order:
    virtual void handleOption(const char *op, const char *value) {
        std::string sop(op);
        if (sop == "p") {
            parseParam_(value);
        }
        else if (sop == "pin") {
            // Get the params from the param file
            std::ifstream pfin(value);
            if (pfin.fail()) {
                std::cerr << "Cannot open parameter file " << value << std::endl;
                exit(1); // ERROR
            }
            std::string line;
            while (std::getline(pfin, line)) {
                parseParam_(line);
            }
        }
    }
};

#endif // __MURM_SIM_OPTION_HANDLER_HPP__
