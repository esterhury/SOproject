# Directed Graph Traffic Simulation Project

## Compilation and Execution
To build and execute the integrated multi-process layout, use the following commands:
- **Milestone 4 Compilation Target:** `make milestone4`
- **Milestone 4 Parallel Run Execution:** `./sim input.txt`

## DevOps & Multi-Agent Infrastructure Implementation
* **Operating System Multi-Processing:** The parent process parses configuration layouts dynamically and orchestrates concurrent traveler lifecycles by deploying an execution loop utilizing `fork()`.
* **Proactive Zombie Reaping Cleanup:** Active OS process indicators are stored systematically inside the `global_pids` array map. Upon safe node arrival or window closure signals, resources are forced cleanly via `kill(pid, SIGKILL)` followed by explicit sequential `waitpid()` harvesting iterations to eliminate zombie process state leaks from the system.

## Workspace Cleanup
To flush out binary artifacts and clean the compilation environment, execute:
`make clean`
