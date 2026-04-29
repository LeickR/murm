
#ifndef __TESTBED_HPP__
#define __TESTBED_HPP__

#include <vector>
#include <string>
#include <cstdint>
#include "../include/Component.hpp"
#include "../include/OutPort.hpp"
#include "../include/InPort.hpp"
#include "Node.hpp"

// The TestBed contains some number of "nodes" (default of 4).
//
// The nodes are arranged in a ring, with each node's "ping_out" port connected
// to the next node's "ping_in" port.
//
class TestBed : public murm::Component {
private:
  // Parameters
  murm::Param<uint> num_nodes { this, "num_nodes", 4,
      "The number of nodes in the TestBed." };
  murm::Param<uint> n2n_delay { this, "n2n_delay", 10,
      "The node-to-node delay (in nsec)." };

  std::vector<Node> nodes;
public:
  // Constructor
  TestBed(murm::Component *parent, const std::string &name) :
    Component(parent, "TestBed", name)
  {
    // Create the Nodes
    nodes.reserve(num_nodes); // (so we're not moving nodes after construction)
    for (uint i = 0; i < num_nodes; ++i) {
      nodes.emplace_back(this, i);
    }

    // And connect them up
    // (each node's ping_out OutPort connects to the next node's ping_in InPort, in a ring)
    for (uint i = 0; i < num_nodes; ++i) {
        uint prev_i = (i == 0) ? (num_nodes - 1) : (i - 1);
        murm::connect(nodes[prev_i].ping_out, nodes[i].ping_in, n2n_delay);
    }

    nodes[0].startPingPong();
  }
};

#endif // __TESTBED_HPP__
