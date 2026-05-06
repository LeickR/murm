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
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../include/CLI11.hpp"
#include "../include/Param.hpp"
#include "../include/Component.hpp"
#include "../include/Stat.hpp"
#include "../include/SimOptionError.hpp"

// Command-line handler for a murm simulation.
//
// Supported options (POSIX/GNU style):
//
//   -p, --param PATH=VALUE   Override a parameter (repeatable).  PATH may
//                            contain '*' / '?' wildcards, e.g.
//                              -p sim.h.iparam=42
//                              --param 'sim.*.iparam=99'
//
//   --pin  FILE              Read parameter overrides from FILE, one per
//                            line (repeatable).
//
//   --pout FILE              After construction, write every parameter's
//                            current value to FILE.
//
//   --lp / --Lp              List parameters by component type, terse /
//                            with descriptions.  After listing, the
//                            simulation quits without running.
//   --ls / --Ls              Same, for stats.
//
//   -h, --help               Print this help and quit (provided by CLI11).
//
// All parser errors are translated into the murm::SimOptionError hierarchy
// (defined in SimOptionError.hpp) so callers can catch a single type and so
// unit tests can verify error paths.
class SimOptionHandler
{
private:
    // Parameter overlay generator (built up as -p / --pin specs are processed).
    murm::ParamOverlayGen param_overlay_gen_;

    // CLI11 parser.  We keep it as a member rather than constructing it
    // anew per parse() so that our option configuration (callbacks, help
    // text, ...) is set up exactly once in the constructor.
    CLI::App app_{"murm simulation runner"};

    // State driven by CLI11.
    std::string pout_file_;
    bool list_params_terse_   {false};
    bool list_params_verbose_ {false};
    bool list_stats_terse_    {false};
    bool list_stats_verbose_  {false};
    bool help_requested_      {false};

    // We accumulate -p / --pin invocations in argv order (CLI11 fires per-
    // occurrence callbacks in argv order) and then process them all after
    // app_.parse() returns.  That way an exception from parseParam_ /
    // readParamFile_ propagates cleanly out of parseSimOptions instead of
    // being raised from inside a CLI11 callback (where its handling is less
    // well-defined), and ordering between -p and --pin is preserved.
    enum class PendingKind { Inline, File };
    struct PendingOverride { PendingKind kind; std::string value; };
    std::vector<PendingOverride> pending_;

    void parseParam_(const std::string &pover) {
        std::cout << "Param override:  '" << pover << "'" << std::endl;
        param_overlay_gen_.addOverride(pover);
    }

    void readParamFile_(const std::string &path) {
        std::ifstream pfin(path);
        if (pfin.fail()) {
            throw murm::ParamFileNotFoundError(path);
        }
        std::string line;
        while (std::getline(pfin, line)) {
            parseParam_(line);
        }
    }

    void configureApp_() {
        app_.add_option("-p,--param",
                        "Parameter override of the form PATH=VALUE; repeatable. "
                        "PATH supports '*' / '?' wildcards.")
            ->type_name("PATH=VALUE")
            ->take_all()
            ->each([this](const std::string &v) {
                pending_.push_back({PendingKind::Inline, v});
            });

        app_.add_option("--pin",
                        "Read parameter overrides from FILE, one PATH=VALUE per line; "
                        "repeatable.")
            ->type_name("FILE")
            ->take_all()
            ->each([this](const std::string &v) {
                pending_.push_back({PendingKind::File, v});
            });

        app_.add_option("--pout", pout_file_,
                        "After construction, dump every parameter's current value to FILE.")
            ->type_name("FILE");

        app_.add_flag("--lp", list_params_terse_,
                      "List parameters by component type, then quit.");
        app_.add_flag("--Lp", list_params_verbose_,
                      "Like --lp but include parameter descriptions.");
        app_.add_flag("--ls", list_stats_terse_,
                      "List stats by component type, then quit.");
        app_.add_flag("--Ls", list_stats_verbose_,
                      "Like --ls but include stat descriptions.");
    }

public:
    // Constructor
    SimOptionHandler() {
        configureApp_();
    }

