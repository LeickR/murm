
#ifndef __NODE_HPP__
#define __NODE_HPP__

#include <vector>
#include <string>
#include <cstdint>
#include "../include/Component.hpp"
#include "../include/OutPort.hpp"
#include "../include/InPort.hpp"
#include <string>
#include "Gizmo.hpp"

// Each Node consists of some number of "gizmos" (default of 3).
// Each Node has one InPort called "ping_in" and one OutPort called "ping_out".
//
// The Node's "ping_in" port connects to the "ping_in" of the first gizmo.
// Each gizmo is daisy-chained to the next, and the last gizmo's "ping_out" port
// is connected to the Node's "ping_out" port.
//
class Node : public murm::Component {
private:
  // Parameters
  murm::Param<uint> num_gizmos { this, "num_gizmos", 3,
      "The number of gizmos in the Node." };
  murm::Param<uint> g2g_delay { this, "g2g_delay", 1,
      "The gizmo-to-gizmo latency (in nsec)." };

  std::vector<Gizmo> gizmos;
public:
  // The in and out ports:
  murm::InPort<int> ping_in { this };
  murm::OutPort<int> ping_out { this };
  
  // Constructor
  Node(murm::Component *parent, const std::string &name) :
    Component(parent, "Node", name)
  {
    // Create the Gizmos
    gizmos.reserve(num_gizmos); // (so we're not moving gizmos after construction)
    for (uint i = 0; i < num_gizmos; ++i) {
      gizmos.emplace_back(this, std::string("gizmo") + std::to_string(i));
    }

    // And connect them up
    // (the Node's ping_in is attached to the first gizmo's ping_in,
    //  the last gizmo's ping_out is attached to the Node's ping_out,
    //  and all of the gizmos are daisy-chained in between)
    murm::connect(ping_in, gizmos.front().ping_in, 0);
    for (uint i = 1; i < num_gizmos; ++i) {
        murm::connect(gizmos[i-1].ping_out, gizmos[i].ping_in, g2g_delay);
    }
    murm::connect(gizmos.back().ping_out, ping_out, 0);
  }

  // Called at the beginning to get things started
  void startPingPong() {
    gizmos[0].startPingPong();
  }
};

#endif // __NODE_HPP__
