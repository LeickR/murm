// Tests for Bundle and the std::vector<>/std::vector<> connect specialization.

#include <vector>

#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "Component.hpp"
#include "InPort.hpp"
#include "OutPort.hpp"
#include "Bundle.hpp"


// A component containing N InPorts and N OutPorts (for vector-vector tests),
// and a parallel "received" log per InPort.
class VecHost : public murm::Component {
public:
    int n;
    std::vector<murm::InPort<int>>  ins;
    std::vector<murm::OutPort<int>> outs;
    std::vector<std::vector<int>>   received;

    VecHost(murm::Component *parent, const std::string &name, int num)
        : Component(parent, "VecHost", name), n(num)
    {
        ins.reserve(n);
        outs.reserve(n);
        received.assign(n, {});
        for (int i = 0; i < n; ++i) {
            ins.emplace_back(this);
            outs.emplace_back(this);
        }
        for (int i = 0; i < n; ++i) {
            ins[i].bindToIndexedMethod(i, this, &VecHost::handle);
        }
    }

    void handle(int idx, int v) { received[idx].push_back(v); }
};


// A component containing two Bundles -- one of OutPorts, one of InPorts.
// Each InPort has its own bound handler.
class BundleHost : public murm::Component {
public:
    int n;
    Bundle<murm::OutPort<int>> outs;
    Bundle<murm::InPort<int>>  ins;
    std::vector<std::vector<int>> received;

    BundleHost(murm::Component *parent, const std::string &name, int num)
        : Component(parent, "BundleHost", name),
          n(num),
          outs(num, /*OutPort ctor arg:*/ this),
          ins (num, /*InPort  ctor arg:*/ this)
    {
        received.assign(n, {});
        for (int i = 0; i < n; ++i) {
            ins[i].bindToIndexedMethod(i, this, &BundleHost::handle);
        }
    }

    void handle(int idx, int v) { received[idx].push_back(v); }
};


//// Bundle basics ////////////////////////////////////////////////////////////

TEST(bundle_size_and_indexing) {
    murm_test::reset_eventmanager();
    murm::TopLevelComponent root(&murm::EventManager::getDefaultEventManager());

    BundleHost host(&root, "host", 4);
    ASSERT_EQ((size_t)4, host.outs.size());
    ASSERT_EQ((size_t)4, host.ins.size());
}


//// std::vector<InPort> ↔ std::vector<OutPort> via Connector specialization //

TEST(vector_to_vector_pairwise_connect) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    VecHost a(&root, "a", 3);
    VecHost b(&root, "b", 3);

    // a.outs[i] -> b.ins[i] for i = 0..2, all with delay 4.
    murm::connect(a.outs, b.ins, 4);

    a.outs[0](100);
    a.outs[1](200);
    a.outs[2](300);
    em.run_to(50);

    ASSERT_EQ((size_t)1, b.received[0].size());  ASSERT_EQ(100, b.received[0][0]);
    ASSERT_EQ((size_t)1, b.received[1].size());  ASSERT_EQ(200, b.received[1][0]);
    ASSERT_EQ((size_t)1, b.received[2].size());  ASSERT_EQ(300, b.received[2][0]);

    // Cross-talk check: nothing went to a.received.
    for (int i = 0; i < 3; ++i) ASSERT_EQ((size_t)0, a.received[i].size());
}


//// Bundle ↔ Bundle connect (delegates to vector-vector) /////////////////////

TEST(bundle_to_bundle_pairwise_connect) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    murm::TopLevelComponent root(&em);

    BundleHost src(&root, "src", 3);
    BundleHost dst(&root, "dst", 3);

    // For dst's InPorts to be considered the receivers, we need the OutPorts
    // on src to point to InPorts on dst.  src.outs[i] -> dst.ins[i].
    murm::connect(src.outs, dst.ins, 2);

    src.outs[0](10);
    src.outs[1](20);
    src.outs[2](30);
    em.run_to(50);

    ASSERT_EQ((size_t)1, dst.received[0].size());  ASSERT_EQ(10, dst.received[0][0]);
    ASSERT_EQ((size_t)1, dst.received[1].size());  ASSERT_EQ(20, dst.received[1][0]);
    ASSERT_EQ((size_t)1, dst.received[2].size());  ASSERT_EQ(30, dst.received[2][0]);
}


RUN_ALL_TESTS()
