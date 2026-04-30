// Tests for EventManager and ScheduledMethod.

#include <string>
#include <vector>

#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "Component.hpp"
#include "EventManager.hpp"
#include "ScheduledMethod.hpp"
#include "InitMethod.hpp"


// A component that exposes scheduled and init methods that just append to a
// log when called.  Used by the tests below to observe execution order.
class Logger : public murm::Component {
public:
    std::vector<std::pair<uint64_t, int>> log;     // (time, value) of each call

    murm::ScheduledMethod<int> sched_recv { this };
    murm::InitMethod<int>      init_recv  { this };

    Logger(murm::Component *parent, const std::string &name)
        : Component(parent, "Logger", name)
    {
        sched_recv.bindToMethod(this, &Logger::handleScheduled);
        init_recv.bindToMethod(this, &Logger::handleInit);
    }

    void handleScheduled(int v) { log.emplace_back(getTime(), v); }
    void handleInit(int v)      { log.emplace_back(getTime(), v); }
};


//// EventManager basic plumbing //////////////////////////////////////////////

TEST(eventmanager_starts_at_time_zero) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    ASSERT_EQ((uint64_t)0, em.getTime());
}

TEST(eventmanager_run_to_advances_time) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();
    em.run_to(100);
    ASSERT_EQ((uint64_t)100, em.getTime());
}


//// ScheduledMethod fires at the right time //////////////////////////////////

TEST(scheduled_method_fires_at_relative_time) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();

    murm::TopLevelComponent root(&em);
    Logger logger(&root, "log");

    logger.sched_recv(10, 42);
    em.run_to(100);

    ASSERT_EQ((size_t)1, logger.log.size());
    ASSERT_EQ((uint64_t)10, logger.log[0].first);
    ASSERT_EQ(42, logger.log[0].second);
}

TEST(scheduled_method_multiple_events_in_time_order) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();

    murm::TopLevelComponent root(&em);
    Logger logger(&root, "log");

    // Schedule out of order; the event manager must replay them in time order.
    logger.sched_recv(20, 200);
    logger.sched_recv(5,  50);
    logger.sched_recv(15, 150);
    em.run_to(100);

    ASSERT_EQ((size_t)3, logger.log.size());
    ASSERT_EQ((uint64_t)5,  logger.log[0].first);  ASSERT_EQ(50,  logger.log[0].second);
    ASSERT_EQ((uint64_t)15, logger.log[1].first);  ASSERT_EQ(150, logger.log[1].second);
    ASSERT_EQ((uint64_t)20, logger.log[2].first);  ASSERT_EQ(200, logger.log[2].second);
}

TEST(scheduled_method_relative_to_current_time) {
    // ScheduledMethod's "time" argument is relative to the current EM time:
    // an event scheduled with delay=5 from t=10 fires at t=15, not t=5.
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();

    murm::TopLevelComponent root(&em);
    Logger logger(&root, "log");

    em.run_to(10);
    ASSERT_EQ((uint64_t)10, em.getTime());

    logger.sched_recv(5, 99);
    em.run_to(100);

    ASSERT_EQ((size_t)1, logger.log.size());
    ASSERT_EQ((uint64_t)15, logger.log[0].first);
    ASSERT_EQ(99, logger.log[0].second);
}

TEST(events_after_run_to_horizon_do_not_fire) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();

    murm::TopLevelComponent root(&em);
    Logger logger(&root, "log");

    logger.sched_recv(50,  1);
    logger.sched_recv(150, 2);   // beyond the run_to horizon
    em.run_to(100);

    ASSERT_EQ((size_t)1, logger.log.size());
    ASSERT_EQ((uint64_t)50, logger.log[0].first);
    ASSERT_EQ(1, logger.log[0].second);
}


//// InitMethod fires during init() ///////////////////////////////////////////

TEST(init_method_fires_during_init_at_time_zero) {
    murm_test::reset_eventmanager();
    auto &em = murm::EventManager::getDefaultEventManager();

    murm::TopLevelComponent root(&em);
    Logger logger(&root, "log");

    logger.init_recv(7);
    logger.init_recv(8);

    // Before init(), nothing has fired.
    ASSERT_EQ((size_t)0, logger.log.size());

    em.init();

    ASSERT_EQ((size_t)2, logger.log.size());
    // FIFO order at time 0:
    ASSERT_EQ((uint64_t)0, logger.log[0].first);  ASSERT_EQ(7, logger.log[0].second);
    ASSERT_EQ((uint64_t)0, logger.log[1].first);  ASSERT_EQ(8, logger.log[1].second);
}


//// reset() between tests ////////////////////////////////////////////////////

TEST(reset_clears_pending_and_resets_time) {
    auto &em = murm::EventManager::getDefaultEventManager();
    em.reset();

    murm::TopLevelComponent root(&em);
    Logger logger(&root, "log");
    logger.sched_recv(10, 1);
    em.run_to(50);
    ASSERT_EQ((size_t)1, logger.log.size());

    em.reset();
    ASSERT_EQ((uint64_t)0, em.getTime());

    // After reset, no leftover events should fire.
    Logger logger2(&root, "log2");
    em.run_to(100);
    ASSERT_EQ((size_t)0, logger2.log.size());
}


RUN_ALL_TESTS()