    // Access the underlying CLI11 parser so a model developer can register
    // their own command-line options on top of the framework's set, e.g.:
    //
    //     SimOptionHandler soh;
    //     int    seed = 42;
    //     double temp = 0.0;
    //     soh.getApp().add_option("--seed", seed, "Random seed");
    //     soh.getApp().add_option("--temp", temp, "Temperature in K");
    //     soh.parseSimOptions(argc, argv);   // parses framework + custom options
    //
    // Subclassing works too: a derived class can do the same in its
    // constructor body and ship a reusable bundle of model-specific options.
    //
    // Callers SHOULD NOT call getApp().parse(...) directly - go through
    // parseSimOptions() so the framework's exception translation, pending
    // -p / --pin processing, and overlay application all run.
    CLI::App       &getApp()       { return app_; }
    const CLI::App &getApp() const { return app_; }

    // Read-only access to the parameter-overlay tree we built up while
    // parsing.  Useful for tests, and for callers that want to apply the
    // overlay to a non-singleton root.
    const murm::ParamOverlayNode *getParamOverlayNode() const {
        return param_overlay_gen_.getParamOverlay();
    }

    // Whether `-h` / `--help` was given (in which case CLI11 has already
    // printed the help text and parseSimOptions returned without applying
    // any overrides).  processCLA() also treats this as a "quit" signal.
    bool helpRequested() const { return help_requested_; }

    void parseSimOptions(const int argc, char* argv[]) {
        try {
            app_.parse(argc, argv);
        }
        catch (const CLI::CallForHelp &) {
            // -h / --help: print the help text and treat the rest of the
            // run as a graceful no-op.  We don't throw here, so that an
            // example1-style main() doesn't have to special-case help.
            std::cout << app_.help();
            help_requested_ = true;
            return;
        }
        catch (const CLI::ParseError &e) {
            // Any CLI11 parse error (unknown option, extra args, missing
            // value, conversion failure, ...) becomes a single
            // murm::UnknownOptionError with CLI11's original message
            // forwarded verbatim - CLI11 already names the offending token
            // and reason, and double-wrapping it produces awkward output.
            throw murm::UnknownOptionError(e.what());
        }

        // Now process the -p / --pin invocations in argv order.  A bad
        // override spec or missing pin-file at this point throws
        // murm::InvalidOverrideSpec / murm::ParamFileNotFoundError, which
        // propagates out of parseSimOptions just like an option-parse
        // error would.
        for (const auto &p : pending_) {
            if (p.kind == PendingKind::Inline) { parseParam_(p.value); }
            else                               { readParamFile_(p.value); }
        }

        // Hand off the parameter overlay we built to the top-level "sim"
        // object.
        murm::Component::getToplevelComponent()
            ->addParamOverlayNode(param_overlay_gen_.getParamOverlay());
    }

    // Handle post-construction command-line actions (e.g., list params/stats,
    // dump params).  Returns true if the simulation should quit without
    // running -- either because --help was given, or because one of the
    // listing flags was passed.
    bool processCLA() {
        if (help_requested_) { return true; }

        murm::Component *sim = murm::Component::getToplevelComponent();
        bool quit = false;

        if (!pout_file_.empty()) {
            std::ofstream pfout(pout_file_);
            for (auto &comp : *sim) {
                for (const auto &param : comp.getParams()) {
                    pfout << param->getFullName() << "=" << param->getValStr() << std::endl;
                }
            }
        }

        if (list_params_terse_ || list_params_verbose_) {
            const bool print_descrip = list_params_verbose_;
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

        if (list_stats_terse_ || list_stats_verbose_) {
            const bool print_descrip = list_stats_verbose_;
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
};

#endif // __MURM_SIM_OPTION_HANDLER_HPP__
