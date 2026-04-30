// Tests for ThreadedEventMaster.
//
// Notes on isolation:
//   * The master is a singleton, and `createThreads()` cannot be safely
//     called more than once per process (it appends to a vector instead of
//     replacing it).  So every test in this binary lives under a single
//     `createThreads(NUM_TEST_THREADS)` call performed in `main()` before any
//     tests run.
//   * Per-thread EventManagers can be reset between tests via
//     `reset_master_eventmanagers()`.

#include <vector>

#include "test_framework.hpp"
#include "Component.hpp"
#include "EventManager.hpp"
#include "ScheduledMethod.hpp"
#include "ThreadedEventMaster.hpp"


namespace {
constexpr int NUM_TEST_THREADS = 2;

// Reset every per-thread EventManager so the next test starts with a clean
// slate.  This is the multi-thread analogue of murm_test::reset_eventmanager.
void reset_master_eventmanagers() {
    auto &master = murm::ThreadedEventMaster::getDefaultThreadedEventMaster();
    for (int i = 0; i < master.getNumThreads(); ++i) {
        master.getEventManagerForThread(i)->reset();
    }
}

} // anonymous namespace


// A logger that records callback time + value, used identically on every
// thread.
class Logger : public murm::Component {
public:
    std::vector<std::pair<uint64_t, int>> log;
    murm::ScheduledMethod<int> sched { this };

    Logger(murm::Component *parent, const std::string &name)
        : Component(parent, "Logger", name)
    {
        sched.bindToMethod(this, &Logger::handle);
    }

    void handle(int v) { log.emplace_back(getTime(), v); }
};


//// Master metadata //////////////////////////////////////////////////////////

TEST(getMaxNumThreads_is_positive) {
    auto &master = murm::ThreadedEventMaster::getDefaultThreadedEventMaster();
    ASSERT_TRUE(master.getMaxNumThreads() >= 1);
}

TEST(createThreads_set_thread_count) {
    auto &master = murm::ThreadedEventMaster::getDefaultThreadedEventMaster();
    ASSERT_EQ(NUM_TEST_THREADS, master.getNumThreads());
}

TEST(per_thread_eventmanagers_accessible_and_distinct) {
    auto &master = murm::ThreadedEventMaster::getDefaultThreadedEventMaster();
    auto *em0 = master.getEventManagerForThread(0);
    auto *em1 = master.getEventManagerForThread(1);
    ASSERT_TRUE(em0 != nullptr);
    ASSERT_TRUE(em1 != nullptr);
    ASSERT_TRUE(em0 != em1);
    // Thread 0's EM should be the global singleton.
    ASSERT_TRUE(em0 == &murm::EventManager::getDefaultEventManager());
}


//// run_to drives every per-thread EM ////////////////////////////////////////

TEST(master_run_to_fires_events_on_every_thread) {
    reset_master_eventmanagers();
    auto &master = murm::ThreadedEventMaster::getDefaultThreadedEventMaster();
    auto *em0 = master.getEventManagerForThread(0);
    auto *em1 = master.getEventManagerForThread(1);

    // Each thread gets its own root + its own logger.
    murm::TopLevelComponent root0(em0);
    murm::TopLevelComponent root1(em1);

    Logger l0(&root0, "l0");
    Logger l1(&root1, "l1");

    l0.sched(10, 100);
    l0.sched(40, 400);
    l1.sched(20, 200);
    l1.sched(30, 300);

    master.run_to(100);

    ASSERT_EQ((size_t)2, l0.log.size());
    ASSERT_EQ((uint64_t)10, l0.log[0].first);  ASSERT_EQ(100, l0.log[0].second);
    ASSERT_EQ((uint64_t)40, l0.log[1].first);  ASSERT_EQ(400, l0.log[1].second);

    ASSERT_EQ((size_t)2, l1.log.size());
    ASSERT_EQ((uint64_t)20, l1.log[0].first);  ASSERT_EQ(200, l1.log[0].second);
    ASSERT_EQ((uint64_t)30, l1.log[1].first);  ASSERT_EQ(300, l1.log[1].second);
}


//// init() can be called even when nothing is queued ////////////////////////

TEST(master_init_then_run_to_works) {
    reset_master_eventmanagers();
    auto &master = murm::ThreadedEventMaster::getDefaultThreadedEventMaster();
    auto *em0 = master.getEventManagerForThread(0);
    auto *em1 = master.getEventManagerForThread(1);

    murm::TopLevelComponent root0(em0);
    murm::TopLevelComponent root1(em1);
    Logger l0(&root0, "l0");
    Logger l1(&root1, "l1");

    master.init();    // no init events queued; should be a no-op
    ASSERT_EQ((uint64_t)0, em0->getTime());
    ASSERT_EQ((uint64_t)0, em1->getTime());

    l0.sched(5, 1);
    l1.sched(7, 2);
    master.run_to(50);
    ASSERT_EQ((size_t)1, l0.log.size());
    ASSERT_EQ((size_t)1, l1.log.size());
}


// NOTE: cross-thread port delivery via BetweenThreadQueue is not exercised
// here.  The BTQ uses a rolling-roadblock + spin protocol that is sensitive
// to event ordering across threads, and writing a deterministic test that
// reliably terminates without instrumenting the BTQ internals is out of
// scope for this suite.  The single-thread and per-thread-EventManager
// scaffolding above is sufficient to verify that the master correctly
// dispatches work on the EMs it owns.


// Custom entry point: install threads exactly once before running tests.
int main() {
    auto &master = murm::ThreadedEventMaster::getDefaultThreadedEventMaster();
    if (master.getMaxNumThreads() < NUM_TEST_THREADS) {
        std::cerr << "test_threading.cpp: needs at least "
                  << NUM_TEST_THREADS << " OpenMP threads, but only "
                  << master.getMaxNumThreads() << " available; skipping.\n";
        return 0;
    }
    master.createThreads(NUM_TEST_THREADS);
    return ::murm_test::run_all(__FILE__);
}
