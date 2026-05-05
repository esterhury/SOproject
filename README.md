# Directed Graph Traffic Simulation

## Group Members & Responsibilities
*   **Ester Hury**: Sub-task 1 - Building the Graph data structures and file parsing logic.
*   **Shira Cadury**: Sub-task 2 - Implementing the Dijkstra algorithm for shortest path calculation.
*   **Michal Levin**: Sub-task 3 - Error handling, terminal output formatting, and specific cases (e.g., "No path found").
*   **Elia Levi**: Sub-task 4 - Creating the Makefile, writing the README documentation, and final code testing.

## Project Description
This project is a semester-long simulation of a directed graph traffic system.
Multiple "passengers" (processes) move simultaneously across a graph using 
operating system mechanisms such as processes, IPC, synchronization, and scheduling.

## Background Story: City Transit Network
Our graph represents a simplified map of a city's public transport system.
Nodes represent major stations (bus/train), and directed edges represent 
one-way routes between them, with weights indicating the travel time or distance.

## How to Run (Milestone 1)
1. Ensure you have the source files: `main.c`, `graph.c`, `graph.h`, and `Makefile`.[cite: 1]
2. Ensure `input.txt` is in the same directory.[cite: 1]
3. Open terminal and run:
   ```bash
   make milestone1
   ./graph_app
