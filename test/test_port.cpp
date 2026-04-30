// Tests for InPort / OutPort: binding, cross-link, chains in both orderings,
// dangling, multi-OutPort feeding into a shared chain, delays.

#include <vector>

#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "Component.hpp"
#include "InPort.hpp"
#include "OutPort.hpp"


// A "sink" component with a bound InPort that records every value it receives,
// along with the simulation time at which each call was processed.
class Sink : public murm::Component {
public:
    std::vector<std::pair<uint64_t, int>> received;
    murm::InPort<int> in_port { this };

    Sink(murm::Component *parent, const std::string &name)
        : Component(parent, "Sink", name)
    {
        in_port.bindToMethod(this, &Sink::handle);
    }

    void handle(int v) { received.emplace_back(getTime(), v); }
};


// A "source" component with a single OutPort.
class Source : public murm::Component {
public:
    murm::OutPort<int> out_port { this };

    Source(murm::Component *parent, const std::string &name)
        : Component(parent, "Source", name) {}
};


// A hierarchical "outer" component that exposes its own InPort and OutPort
// and contains an inner Sink and Source.  Used to test InPort↔InPort and
// OutPort↔OutPort chaining.
class Outer : public murm::Component {
public:
    Sink   inner_sink   { this, "inner_sink" };
    Source inner_source { this, "inner_source" };

    murm::InPort<int>  in_port  { this };
    murm::OutPort<int> out_port { this };

    Outer(murm::Component *parent, const std::string &name)
        : Component(parent, "Outer", name) {}
};


//// Direct OutPort → InPort cross-link ///////////////////////////////////////

TEST(cross_in_to_out_form_works) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Source src(&root, "src");
    Sink   sink(&root, "sink");
    murm::connect(src.out_port, sink.in_port, 5);  // OutPort first

    src.out_port(99);
    em.run_to(100);

    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ((uint64_t)5, sink.received[0].first);
    ASSERT_EQ(99, sink.received[0].second);
}

TEST(cross_in_to_out_form_works_with_args_swapped) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Source src(&root, "src");
    Sink   sink(&root, "sink");
    murm::connect(sink.in_port, src.out_port, 5);  // InPort first

    src.out_port(99);
    em.run_to(100);

    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ((uint64_t)5, sink.received[0].first);
    ASSERT_EQ(99, sink.received[0].second);
}

TEST(connect_state_flags_after_cross_link) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Source src(&root, "src");
    Sink   sink(&root, "sink");
    ASSERT_FALSE(src.out_port.isConnected());
    ASSERT_FALSE(sink.in_port.isConnected());

    murm::connect(src.out_port, sink.in_port, 0);

    ASSERT_TRUE(src.out_port.isConnected());
    ASSERT_TRUE(sink.in_port.isConnected());
}


//// InPort↔InPort and OutPort↔OutPort chains, in both orderings //////////////

TEST(chain_inport_outport_works_natural_order) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Outer  outer(&root, "outer");
    Source src(&root, "src");
    Sink   sink(&root, "sink");

    // Inner wiring (natural data-flow order).
    murm::connect(outer.in_port,  outer.inner_sink.in_port,    1);
    murm::connect(outer.inner_source.out_port, outer.out_port, 2);

    // External wiring.
    murm::connect(src.out_port,  outer.in_port,        10);
    murm::connect(outer.out_port, sink.in_port,        20);

    src.out_port(7);
    outer.inner_source.out_port(8);
    em.run_to(200);

    // src(7) → outer.in (delay 10) → outer.inner_sink.in (delay 1) total 11
    ASSERT_EQ((size_t)1, outer.inner_sink.received.size());
    ASSERT_EQ((uint64_t)11, outer.inner_sink.received[0].first);
    ASSERT_EQ(7, outer.inner_sink.received[0].second);

    // outer.inner_source(8) → outer.out (delay 2) → sink.in (delay 20) total 22
    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ((uint64_t)22, sink.received[0].first);
    ASSERT_EQ(8, sink.received[0].second);
}

