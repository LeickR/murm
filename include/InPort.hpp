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


#ifndef __MURM_INPORT_HPP__
#define __MURM_INPORT_HPP__

#include <vector>
#include "marcmo/SRDelegate.hpp"
#include "Component.hpp"
#include "EventManager.hpp"

namespace murm {

// Forward declaration: InPort tracks pointers to OutPorts that have been cross-linked
// to it.  We only need the type as a pointer, so a forward declaration is sufficient.
template<typename... ParamTypes> class OutPort;

// InPort class
// (can be bound to a method or chained to other InPorts in a hierarchical component
//  arrangement; chain links are bidirectional so that the order of connect() calls
//  does not matter)
// ParamTypes are the types of data that are passed through the port
template<typename... ParamTypes>
class InPort {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;
    typedef generic::delegate<void(int, ParamTypes...)> IndexedDelegateType;

private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    // The delegate (for an InPort bound to a method)
    DelegateType del_;
    // The original delegate for an indexed InPort:
    IndexedDelegateType orig_del_;
    int idx_ {-1};

    bool connected_ {false};  // true if this InPort has been wired into a complete chain
    bool dangling_ {false}; // true if this is intentionally left dangling (unconnected)

    // Support for InPort↔InPort chaining (hierarchical components).
    // The chain is stored as an undirected adjacency list: when two InPorts are
    // connected via murm::connect(a, b, delay), each of a and b records the other
    // here.  Resolution walks the resulting tree at connection time.
    struct ChainPeer {
        InPort<ParamTypes...> *peer;
        uint64_t delay;
    };
    std::vector<ChainPeer> chain_peers_;

    // Every OutPort that has cross-linked into this InPort registers itself here so
    // that, when the InPort chain or its bound delegate later changes, the OutPort
    // can be (re)resolved.
    std::vector<OutPort<ParamTypes...> *> cross_outports_;

public:
    // Constructor
    InPort(Component *owner) :
        owner_(owner),
        event_manager_(owner->getEventManager())
    {}

    // Copy constructor
    InPort(const InPort&) = default;

    EventManager *getEventManager() { return event_manager_; }

    // Bind this InPort to a method:
    //
    //    my_inport.bindToMethod(obj_ptr, &MyClass::method);
    //
    // After binding, every OutPort already cross-linked anywhere in this InPort's
    // chain is given a chance to (re)resolve, so that "bind first then chain" and
    // "chain first then bind" produce the same final state.
    template <typename T>
    void bindToMethod(T *obj_ptr, void (T::*method)(ParamTypes...)) {
        del_ = generic::delegate<void(ParamTypes...)>::from(*obj_ptr, method);
        notifyChainCrossOutports_(nullptr);
    }

    // Make this an indexed InPort and bind to a method:
    //
    //    my_inport.bindToIndexedMethod(idx, obj_ptr, &MyClass::method);
    //
    //void setDelegateAndIndex(int idx, const IndexedDelegateType &new_del) {
    //   idx_ = idx;
    //   orig_del_ = new_del;
    //   BIND_TO_METHOD(this, InPort::indexedCall);
    //}

     template <typename T>
     void bindToIndexedMethod(int idx, T *obj_ptr,
                                      void (T::*method)(int, ParamTypes...))
     {
          idx_ = idx;
          orig_del_ = generic::delegate<void(int, ParamTypes...)>::from(*obj_ptr, method);
          bindToMethod(this, InPort::indexedCall);
     }

    // Adds the InPort index to the call parameters and invokes the original delegate:
    void indexedCall(ParamTypes... params) {
        orig_del_(idx_, params...);
    }

    // Mark if this InPort is intentionally left dangling (unconnected)
    void setDangling(bool dangling = true) {
        // Make sure that it's not already connected
        if (dangling) { assert(connected_ == false); }
        dangling_ = dangling;
    }
    bool isDangling() { return dangling_; }


    // Used when connecting to an OutPort
    // (don't call this directly, use "murm::connect(in_port, out_port);" instead)
    DelegateType &getDelegate() { return del_; }

    // Returns true if bindToMethod() has been called on this InPort.
    bool isBound() const { return bool(del_); }


    //// InPort↔InPort chain plumbing (don't call directly, use murm::connect()) ////

    // Append an undirected chain edge from this InPort to peer (with the given delay).
    // Connector<InPort, InPort> calls this on both ports so the chain is symmetric.
    void addChainPeer(InPort<ParamTypes...> *peer, uint64_t delay) {
        chain_peers_.push_back({peer, delay});
    }

    // Register an OutPort that has cross-linked into this InPort, so that we can
    // (re)resolve it later when the chain or binding state changes.
    void addCrossOutport(OutPort<ParamTypes...> *outport) {
        cross_outports_.push_back(outport);
    }

    // Walk the (undirected, tree-shaped) chain starting from this InPort and find
    // the bound endpoint, accumulating the per-edge delays along the unique path.
    // came_from is used to avoid revisiting the previous node during the walk;
    // pass nullptr at the top-level call.
    // Returns nullptr if no bound InPort is reachable.
    InPort<ParamTypes...> *findBoundEnd_(uint64_t &accumulated,
                                         InPort<ParamTypes...> *came_from) {
        if (isBound()) {
            return this;
        }
        for (auto &link : chain_peers_) {
            if (link.peer == came_from) continue;
            uint64_t branch_delay = accumulated + link.delay;
            InPort<ParamTypes...> *found = link.peer->findBoundEnd_(branch_delay, this);
            if (found != nullptr) {
                accumulated = branch_delay;
                return found;
            }
        }
        return nullptr;
    }

    // Convenience wrapper used by OutPort::connectToInPort.  Asserts a bound endpoint
    // exists (it must by the time an OutPort is being resolved against this chain).
    InPort<ParamTypes...> &getChainEnd(uint64_t &accumulated_delay) {
        accumulated_delay = 0;
        InPort<ParamTypes...> *end = findBoundEnd_(accumulated_delay, nullptr);
        assert(end != nullptr); // must have a bound delegate somewhere in the chain
        return *end;
    }

    // Idempotently mark this InPort and every other InPort reachable through chain
    // peers as connected.  Called once an OutPort has fully resolved against the
    // chain.  came_from is used to avoid revisiting; pass nullptr at the top level.
    void markChainConnectedRecursive_(InPort<ParamTypes...> *came_from) {
        if (!connected_) {
            assert(dangling_ == false);
            connected_ = true;
        }
        for (auto &link : chain_peers_) {
            if (link.peer == came_from) continue;
            link.peer->markChainConnectedRecursive_(this);
        }
    }
    void markChainConnected() { markChainConnectedRecursive_(nullptr); }

    // Notify every OutPort cross-linked anywhere in this chain that something has
    // changed (e.g. a new chain edge or a fresh binding) so that they may try to
    // (re)resolve.  Defined out-of-line below, after OutPort is declared.
    void notifyChainCrossOutports_(InPort<ParamTypes...> *came_from);


    // Mark when this InPort has been connected (legacy single-port API; kept for
    // backwards compatibility with code outside the chain machinery).
    void setConnected(bool connected = true) {
        if (connected) {
            assert(connected_ == false);
            assert(dangling_ == false);
        }
        connected_ = connected;
    }
    bool isConnected() { return connected_; }
};

} // namespace murm

// Have to put this at the end so that both InPort and OutPort are defined for the connect function
// (and InPort is defined for the OutPort).
//#include "InPortOutPort_connect.hpp"
#include "OutPort.hpp"

#endif // __MURM_INPORT_HPP__
