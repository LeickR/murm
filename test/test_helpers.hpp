// Helpers shared across murm test binaries.
//
// Each test binary uses the same singleton EventManager (returned by
// EventManager::getDefaultEventManager()).  Between tests we call
// reset_eventmanager() so simulation time goes back to 0 and any leftover
// queued events are cleared.

#ifndef MURM_TEST_HELPERS_HPP
#define MURM_TEST_HELPERS_HPP

#include "EventManager.hpp"
#include "Component.hpp"

namespace murm_test {

// Reset the global EventManager between tests so the next test starts at t=0
// with empty event queues.
inline void reset_eventmanager() {
    murm::EventManager::getDefaultEventManager().reset();
}

} // namespace murm_test

#endif // MURM_TEST_HELPERS_HPP
