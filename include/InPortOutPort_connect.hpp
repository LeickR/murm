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


#ifndef __MURM_INPORT_OUTPORT_CONNECT_HPP__
#define __MURM_INPORT_OUTPORT_CONNECT_HPP__

#include <cassert>
#include "InPort.hpp"
#include "OutPort.hpp"
#include "connect.hpp"

namespace murm {

// Specialization for connecting InPorts to OutPorts
template<typename... ParamTypes>
struct Connector<InPort<ParamTypes...>, OutPort<ParamTypes...>>
{
    static void connect(InPort<ParamTypes...> &iport, OutPort<ParamTypes...> &oport, uint64_t delay)
    {
        oport.connectToInPort(iport, delay);
    }
};

// and vice-versa
template<typename... ParamTypes>
struct Connector<OutPort<ParamTypes...>, InPort<ParamTypes...>>
{
    static void connect(OutPort<ParamTypes...> &oport, InPort<ParamTypes...> &iport, uint64_t delay)
    {
        murm::connect(iport, oport, delay);
    }
};

// InPort↔InPort: add a symmetric (undirected) chain edge between two InPorts in a
// hierarchical component arrangement.  The two arguments play interchangeable
// roles: the connector does not care which InPort is the bound one or which is
// the externally-facing one — that is discovered automatically when an OutPort
// later cross-links into the chain.
template<typename... ParamTypes>
struct Connector<InPort<ParamTypes...>, InPort<ParamTypes...>>
{
    static void connect(InPort<ParamTypes...> &a, InPort<ParamTypes...> &b, uint64_t delay)
    {
        assert(a.isDangling() == false);
        assert(b.isDangling() == false);
        a.addChainPeer(&b, delay);
        b.addChainPeer(&a, delay);
        // The new edge may have just connected an existing cross-linked OutPort to
        // (or closer to) a bound delegate — give every cross-linked OutPort in the
        // newly-merged chain a chance to (re)resolve.
        a.notifyChainCrossOutports_(nullptr);
    }
};

// OutPort↔OutPort: add a symmetric (undirected) chain edge between two OutPorts in
// a hierarchical component arrangement.  As with the InPort↔InPort case, the
// arguments play interchangeable roles: the connector does not care which OutPort
// is the externally-cross-linked one.
template<typename... ParamTypes>
struct Connector<OutPort<ParamTypes...>, OutPort<ParamTypes...>>
{
    static void connect(OutPort<ParamTypes...> &a, OutPort<ParamTypes...> &b, uint64_t delay)
    {
        assert(a.isDangling() == false);
        assert(b.isDangling() == false);
        a.addChainPeer(&b, delay);
        b.addChainPeer(&a, delay);
        // The new edge may have just connected the chain to a cross-linked anchor
        // (and/or to a bound InPort delegate) — try to resolve every OutPort in the
        // newly-merged chain.
        a.tryResolveChain_(nullptr);
    }
};

} // namespace murm

#endif // __MURM_INPORT_OUTPORT_CONNECT_HPP__
