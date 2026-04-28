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


#ifndef __MURM_OUTPORT_HPP__
#define __MURM_OUTPORT_HPP__

#include <cassert>
#include <vector>
#include "marcmo/SRDelegate.hpp"
#include "Component.hpp"
#include "EventManager.hpp"
#include "InPort.hpp"
#include "connect.hpp"
#include "BetweenThreadQueue.hpp"

namespace murm {

// OutPort class
// (can be cross-linked to an InPort and/or chained to other OutPorts in a hierarchical
//  component arrangement; chain links are bidirectional so that the order of connect()
//  calls does not matter)
// ParamTypes are the types of data that are passed through the port
template<typename... ParamTypes>
class OutPort {
public:
    typedef generic::delegate<void(ParamTypes...)> DelegateType;

private:
    Component *owner_ {nullptr};
    EventManager *event_manager_ {nullptr};
    BetweenThreadQueue<EventManager::EventType> *btqueue_ {nullptr};
    bool remote_ {false};  // True if the InPort on the "other end" belongs to a different EventManager

    // The delegate (copied over when the chain of connection ends in an InPort bound to a method)
    DelegateType del_;
    uint64_t delta_time_ {0}; // Total delay of the link (after connected to an InPort)

    bool connected_ {false};  // true if this OutPort has been resolved to a delegate
    bool dangling_ {false}; // true if this is intentionally left dangling (unconnected)

    // Support for OutPort↔OutPort chaining (hierarchical components).
    // The chain is stored as an undirected adjacency list: when two OutPorts are
    // connected via murm::connect(a, b, delay), each of a and b records the other
    // here.  Resolution walks the resulting tree at connection time.
    struct ChainPeer {
        OutPort<ParamTypes...> *peer;
        uint64_t delay;
    };
    std::vector<ChainPeer> chain_peers_;

    // Cross-link to an InPort.  At most one OutPort in any given OutPort chain may
    // hold a cross-link (multiple cross-links in a single chain would be ambiguous).
    InPort<ParamTypes...> *cross_inport_ {nullptr};
    uint64_t cross_delay_ {0};


    // Configure this single OutPort with a resolved delegate and delay.
    // Called once we have walked the chains and know what to wire up.
    void configureSelf_(InPort<ParamTypes...> &bound_inport, uint64_t total_delay) {
        assert(connected_ == false);
        assert(dangling_ == false);

        setDelegate(bound_inport.getDelegate());
        setDeltaTime(total_delay);

        if (event_manager_ == bound_inport.getEventManager()) {
            // Same EventManager — no extra setup needed
        } else {
            remote_ = true;
            ThreadedEventManager *source_tem = dynamic_cast<ThreadedEventManager *>(event_manager_);
            ThreadedEventManager *dest_tem = dynamic_cast<ThreadedEventManager *>(bound_inport.getEventManager());
            btqueue_ = dest_tem->getBetweenThreadQueueFrom(source_tem);
            btqueue_->updateDelay(total_delay);
        }

        setConnected();
    }

    // Walk this OutPort's chain (undirected, tree-shaped) to find the unique anchor:
    // the OutPort that holds the cross-link into an InPort.  Returns nullptr if no
    // OutPort in this chain has been cross-linked yet.
    OutPort<ParamTypes...> *findAnchor_(OutPort<ParamTypes...> *came_from) {
        if (cross_inport_ != nullptr) return this;
        for (auto &link : chain_peers_) {
            if (link.peer == came_from) continue;
            OutPort<ParamTypes...> *found = link.peer->findAnchor_(this);
            if (found != nullptr) return found;
        }
        return nullptr;
    }

    // Walk the chain from this OutPort to target, accumulating per-edge delay along
    // the unique tree path.  Returns true on success.
    bool findChainDistanceTo_(OutPort<ParamTypes...> *target, uint64_t &accumulated,
                              OutPort<ParamTypes...> *came_from) {
        if (this == target) return true;
        for (auto &link : chain_peers_) {
            if (link.peer == came_from) continue;
            uint64_t branch = accumulated + link.delay;
            if (link.peer->findChainDistanceTo_(target, branch, this)) {
                accumulated = branch;
                return true;
            }
        }
        return false;
    }

