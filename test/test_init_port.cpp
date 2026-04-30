// Tests for InitInPort / InitOutPort: binding, cross-link, chains in both
// orderings, immediate vs queued init delivery, dangling.

#include <vector>

#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "Component.hpp"
#include "InitInPort.hpp"
#include "InitOutPort.hpp"


// A "sink" component with a bound InitInPort that records every value it
// receives.
class InitSink : public murm::Component {
public:
    std::vector<int> received;

    murm::InitInPort<int> in_port { this };

    InitSink(murm::Component *parent, const std::string &name)
        : Component(parent, "InitSink", name)
    {
        in_port.bindToMethod(this, &InitSink::handle);
    }

    void handle(int v) { received.push_back(v); }
};


// A source with two InitOutPorts: one immediate, one queued.
class InitSource : public murm::Component {
public:
    murm::InitOutPort<int> out_immediate { this, /*immediate=*/true };
    murm::InitOutPort<int> out_queued    { this, /*immediate=*/false };

    InitSource(murm::Component *parent, const std::string &name)
        : Component(parent, "InitSource", name) {}
};


// Hierarchical wrapper exposing an InitInPort and an InitOutPort and
// containing inner sinks/sources, used to test chaining.
class InitOuter : public murm::Component {
public:
    InitSink   inner_sink   { this, "inner_sink" };
    InitSource inner_source { this, "inner_source" };

    murm::InitInPort<int>  in_port  { this };
    murm::InitOutPort<int> out_port { this, /*immediate=*/true };

    InitOuter(murm::Component *parent, const std::string &name)
        : Component(parent, "InitOuter", name) {}
};


//// Direct cross-link, immediate ////////////////////////////////////////////

TEST(immediate_init_outport_delivers_synchronously) {
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    InitSource src(&root, "src");
    InitSink   sink(&root, "sink");
    murm::connect(src.out_immediate, sink.in_port);

    src.out_immediate(123);
    // Immediate ports invoke the bound delegate synchronously; no init() needed.
    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ(123, sink.received[0]);
}

TEST(immediate_init_outport_works_with_args_swapped) {
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    InitSource src(&root, "src");
    InitSink   sink(&root, "sink");
    murm::connect(sink.in_port, src.out_immediate);   // InPort first

    src.out_immediate(123);
    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ(123, sink.received[0]);
}


//// Direct cross-link, queued ////////////////////////////////////////////////

TEST(queued_init_outport_does_not_fire_until_init_runs) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    InitSource src(&root, "src");
    InitSink   sink(&root, "sink");
    murm::connect(src.out_queued, sink.in_port);

    src.out_queued(7);
    src.out_queued(8);
    ASSERT_EQ((size_t)0, sink.received.size());

    em.init();

    ASSERT_EQ((size_t)2, sink.received.size());
    ASSERT_EQ(7, sink.received[0]);
    ASSERT_EQ(8, sink.received[1]);
}


//// Chaining (order-independent) /////////////////////////////////////////////

TEST(init_chain_natural_order) {
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    InitOuter  outer(&root, "outer");
    InitSource src(&root, "src");
    InitSink   sink(&root, "sink");

    // Inner wiring (natural data-flow order).
    murm::connect(outer.in_port,  outer.inner_sink.in_port);
    murm::connect(outer.inner_source.out_immediate, outer.out_port);

    // External wiring.
    murm::connect(src.out_immediate, outer.in_port);
    murm::connect(outer.out_port,    sink.in_port);

    src.out_immediate(11);
    outer.inner_source.out_immediate(22);

    ASSERT_EQ((size_t)1, outer.inner_sink.received.size());
    ASSERT_EQ(11, outer.inner_sink.received[0]);

    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ(22, sink.received[0]);
}

TEST(init_chain_reversed_args) {
    // Same topology, every connect() reversed.
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    InitOuter  outer(&root, "outer");
    InitSource src(&root, "src");
    InitSink   sink(&root, "sink");

    murm::connect(outer.inner_sink.in_port, outer.in_port);
    murm::connect(outer.out_port, outer.inner_source.out_immediate);

    murm::connect(outer.in_port,  src.out_immediate);
    murm::connect(sink.in_port,   outer.out_port);

    src.out_immediate(11);
    outer.inner_source.out_immediate(22);

    ASSERT_EQ((size_t)1, outer.inner_sink.received.size());
    ASSERT_EQ(11, outer.inner_sink.received[0]);
    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ(22, sink.received[0]);
}

TEST(init_chain_built_before_bind) {
    // Build the entire topology before any port has a bound delegate, then
    // bind the sink's InitInPort last and verify everything still wires up.
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    InitOuter outer(&root, "outer");
    InitSource src(&root, "src");

    // Build a separate sink whose binding we control manually.
    class LateBindSink : public murm::Component {
    public:
        std::vector<int> received;
        murm::InitInPort<int> in_port { this };
        LateBindSink(murm::Component *parent, const std::string &name)
            : Component(parent, "LateBindSink", name) {}
        void handle(int v) { received.push_back(v); }
    };
    LateBindSink sink(&root, "sink");

    // Set up all chain edges first.
    murm::connect(outer.in_port,  outer.inner_sink.in_port);
    murm::connect(outer.inner_source.out_immediate, outer.out_port);
    murm::connect(src.out_immediate, outer.in_port);
    murm::connect(outer.out_port,    sink.in_port);

    // Now bind the sink's delegate (after every chain/cross has been added).
    sink.in_port.bindToMethod(&sink, &LateBindSink::handle);

    outer.inner_source.out_immediate(42);

    ASSERT_EQ((size_t)1, sink.received.size());
    ASSERT_EQ(42, sink.received[0]);
}


//// Dangling /////////////////////////////////////////////////////////////////

TEST(dangling_init_inport) {
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    InitSink sink(&root, "sink");
    sink.in_port.setDangling();
    ASSERT_TRUE(sink.in_port.isDangling());
    ASSERT_FALSE(sink.in_port.isConnected());
}

TEST(dangling_init_outport) {
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    InitSource src(&root, "src");
    src.out_immediate.setDangling();
    ASSERT_TRUE(src.out_immediate.isDangling());
    ASSERT_FALSE(src.out_immediate.isConnected());
}


//// bindToIndexedMethod //////////////////////////////////////////////////////

class IndexedInitSink : public murm::Component {
public:
    std::vector<std::pair<int,int>> received;
    murm::InitInPort<int> in0 { this };
    murm::InitInPort<int> in1 { this };

    IndexedInitSink(murm::Component *parent, const std::string &name)
        : Component(parent, "IndexedInitSink", name)
    {
        in0.bindToIndexedMethod(0, this, &IndexedInitSink::handle);
        in1.bindToIndexedMethod(1, this, &IndexedInitSink::handle);
    }

    void handle(int idx, int v) { received.emplace_back(idx, v); }
};

TEST(init_bindToIndexedMethod_passes_index_to_handler) {
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    InitSource src0(&root, "src0");
    InitSource src1(&root, "src1");
    IndexedInitSink sink(&root, "sink");

    murm::connect(src0.out_immediate, sink.in0);
    murm::connect(src1.out_immediate, sink.in1);

    src0.out_immediate(11);
    src1.out_immediate(22);

    ASSERT_EQ((size_t)2, sink.received.size());
    ASSERT_EQ(0, sink.received[0].first);  ASSERT_EQ(11, sink.received[0].second);
    ASSERT_EQ(1, sink.received[1].first);  ASSERT_EQ(22, sink.received[1].second);
}


RUN_ALL_TESTS()
