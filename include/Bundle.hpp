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


#ifndef __MURM_BUNDLE_HPP__
#define __MURM_BUNDLE_HPP__

#include "connect.hpp"

// Bundle of connectors
// (a Bundle is basically a Vector with some "extras")

// Bundle of connectors
template <typename ConnectorT>
struct Bundle {
    typedef ConnectorT ConnectorType;

    std::vector<ConnectorT> connectors;

    // Constructor
    template <typename... ParamTypes>
    Bundle(int num_conn, ParamTypes... params)
    {
        for (int i = 0; i < num_conn; ++i) {
            connectors.emplace_back(params...);
        }
    }

    // Copy constructor
    Bundle(const Bundle&) = default;

    // Mark if this Bundle is intentionally left dangling (unconnected)
    void setDangling(bool dangling = true) {
        for (auto &conn : connectors) { conn.setDangling(dangling); }
    }

    ConnectorT &operator[](int i) { return connectors[i]; }

    size_t size() { return connectors.size(); }
};

// murm::connect specializations for connecting Bundles
namespace murm {
    template <typename T, typename U>
    struct Connector<Bundle<T>, Bundle<U>> {
        static void connect(Bundle<T> &b1, Bundle<U> &b2, uint64_t delay) {
            murm::connect(b1.connectors, b2.connectors, delay);
        }
    };
} // namespace murm

#endif // __MURM_BUNDLE_HPP__