    // Try to resolve this single OutPort.  No-op if already connected, or if the
    // chains aren't yet far enough along to determine a delegate.
    void tryResolveSelf_() {
        if (connected_) return;
        if (dangling_) return;

        OutPort<ParamTypes...> *anchor = findAnchor_(nullptr);
        if (anchor == nullptr) return; // no cross-link in this OutPort chain yet

        uint64_t inport_delay = 0;
        InPort<ParamTypes...> *bound_inport =
            anchor->cross_inport_->findBoundEnd_(inport_delay, nullptr);
        if (bound_inport == nullptr) return; // no bound delegate in the InPort chain yet

        uint64_t my_chain_delay = 0;
        bool ok = anchor->findChainDistanceTo_(this, my_chain_delay, nullptr);
        assert(ok);

        uint64_t total_delay = my_chain_delay + anchor->cross_delay_ + inport_delay;
        configureSelf_(*bound_inport, total_delay);

        // Idempotently mark every InPort in the chain as connected too.
        anchor->cross_inport_->markChainConnected();
    }


public:
    // Constructor
    OutPort(Component *owner) :
        owner_(owner),
        event_manager_(owner->getEventManager())
    {}

    // Copy constructor
    OutPort(const OutPort&) = default;

    EventManager *getEventManager() { return event_manager_; }

    // The OutPort is a functor:
    void operator()(ParamTypes... params) {
        assert(connected_ == true);
        uint64_t new_time = event_manager_->getTime() + delta_time_;
        // Create an event (Event(new_time, del_, params...)) and enqueue it to the EventHandler or BetweenThreadQueue
        //event_creator_(new_time, del_, std::forward<ParamTypes>(params)...);
        if (remote_) {
            btqueue_->emplace(new_time, del_, std::forward<ParamTypes>(params)...);
        } else {
            event_manager_->emplace(new_time, del_, std::forward<ParamTypes>(params)...);
        }
    }

    // Mark if this OutPort is intentionally left dangling (unconnected)
    void setDangling(bool dangling = true) {
        // Make sure that it's not already connected
        if (dangling) { assert(connected_ == false); }
        dangling_ = dangling;
    }
    bool isDangling() { return dangling_; }

    // Internal setters used during resolution.
    // (don't call these directly, use "murm::connect(in_port, out_port);" instead)
    void setDelegate(const DelegateType &new_del) { del_ = new_del; }
    void setDeltaTime(uint64_t total_delta) { delta_time_ = total_delta; }


    //// OutPort↔InPort cross-link plumbing (don't call directly, use murm::connect()) ////

    // Record a cross-link from this OutPort to inport with the given edge delay,
    // then try to resolve everything in this OutPort's chain.  Order independent:
    // the bound endpoint and any chain extensions may have been added before or
    // after this call.
    void connectToInPort(InPort<ParamTypes...> &inport, uint64_t delay) {
        assert(inport.isDangling() == false);
        assert(isDangling() == false);
        assert(cross_inport_ == nullptr);  // each OutPort holds at most one cross-link

        cross_inport_ = &inport;
        cross_delay_ = delay;
        inport.addCrossOutport(this);

        tryResolveChain_(nullptr);
    }


    //// OutPort↔OutPort chain plumbing (don't call directly, use murm::connect()) ////

    // Append an undirected chain edge from this OutPort to peer (with the given delay).
    // Connector<OutPort, OutPort> calls this on both ports so the chain is symmetric.
    void addChainPeer(OutPort<ParamTypes...> *peer, uint64_t delay) {
        chain_peers_.push_back({peer, delay});
    }

    // Walk this OutPort's chain and ask each peer to (re)resolve.  Idempotent and
    // safe to call repeatedly: already-resolved OutPorts short-circuit immediately.
    void tryResolveChain_(OutPort<ParamTypes...> *came_from) {
        tryResolveSelf_();
        for (auto &link : chain_peers_) {
            if (link.peer == came_from) continue;
            link.peer->tryResolveChain_(this);
        }
    }


    // Mark when this OutPort has been connected (legacy single-port API; kept for
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


// Out-of-line definition of InPort::notifyChainCrossOutports_, deferred until
// OutPort is fully declared so that we can call its methods on the cross-linked
// OutPort pointers.
template<typename... ParamTypes>
void InPort<ParamTypes...>::notifyChainCrossOutports_(InPort<ParamTypes...> *came_from) {
    for (auto *outport : cross_outports_) {
        outport->tryResolveChain_(nullptr);
    }
    for (auto &link : chain_peers_) {
        if (link.peer == came_from) continue;
        link.peer->notifyChainCrossOutports_(this);
    }
}

} // namespace murm

// Have to put this at the end so that both InPort and OutPort are defined for the connect function
#include "InPortOutPort_connect.hpp"

#endif // __MURM_OUTPORT_HPP__
