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
---

## Milestone 6 Update

### Compilation and Execution
* [cite_start]**Compilation:** `make milestone6` [cite: 12]
* [cite_start]**Execution:** `./sim input.txt` [cite: 4, 12]

### Implementation & Synchronization
* [cite_start]**Strict Node Mutual Exclusion:** Enforced a critical section constraint where a maximum of one passenger process can occupy a graph vertex at any given moment[cite: 18, 24].
* [cite_start]**Junction Wait Delay:** Implemented a strict 1-second holding time (`sleep(1)`) inside the locked node, causing trailing processes to safely wait outside the intersection[cite: 19, 20].
* [cite_start]**Visual Wait States:** Enhanced the Raylib graphical user interface to clearly display waiting processes with a distinct color and context labels to avoid intersection overlaps[cite: 24].
* [cite_start]**Deadlock Prevention:** Managed systematic semaphore release sequences (`sem_post`) to completely eliminate starvation and ensure final destination nodes are cleared upon arrival[cite: 26].

