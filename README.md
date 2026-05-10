# Directed Graph Traffic Simulation Project

## Compilation and Execution
To build and run the project for each milestone, use the following commands:

- **Milestone 1:** - Compile: `make milestone1`
   - Run: `./dijkstra`
- **Milestone 2:** - Compile: `make milestone2`
   - Run: `./sim`
- **Milestone 3:** - Compile: `make milestone3`
   - Run: `./sim`

## Implementation Overview

### Milestone 1: Graph & Dijkstra
- Implementation of a directed graph using adjacency lists.
- Calculation of the shortest path using **Dijkstra's Algorithm**.
- Input is read from `input.txt`, defining nodes, edges, and the target query.

### Milestone 2: GUI Visualization
- Integration with the **Raylib** library for graphical rendering.
- Cities (nodes) are positioned on a grid with randomized offsets.
- Directed edges are drawn with arrows and weight labels (km).

### Milestone 3: Animation & Simulation
- **Edge Traversal:** An edge with weight $W$ is divided into $W$ jumps.
- **Timing:** Each jump takes exactly **300ms**, as required.
- **Node Interaction:** The vehicle stops for **1 second** at each intermediate city.
- **Path Reconstruction:** The shortest path is highlighted in Yellow.
- **UI Control:** Added Play/Stop/Replay buttons and a simulation finished message.

## Cleanup
To remove all compiled binaries and object files, run:
`make clean`