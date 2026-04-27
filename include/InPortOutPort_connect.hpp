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

// InPort→InPort: chain port_a to port_b.
// When an OutPort later connects to port_a, it follows the chain to find the
// bound InPort (at the end of the chain) and accumulates delays along the way.
template<typename... ParamTypes>
struct Connector<InPort<ParamTypes...>, InPort<ParamTypes...>>
{
    static void connect(InPort<ParamTypes...> &port_a, InPort<ParamTypes...> &port_b, uint64_t delay)
    {
        assert(port_a.isDangling() == false);
        assert(port_b.isDangling() == false);
        assert(port_a.isConnected() == false);
        port_a.setChainNext(&port_b, delay);
    }
};

// OutPort→OutPort: chain inner through outer.
// When outer later connects to an InPort, inner is also connected to that InPort
// with the accumulated delay (outer_delay + chain_delay).
template<typename... ParamTypes>
struct Connector<OutPort<ParamTypes...>, OutPort<ParamTypes...>>
{
    static void connect(OutPort<ParamTypes...> &inner, OutPort<ParamTypes...> &outer, uint64_t delay)
    {
        assert(inner.isDangling() == false);
        assert(outer.isDangling() == false);
        outer.setChainedOutPort(&inner, delay);
    }
};

} // namespace murm

#endif // __MURM_INPORT_OUTPORT_CONNECT_HPP__
