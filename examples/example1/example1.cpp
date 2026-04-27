
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <ctime>
#include <chrono>
#include "../include/constants.hpp"
#include "../include/ThreadedEventMaster.hpp"
#include "../include/Param.hpp"
#include "../include/Stat.hpp"
#include "../include/SimOptionHandler.hpp"

#include "TestBed.hpp"

int main(const int argc, char* argv[])
{
   // Print the command line:
   for (uint i=0; i<argc; ++i) { std::cout << argv[i] << " "; }
   std::cout << "\n" << std::endl;

   // The rest of main() consists of the following steps:
   // 1. Pre-construction setup:
   //    a. Parse the command-line options.
   //    b. Create the master controller.
   //    c. Specify any simulation-wide parameters.
   //    d. If multithreaded, create the threads.
   // 2. Construct the simulation.
   // 3. Post-construction initialization:
   //    a. Do post-construction processing of command line arguments.
   //    b. Complete post-construction initialization.
   // 4. Run the simulation.
   // 5. Do any post-simulation actions (e.g., dump all stats, etc.)

   // Parse the command-line options:
   SimOptionHandler soh;
   soh.parseSimOptions(argc, argv);

   // Create the Event master (oversees the Event managers for all threads)
   murm::ThreadedEventMaster &master = murm::ThreadedEventMaster::getDefaultThreadedEventMaster();
   
   murm::TopLevelComponent *sim =
      dynamic_cast<murm::TopLevelComponent *>(murm::Component::getToplevelComponent());
   assert(sim);

   // Specify some top-level simulation-wide parameters
   murm::Param<int> &num_threads = sim->addParam<int>("num_threads", 1,
           "The number of threads used by the simulator (if zero, the maximum number of "
           "threads will be used and num_threads will be overwritten with this value).");
   // If the num_threads is zero, then set it to the maximum number of threads.
   if (num_threads == 0) {
      num_threads.set(master.getMaxNumThreads());
   }
   murm::Param<uint64_t> &sim_duration = sim->addParam<uint64_t>("sim_duration", 1000,
           "The time limit for the simulation (in ns).");

   // Create the threads
   master.createThreads(num_threads);
   

   //// Construct the simulation ////

   TestBed testbed(nullptr, "testbed");

   //// Done constructing ////
   

   // Post-construction processing of command line arguments (e.g., print parameter list,
   // dump parameters, print stat list, etc.)
   bool quit = soh.processCLA();
   if (quit) { return 0; }

   // Complete post-construction initialization
   master.init();

   // Run the simulation:
   master.run_to(sim_duration);

   // Walk the component tree and print any stats
   for (auto &comp : *sim) {
       for (auto &[name, stat] : comp.getStats()) {
           std::cout << stat->getFullName() << ": count=" << stat->getCount()
                     << ", sum=" << stat->getSum() << std::endl;
       }
   }

   return 0;
}
