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


#ifndef __MURM_CONNECT_HPP__
#define __MURM_CONNECT_HPP__

#include <vector>
#include <functional>
#include <cassert>

// We want to employ partial template specialization of the connect function.
// However, C++ only allows partial template specialization of classes.
// So, we'll use the following trick.
//
// *Why not just overload the functions?  Well, function overloading with function templates
//  follows rather arcane rules that can get rather tricky.  For more information, see
//  http://www.gotw.ca/publications/mill17.htm

namespace murm {
    // Here's the helper class
    template<class T, class U>
    struct Connector
    {
        static void connect(T &t, U &u, uint64_t delay); // not defined - useful versions will be specialized
    };


    // Here's the actual function itself:
    // Connects object t to object u, with the specified added delay
    template<class T, class U>
    void connect(T &t, U &u, uint64_t delay = 0) { Connector<T,U>::connect(t, u, delay); }


    // Some useful specializations:

    // Specializations for reference wrappers
    template<class T, class U>
    struct Connector<std::reference_wrapper<T>, std::reference_wrapper<U>>
    {
        static void connect(std::reference_wrapper<T> &rwt, std::reference_wrapper<U> &rwu, uint64_t delay)
        {
            assert(&(rwt.get()));
            assert(&(rwu.get()));
            murm::connect(rwt.get(), rwu.get(), delay);
        }
    };

    template<class T, class U>
    struct Connector<T, std::reference_wrapper<U>>
    {
        static void connect(T &t, std::reference_wrapper<U> &rwu, uint64_t delay)
        {
            murm::connect(t, rwu.get(), delay);
        }
    };

    template<class T, class U>
    struct Connector<std::reference_wrapper<T>, U>
    {
        static void connect(std::reference_wrapper<T> &rwt, U &u, uint64_t delay)
        {
            murm::connect(rwt.get(), u, delay);
        }
    };

    // Specialization for connecting vectors of things ("bundles").
    // Each connection will be made in order
    // (i.e., v1[0] will be  connected to v2[0], v1[1] will be  connected to v2[1], etc.).
    // Any specified delay will be propagated down to the individual connections.
    // The two vectors MUST be of the same size.
    template<class V, class W>
    struct Connector<std::vector<V>, std::vector<W>>
    {
        static void connect(std::vector<V> &v1, std::vector<W> &v2, uint64_t delay)
        {
            // The two vectors need to be the same size
            assert(&v1);
            assert(&v2);
            assert(v1.size() == v2.size());

            for (uint i = 0; i < v1.size(); ++i) {
                murm::connect(v1[i], v2[i], delay);
            }
        }
    };
} // namespace murm

#endif // __MURM_CONNECT_HPP__

