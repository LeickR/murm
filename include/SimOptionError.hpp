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


#ifndef __MURM_SIM_OPTION_ERROR_HPP__
#define __MURM_SIM_OPTION_ERROR_HPP__

#include <stdexcept>
#include <string>

namespace murm {

// Base for all command-line / parameter-overlay parsing errors.
//
// SimOptionHandler and ParamOverlayGen throw one of the derived types below
// instead of calling assert(0) or exit(1).  This lets a release-build main()
// catch SimOptionError, print a diagnostic, and return a non-zero status,
// while letting unit tests catch the specific derived type to verify error
// paths without taking down the whole test binary.
class SimOptionError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// A command-line option not in the option-definition string (e.g. "-zzz").
class UnknownOptionError : public SimOptionError {
public:
    explicit UnknownOptionError(const std::string &option)
        : SimOptionError("unknown option: -" + option) {}
};

// A stray positional argument the parser cannot interpret.
class UnknownArgumentError : public SimOptionError {
public:
    explicit UnknownArgumentError(const std::string &arg)
        : SimOptionError("unknown argument: " + arg) {}
};

// A parameter override (e.g. from "-p" or a "-pin" file) that does not contain
// the required '=' separator (e.g. "-p sim.x.foo").
class InvalidOverrideSpec : public SimOptionError {
public:
    explicit InvalidOverrideSpec(const std::string &spec)
        : SimOptionError("invalid parameter override (missing '='): " + spec) {}
};

// "-pin <file>" could not open the requested file.
class ParamFileNotFoundError : public SimOptionError {
public:
    explicit ParamFileNotFoundError(const std::string &path)
        : SimOptionError("cannot open parameter file: " + path) {}
};

} // namespace murm

#endif // __MURM_SIM_OPTION_ERROR_HPP__
