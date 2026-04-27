
#ifndef __GIZMO_HPP__
#define __GIZMO_HPP__

#include <vector>
#include <string>
#include <cstdint>
#include "../include/Component.hpp"
#include "../include/OutPort.hpp"
#include "../include/InPort.hpp"
#include "../include/ScheduledMethod.hpp"

//
// Each gizmo has one InPort called "ping_in" and one OutPort called "ping_out".
//
// When a ping arrives at the gizmo's "ping_in" port, it "processes" the ping for a
// specified amount of time (default of 1 nsec) and then forwards the ping
// through it's "ping_out" port.
//
class Gizmo : public murm::Component {
private:
  // Parameters
  murm::Param<uint> proc_time { this, "proc_time", 1,
      "How long it takes a gizmo to 'process' a ping." };

  murm::ScheduledMethod<int> sch_send_ping { this };

public:
  // The in and out ports:
  murm::InPort<int> ping_in { this };
  murm::OutPort<int> ping_out { this };
  
  // Constructor
  Gizmo(murm::Component *parent, const std::string &name) :
    Component(parent, "Gizmo", name)
  {
    // Connect up the ping_in InPort
    ping_in.bindToMethod(this, &Gizmo::handlePing);

    // and the send_ping scheduler
    sch_send_ping.bindToMethod(this, &Gizmo::sendPing);
  }

  // Handle an incoming ping
  void handlePing(int ping) {
    // After the "processing" time has elapsed, send the ping on its way...
    sch_send_ping(proc_time, ping);
  }
  
  // Send a ping
  void sendPing(int ping) {
    ping_out(ping);
  }

  
  // Called at the beginning to get things started.
  // Send a ping at time = 0;
  void startPingPong() {
    sch_send_ping(0, 100);
  }
};

#endif // __GIZMO_HPP__