TEST(chain_inport_outport_works_reversed_args) {
    // Same topology as above, but every chain connect() has its args swapped
    // and the cross-link is also written backwards.  Result should be byte-
    // identical because the chain machinery is order-symmetric.
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Outer  outer(&root, "outer");
    Source src(&root, "src");
    Sink   sink(&root, "sink");

    murm::connect(outer.inner_sink.in_port,    outer.in_port,  1);  // swapped
    murm::connect(outer.out_port, outer.inner_source.out_port, 2);  // swapped

    murm::connect(outer.in_port, src.out_port,        10);          // swapped (in/out)
    murm::connect(sink.in_port,  outer.out_port,      20);          // swapped (in/out)

    src.out_port(7);
    outer.inner_source.out_port(8);
    em.run_to(200);

    ASSERT_EQ((size_t)1, outer.inner_sink.received.size());
    ASSERT_EQ((uint64_t)11, outer.inner_sink.received[0].first);
    ASSERT_EQ(7, outer.inner_sink.received[0].second);

    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ((uint64_t)22, sink.received[0].first);
    ASSERT_EQ(8, sink.received[0].second);
}

TEST(chain_inport_outport_works_with_chain_built_after_cross) {
    // Build the cross-link first, then add the chain edges -- the chain
    // machinery must back-fill the resolution.
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Outer  outer(&root, "outer");
    Source src(&root, "src");

    // Cross-link first (note: outer.in_port and outer.out_port are not yet
    // chained to anything bound).
    murm::connect(src.out_port, outer.in_port, 10);

    // Now chain the outer.in_port to the actually-bound inner_sink.in_port.
    murm::connect(outer.in_port, outer.inner_sink.in_port, 1);

    src.out_port(7);
    em.run_to(200);

    ASSERT_EQ((size_t)1, outer.inner_sink.received.size());
    ASSERT_EQ((uint64_t)11, outer.inner_sink.received[0].first);
    ASSERT_EQ(7, outer.inner_sink.received[0].second);
}


//// Multi-OutPort feeding into the same InPort chain /////////////////////////

TEST(two_outports_share_one_inport_chain) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Source srcA(&root, "srcA");
    Source srcB(&root, "srcB");
    Sink   sink(&root, "sink");

    murm::connect(srcA.out_port, sink.in_port, 3);
    murm::connect(srcB.out_port, sink.in_port, 7);

    srcA.out_port(100);
    srcB.out_port(200);
    em.run_to(50);

    ASSERT_EQ((size_t)2, sink.received.size());
    // srcA fires first (delay 3, before srcB at delay 7).
    ASSERT_EQ((uint64_t)3, sink.received[0].first);
    ASSERT_EQ(100, sink.received[0].second);
    ASSERT_EQ((uint64_t)7, sink.received[1].first);
    ASSERT_EQ(200, sink.received[1].second);
}


//// Dangling ports ///////////////////////////////////////////////////////////

TEST(dangling_inport_marked_dangling) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Sink sink(&root, "sink");
    sink.in_port.setDangling();
    ASSERT_TRUE(sink.in_port.isDangling());
    ASSERT_FALSE(sink.in_port.isConnected());
}

TEST(dangling_outport_marked_dangling) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Source src(&root, "src");
    src.out_port.setDangling();
    ASSERT_TRUE(src.out_port.isDangling());
    ASSERT_FALSE(src.out_port.isConnected());
}


//// bindToIndexedMethod //////////////////////////////////////////////////////

class IndexedSink : public murm::Component {
public:
    std::vector<std::pair<int,int>> received;   // (index, value) pairs
    murm::InPort<int> in0 { this };
    murm::InPort<int> in1 { this };

    IndexedSink(murm::Component *parent, const std::string &name)
        : Component(parent, "IndexedSink", name)
    {
        in0.bindToIndexedMethod(0, this, &IndexedSink::handle);
        in1.bindToIndexedMethod(1, this, &IndexedSink::handle);
    }

    void handle(int idx, int v) { received.emplace_back(idx, v); }
};

TEST(bindToIndexedMethod_passes_index_to_handler) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    Source src0(&root, "src0");
    Source src1(&root, "src1");
    IndexedSink sink(&root, "sink");

    murm::connect(src0.out_port, sink.in0, 0);
    murm::connect(src1.out_port, sink.in1, 0);

    src0.out_port(11);
    src1.out_port(22);
    em.run_to(50);

    ASSERT_EQ((size_t)2, sink.received.size());
    ASSERT_EQ(0,  sink.received[0].first);   ASSERT_EQ(11, sink.received[0].second);
    ASSERT_EQ(1,  sink.received[1].first);   ASSERT_EQ(22, sink.received[1].second);
}


RUN_ALL_TESTS()
