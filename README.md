# Directed Graph Traffic Simulation Project

## Compilation and Execution Rules
To compile and run the simulation targets dynamically, follow the official target system blueprints below:

### Milestone 4
- **Compilation:** make milestone4
- **Execution:** ./sim

### Milestone 5
- **Compilation:** make milestone5
- **Execution:** ./sim

### Workspace Cleanup
To flush out dynamic binary artifacts and object loops, execute:
make clean

---

## Implementation Description 

### Milestone 4

- Orchestrated concurrent vehicle lifecycles utilizing independent OS child processes via fork(). The parent process drives the graphical layout and prevents system zombie leaks by proactively reaping resources using kill() and waitpid().

### Milestone 5

- Shifted routing calculations (Dijkstra) autonomously inside child processes. Implemented Inter-Process Communication (IPC) using non-blocking UNIX pipes to stream positions to the parent, and enforced intersection mutual exclusion using POSIX semaphores in shared memory blocks to prevent data races and deadlocks.

---

## DevOps & Technical Infrastructure Details

### Milestone 4 Infrastructure
* **Operating System Multi-Processing:** The parent process parses configuration layouts dynamically and orchestrates concurrent traveler lifecycles by deploying an execution loop utilizing fork().
* **Proactive Zombie Reaping Cleanup:** Active OS process indicators are stored systematically inside the global_pids array map to eliminate zombie process state leaks from the system.

### Milestone 5 Infrastructure
* **Asynchronous Message Passing Channels:** Established true UNIX pipe() communication networks utilizing non-blocking operations (O_NONBLOCK) where child processes dynamically broadcast node transition messages back to the father's polling matrix.
* **Cross-Process Resource Locking:** Deployed Shared Memory mapping (MAP_SHARED | MAP_ANONYMOUS) to initialize a contiguous array of POSIX sem_t semaphores. Mutual exclusion locks are systematically handled via sem_wait() and sem_post() sequences across vertices to eliminate intersection collisions.