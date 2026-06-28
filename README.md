# Operating Systems Project – Traffic Simulation on a Graph

This project implements a distributed traffic simulation of passengers (child processes) moving across a network of intersections (graph nodes). The system is centrally managed and synchronized by a parent process (the scheduler) using IPC mechanisms such as non-blocking pipes and semaphores, with a real-time graphical interface powered by Raylib.

---

## Compilation and Execution Commands (Makefile Targets)

You can build each specific milestone or the final submission using the following Makefile targets.

* **Milestone 1:** make milestone1
* **Milestone 2:** make milestone2
* **Milestone 3:** make milestone3
* **Milestone 4:** make milestone4
* **Milestone 5:** make milestone5
* **Milestone 6:** make milestone6
* **Milestone 7 (Final):** make milestone7

To clean all object files and compiled binaries from the repository:

make clean

---

### Execution Usage for Testing (Milestone 7)

The application accepts the scheduling algorithm flag and parameter directly via the command line:

1. Running with First-Come, First-Served (FCFS) Scheduler:  
      **./sim -schd fcfs**

2. Running with Shortest Job First (SJF) Scheduler:  
  **./sim -schd sjf**

*Note: You can also dynamically toggle between algorithms during runtime by pressing the S key on your keyboard.*

---

## Milestone Implementation Overview

* **Milestone 1:** Established the core graph data structures, node definitions, and implemented the file parsing subsystem to safely load network structures from configuration files.
* **Milestone 2:** Extended the graph infrastructure to support dynamic edge weighting, directional vertex navigation, and structured internal node tracking.
* **Milestone 3:** Integrated the Raylib graphical rendering engine, creating visual layers to map out nodes, connections, and interactive simulation layouts.
* **Milestone 4:** Developed and integrated Dijkstra's shortest-path algorithm, enabling individual passenger entities to compute optimal routes from their source to destination.
* **Milestone 5:** Transformed the application into a distributed multi-process architecture where each passenger executes as an independent child process via fork().
* **Milestone 6:** Designed a robust Inter-Process Communication (IPC) architecture utilizing non-blocking pipes (O_NONBLOCK) and dedicated semaphores to regulate junction access and prevent intersection collisions.
* **Milestone 7 (Final Submission):** Upgraded the junction access control from a random selection loop to a centralized scheduling matrix supporting FCFS and SJF. The parent process manages dedicated wait-queues for each node, evaluating process priorities (Burst Times parsed from the input file) to grant intersection clearance.

---

## Scheduling Algorithm Analysis and Comparison

A comparative evaluation of both scheduling strategies was conducted using identical input configurations to monitor their impact on waiting times:

1. FCFS (First-Come, First-Served):
    * Behavior: Processes requesting entry to a specific intersection node are queued chronologically. The first process to register its arrival message via the pipe receives clearance first.
    * Performance Impact: Provides strict operational fairness but suffers heavily from the Convoy Effect. If a slow passenger or one with an extensive path (high burst time) gains node authorization first, it artificially inflates the waiting time for shorter, faster background processes queued behind it.

2. SJF (Shortest Job First):
    * Behavior: The central scheduler maintains a priority queue for each junction sorted by the remaining execution metric (Burst Time). When an intersection clears, the waiting process with the lowest burst time is woken up next.
    * Performance Impact: Switching to SJF significantly minimized the average turnaround and waiting times across the simulation. Shorter tasks dynamically bypass longer ones at congested junctions, clearing the graph rapidly and optimizing overall traffic throughput (though theoretically introducing a risk of process starvation under constant high-load short-process flows).