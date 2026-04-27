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

// Specialization for connecting InitInPorts to InitOutPorts
template<typename... ParamTypes>
struct Connector<InitInPort<ParamTypes...>, InitOutPort<ParamTypes...>>
{
    static void connect(InitInPort<ParamTypes...> &iiport, InitOutPort<ParamTypes...> &ioport, uint64_t delay = 0)
    {
        (void) delay;  // Ignored - all init events happen at initialization time
        
        // Make sure that these ports were not set as "dangling" (intentionally unconnected)
        assert(iiport.isDangling() == false);
        assert(ioport.isDangling() == false);
        // Make sure that they're not already connected
        assert(iiport.isConnected() == false);
        assert(ioport.isConnected() == false);

        ioport.setDelegate(iiport.getDelegate());

        iiport.setConnected();
        ioport.setConnected();
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

} // namespace murm

#endif // __MURM_INIT_INPORT_INIT_OUTPORT_CONNECT_HPP__
