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


#ifndef __MURM_INIT_INPORT_INIT_OUTPORT_CONNECT_HPP__
#define __MURM_INIT_INPORT_INIT_OUTPORT_CONNECT_HPP__

#include <cassert>
#include "InitInPort.hpp"
#include "InitOutPort.hpp"
#include "connect.hpp"

namespace murm {

// Specialization for cross-linking an InitInPort to an InitOutPort.
// As with the chain connectors below, this records the edge and then attempts
// to resolve the InitOutPort's chain.  All of the resolution work is done in
// InitOutPort::connectToInPort.
template<typename... ParamTypes>
struct Connector<InitInPort<ParamTypes...>, InitOutPort<ParamTypes...>>
{
    static void connect(InitInPort<ParamTypes...> &iiport, InitOutPort<ParamTypes...> &ioport, uint64_t delay = 0)
    {
        (void) delay;  // Ignored - all init events happen at initialization time
        ioport.connectToInPort(iiport);
    }
};

// and vice-versa
template<typename... ParamTypes>
struct Connector<InitOutPort<ParamTypes...>, InitInPort<ParamTypes...>>
{
    static void connect(InitOutPort<ParamTypes...> &ioport, InitInPort<ParamTypes...> &iiport, uint64_t delay)
    {
        murm::connect(iiport, ioport, delay);
    }
};

// InitInPort↔InitInPort: add a symmetric (undirected) chain edge between two
// InitInPorts in a hierarchical component arrangement.  The two arguments play
// interchangeable roles: the connector does not care which InitInPort is the
// bound one or which is the externally-facing one — that is discovered
// automatically when an InitOutPort later cross-links into the chain.
template<typename... ParamTypes>
struct Connector<InitInPort<ParamTypes...>, InitInPort<ParamTypes...>>
{
    static void connect(InitInPort<ParamTypes...> &a, InitInPort<ParamTypes...> &b, uint64_t delay = 0)
    {
        (void) delay;  // Ignored - all init events happen at initialization time
        assert(a.isDangling() == false);
        assert(b.isDangling() == false);
        a.addChainPeer(&b);
        b.addChainPeer(&a);
        // The new edge may have just connected an existing cross-linked
        // InitOutPort to (or closer to) a bound delegate — give every
        // cross-linked InitOutPort in the newly-merged chain a chance to
        // (re)resolve.
        a.notifyChainCrossOutports_(nullptr);
    }
};

// InitOutPort↔InitOutPort: add a symmetric (undirected) chain edge between two
// InitOutPorts in a hierarchical component arrangement.  As with the
// InitInPort↔InitInPort case, the arguments play interchangeable roles.
template<typename... ParamTypes>
struct Connector<InitOutPort<ParamTypes...>, InitOutPort<ParamTypes...>>
{
    static void connect(InitOutPort<ParamTypes...> &a, InitOutPort<ParamTypes...> &b, uint64_t delay = 0)
    {
        (void) delay;  // Ignored - all init events happen at initialization time
        assert(a.isDangling() == false);
        assert(b.isDangling() == false);
        a.addChainPeer(&b);
        b.addChainPeer(&a);
        // The new edge may have just connected the chain to a cross-linked
        // anchor (and/or to a bound InitInPort delegate) — try to resolve
        // every InitOutPort in the newly-merged chain.
        a.tryResolveChain_(nullptr);
    }
};

} // namespace murm

#endif // __MURM_INIT_INPORT_INIT_OUTPORT_CONNECT_HPP__
