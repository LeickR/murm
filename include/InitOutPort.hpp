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


#ifndef __MURM_INIT_OUTPORT_HPP__
#define __MURM_INIT_OUTPORT_HPP__

#include <cassert>
#include <vector>
#include "marcmo/SRDelegate.hpp"
#include "Component.hpp"
#include "EventManager.hpp"
#include "InitInPort.hpp"
#include "connect.hpp"

namespace murm {

// InitOutPort class
// (can be cross-linked to an InitInPort and/or chained to other InitOutPorts in
//  a hierarchical component arrangement; chain links are bidirectional so that
//  the order of connect() calls does not matter)
// ParamTypes are the types of data that are passed through the port
template<typename... ParamTypes>
class InitOutPort {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;

private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    // The delegate (copied over when the chain of connection ends in an InitInPort bound to a method)
    DelegateType del_;

    bool immediate_ {false}; // If true, the call via the delegate will be made immediately
                                     // instead of through the EventManager's init queue

    bool connected_ {false}; // true if this InitOutPort has been resolved to a delegate
    bool dangling_ {false}; // true if this is intentionally left dangling (unconnected)

    // InitOutPort↔InitOutPort chain (undirected adjacency list).
    std::vector<InitOutPort<ParamTypes...> *> chain_peers_;

    // Cross-link to an InitInPort.  At most one InitOutPort in any given
    // InitOutPort chain may hold a cross-link.
    InitInPort<ParamTypes...> *cross_inport_ {nullptr};


    // Configure this single InitOutPort with a resolved delegate.
    void configureSelf_(InitInPort<ParamTypes...> &bound_inport) {
        assert(connected_ == false);
        assert(dangling_ == false);

        setDelegate(bound_inport.getDelegate());
        setConnected();
    }

    // Walk this InitOutPort's chain (undirected, tree-shaped) to find the
    // unique anchor: the InitOutPort that holds the cross-link into an
    // InitInPort.  Returns nullptr if no InitOutPort in this chain has been
    // cross-linked yet.
    InitOutPort<ParamTypes...> *findAnchor_(InitOutPort<ParamTypes...> *came_from) {
        if (cross_inport_ != nullptr) return this;
        for (auto *peer : chain_peers_) {
            if (peer == came_from) continue;
            InitOutPort<ParamTypes...> *found = peer->findAnchor_(this);
            if (found != nullptr) return found;
        }
        return nullptr;
    }

    // Try to resolve this single InitOutPort.  No-op if already connected, or
    // if the chains aren't yet far enough along to determine a delegate.
    void tryResolveSelf_() {
        if (connected_) return;
        if (dangling_) return;

        InitOutPort<ParamTypes...> *anchor = findAnchor_(nullptr);
        if (anchor == nullptr) return; // no cross-link in this chain yet

        InitInPort<ParamTypes...> *bound_inport =
            anchor->cross_inport_->findBoundEnd_(nullptr);
        if (bound_inport == nullptr) return; // no bound delegate yet

        configureSelf_(*bound_inport);

        // Idempotently mark every InitInPort in the chain as connected too.
        anchor->cross_inport_->markChainConnected();
    }


public:
    // Constructor
    InitOutPort(Component *owner, bool immediate = false) :
        owner_(owner),
        event_manager_(owner->getEventManager()),
        immediate_(immediate)
    {}

    // The InitOutPort is a functor:
    void operator()(ParamTypes... params) {
        assert(connected_ == true);
        if (immediate_) {
            // Make the call immediately
            del_(params...);
        } else {
            // Create an event (Event(0.0, del_, params...)) and enqueue it to the EventHandler
            event_manager_->emplace_init(del_, std::forward<ParamTypes>(params)...);
        }
    }

    // Mark if this InitOutPort is intentionally left dangling (unconnected)
    void setDangling(bool dangling = true) {
        // Make sure that it's not already connected
        if (dangling) { assert(connected_ == false); }
        dangling_ = dangling;
    }
    bool isDangling() { return dangling_; }

    // Internal setter used during resolution.
    // (don't call this directly, use "murm::connect(in_port, out_port);" instead)
    void setDelegate(const DelegateType &new_del) { del_ = new_del; }


    //// InitOutPort↔InitInPort cross-link plumbing (don't call directly, use murm::connect()) ////

    // Record a cross-link from this InitOutPort to inport, then try to resolve
    // everything in this InitOutPort's chain.  Order independent: the bound
    // endpoint and any chain extensions may have been added before or after
    // this call.
    void connectToInPort(InitInPort<ParamTypes...> &inport) {
        assert(inport.isDangling() == false);
        assert(isDangling() == false);
        assert(cross_inport_ == nullptr);  // each InitOutPort holds at most one cross-link

        cross_inport_ = &inport;
        inport.addCrossOutport(this);

        tryResolveChain_(nullptr);
    }


    //// InitOutPort↔InitOutPort chain plumbing (don't call directly, use murm::connect()) ////

    // Append an undirected chain edge from this InitOutPort to peer.
    void addChainPeer(InitOutPort<ParamTypes...> *peer) {
        chain_peers_.push_back(peer);
    }

    // Walk this InitOutPort's chain and ask each peer to (re)resolve.
    // Idempotent and safe to call repeatedly: already-resolved InitOutPorts
    // short-circuit immediately.
    void tryResolveChain_(InitOutPort<ParamTypes...> *came_from) {
        tryResolveSelf_();
        for (auto *peer : chain_peers_) {
            if (peer == came_from) continue;
            peer->tryResolveChain_(this);
        }
    }


    // Mark when this InitOutPort has been connected (legacy single-port API;
    // kept for backwards compatibility with code outside the chain machinery).
    void setConnected(bool connected = true) {
        if (connected) {
            assert(connected_ == false);
            assert(dangling_ == false);
        }
        connected_ = connected;
    }
    bool isConnected() { return connected_; }

    // Set whether this is an immediate connection.  Each InitOutPort in a
    // chain keeps its own immediate_ setting (since they may live in different
    // components); only the delegate is shared.
    void setImmediate(bool immediate = true) {
        immediate_ = immediate;
    }
    bool isImmediate() { return immediate_; }
};


// Out-of-line definition of InitInPort::notifyChainCrossOutports_, deferred
// until InitOutPort is fully declared so that we can call its methods on the
// cross-linked InitOutPort pointers.
template<typename... ParamTypes>
void InitInPort<ParamTypes...>::notifyChainCrossOutports_(InitInPort<ParamTypes...> *came_from) {
    for (auto *outport : cross_outports_) {
        outport->tryResolveChain_(nullptr);
    }
    for (auto *peer : chain_peers_) {
        if (peer == came_from) continue;
        peer->notifyChainCrossOutports_(this);
    }
}

} // namespace murm

// Have to put this at the end so that both InitInPort and InitOutPort are defined for the connect function
#include "InitInPortInitOutPort_connect.hpp"

#endif // __MURM_INIT_OUTPORT_HPP__
