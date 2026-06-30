#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "raylib.h"
#include "graph.h"
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>

// --- NEW: Static variables for centralized queue management (Bat 1) ---
static NodeQueue nodeQueues[1000];
static int globalArrivalCounter = 0;
static int nodeOwner[1000];
// ---------------------------------------------------------------------

// Create a new graph with a given number of vertices
Graph* createGraph(int vertices){
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->numVertices = vertices;
    graph->adjLists = (Node**)malloc(vertices * sizeof(Node*));

    for (int i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
    }
    graph->positions = (Vector2*)malloc(vertices * sizeof(Vector2));

    // Allocate a contiguous array of semaphores within a shared memory block for cross-process synchronization
    graph->semaphores = (sem_t*)mmap(NULL, vertices * sizeof(sem_t),
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (graph->semaphores == MAP_FAILED) {// Validate that the shared memory allocation for the semaphore array succeeded completely
        perror("Error: mmap failed for semaphores");
        exit(1);
    }

    // Initialize each vertex semaphore to be shared among processes and set its initial capacity to 1
    for (int i = 0; i < vertices; i++) {
        if (sem_init(&(graph->semaphores[i]), 1, 1) != 0) {
            perror("Error: sem_init failed");
            exit(1);
        }
    }

    // --- NEW: Allocate private agent semaphores for parent-driven centralized control ---
    graph->agent_semaphores = (sem_t*)mmap(NULL, MAX_PASSENGERS * sizeof(sem_t),
                                           PROT_READ | PROT_WRITE,
                                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (graph->agent_semaphores == MAP_FAILED) {
        perror("Error: mmap failed for agent semaphores");
        exit(1);
    }
    for (int i = 0; i < MAX_PASSENGERS; i++) {
        sem_init(&(graph->agent_semaphores[i]), 1, 0); // Start blocked (0)
    }
    // --------------------------------------------------------------------------------

    return graph;
}

// Add an edge to the graph
void addEdge(Graph* graph, int src, int dest, int weight) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->dest = dest;
    newNode->weight = weight;
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;
}

// Load graph structure and dynamic travelers arrays from the input file
// --- NEW: Updated signature and logic to parse burst times for SJF ---
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int** burstTimesArray, int* numTravelers) {

    // Open the input file for reading operations only
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file.\n");
        return NULL;
    }

    // Read the total number of vertices (n) and edges (m) from the first line
    int n, m;
    if (fscanf(file, "%d %d", &n, &m) != 2) {
        fclose(file);
        return NULL;
    }

    // Allocate memory and initialize the graph structure
    Graph* graph = createGraph(n);

    // Loop through all edges and add them to the adjacency list
    for (int i = 0; i < m; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) == 3) {
            // Validate that edge weights are non-negative to ensure Dijkstra correctness
            if (w < 0) {
                printf("Error: Invalid input. Weights cannot be negative.\n");
                fclose(file);
                freeGraph(graph);
                return NULL;
            }
            addEdge(graph, u, v, w);
        }
    }

    char line[256];
    bool foundTravelers = false;

    // Scan the file line by line to locate the travelers header
    while (fgets(line, sizeof(line), file)) {
        // Look for the keyword travelers dynamically to ensure reliable parsing
        if (strstr(line, "travelers") != NULL) {
            foundTravelers = true;
            break;
        }
    }

    // Handle the error case where the travelers section is missing
    if (!foundTravelers) {
        printf("Error: No Travelers found.\n");
        *numTravelers = 0;
        *sourcesArray = NULL;
        *destsArray = NULL;
        *burstTimesArray = NULL; // NEW
        fclose(file);
        return graph;
    }

    // Read the total number of travelers located right under the header
    if (fscanf(file, "%d", numTravelers) != 1) {
        printf("Error: could not read number of travelers.\n");
        *numTravelers = 0;
        fclose(file);
        return graph;
    }

    // Allocate dynamic memory on the Heap for source, destination, and burst time arrays
    *sourcesArray = (int*)malloc((*numTravelers) * sizeof(int));
    *destsArray = (int*)malloc((*numTravelers) * sizeof(int));
    *burstTimesArray = (int*)malloc((*numTravelers) * sizeof(int)); // NEW

    // Verify that memory allocation for the dynamic arrays succeeded completely
    if (*sourcesArray == NULL || *destsArray == NULL || *burstTimesArray == NULL) {
        printf("Error: Memory allocation failed for travelers arrays.\n");
        *numTravelers = 0;
        fclose(file);
        return graph;
    }

    // Read all individual source, destination, and (optional) burst time parameters
    for (int i = 0; i < *numTravelers; i++) {
        int src, dst, burst = 10;
        // Attempt to read 3 values: Source, Destination, and Burst Time
        int readCount = fscanf(file, "%d %d %d", &src, &dst, &burst);

        if (readCount >= 2) {
            (*sourcesArray)[i] = src;
            (*destsArray)[i] = dst;
            (*burstTimesArray)[i] = (readCount == 3) ? burst : 10; // Default to 10 if burst time is missing
        } else {
            printf("Warning: Error reading data for traveler %d. Setting to -1.\n", i);
            (*sourcesArray)[i] = -1;
            (*destsArray)[i] = -1;
            (*burstTimesArray)[i] = 10;
        }
    }

    fclose(file);
    return graph;
}

//Clean up memory to prevent leaks
void freeGraph(Graph* graph) {
    if (!graph) return;

    if (graph->semaphores != NULL) {
        for (int i = 0; i < graph->numVertices; i++) {
            sem_destroy(&(graph->semaphores[i]));
        }
        munmap(graph->semaphores, graph->numVertices * sizeof(sem_t));
    }

    // --- NEW: Clean up the new agent semaphores ---
    if (graph->agent_semaphores != NULL) {
        for (int i = 0; i < MAX_PASSENGERS; i++) {
            sem_destroy(&(graph->agent_semaphores[i]));
        }
        munmap(graph->agent_semaphores, MAX_PASSENGERS * sizeof(sem_t));
    }
    // ----------------------------------------------

    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Node* toDelete = temp;
            temp = temp->next;
            free(toDelete);
        }
    }
    free(graph->adjLists);
    free(graph->positions);
    free(graph);
}

// Finds the shortest path cost from source to destination.
int dijkstra(Graph* graph, int src, int dst, int parent[]) {
    if (graph == NULL || src < 0 || src >= graph->numVertices || dst < 0 || dst >= graph->numVertices) {
        return -1;
    }

    int numVertices = graph->numVertices;
    int* distances = (int*)malloc(numVertices * sizeof(int));
    int* visited = (int*)malloc(numVertices * sizeof(int));

    for (int i = 0; i < numVertices; i++) {
        distances[i] = INT_MAX;
        visited[i] = 0;
        parent[i] = -1;
    }
    distances[src] = 0;
    for (int i = 0; i < numVertices - 1; i++) {
        int minDistance = INT_MAX;
        int u = -1;
        for (int v = 0; v < numVertices; v++) {
            if (visited[v] == 0 && distances[v] <= minDistance) {
                minDistance = distances[v];
                u = v;
            }
        }
        if (u == -1 || distances[u] == INT_MAX || u == dst) {
            break;
        }
        visited[u] = 1;
        Node* current = graph->adjLists[u];
        while (current) {
            int v = current->dest;
            int weight = current->weight;

            if (!visited[v] && distances[u] != INT_MAX && (distances[u] + weight < distances[v])) {
                distances[v] = distances[u] + weight;
                parent[v] = u;
            }
            current = current->next;
        }
    }
    int result = distances[dst];
    free(distances);
    free(visited);

    return result;
}

// printing path
void printPath(int* parent, int src, int dst) {
    if (dst == src) {
        printf("%d", src);
        return;
    }
    printPath(parent, src, parent[dst]);
    printf(" -> %d", dst);
}

void computePosition(Graph* graph) {
    if (!graph || graph->numVertices <= 0) {
        return;
    }
    if (graph->numVertices == 5) {
        graph->positions[0] = (Vector2){ 150.0f, 150.0f }; // Jerusalem (Top Left)
        graph->positions[1] = (Vector2){ 350.0f, 250.0f }; // Tel Aviv (Middle - Lowered to prevent overlap!)
        graph->positions[2] = (Vector2){ 550.0f, 150.0f }; // Haifa (Top Right)
        graph->positions[3] = (Vector2){ 700.0f, 320.0f }; // Node 3 (Far Right)
        graph->positions[4] = (Vector2){ 250.0f, 480.0f }; // Beersheba (Bottom Left)
        return;
    }

    Vector2 center = { 400.0f, 300.0f };
    float radius = 180.0f;
    for (int i = 0; i < graph->numVertices; i++) {
        float angle = (float)i * (2.0f * PI / graph->numVertices);
        graph->positions[i].x = center.x + cosf(angle) * radius;
        graph->positions[i].y = center.y + sinf(angle) * radius;
    }
}

void drawGraph(Graph* graph, Path path) {
    if (!graph) return;

    const char* cityNames[] = {"Jerusalem", "Tel Aviv", "Haifa", "Eilat", "Beersheba", "Ashdod", "Tiberias"};
    ClearBackground((Color){ 240, 242, 245, 255 });

    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Vector2 startPos = graph->positions[i];
            Vector2 endPos = graph->positions[temp->dest];

            Color edgeColor = (Color){ 160, 160, 160, 255 };
            float thickness = 2.0f;

            if (path.active) {
                for (int k = 0; k < path.count - 1; k++) {
                    if (path.nodes[k] == i && path.nodes[k+1] == temp->dest) {
                        edgeColor = YELLOW;
                        thickness = 4.5f;
                        break;
                    }
                }
            }
            DrawLineEx(startPos, endPos, thickness, edgeColor);

            float dx = endPos.x - startPos.x;
            float dy = endPos.y - startPos.y;
            float length = sqrtf(dx * dx + dy * dy);

            if (length > 0) {
                Vector2 unitDir = { dx / length, dy / length };

                Vector2 arrowPos = { endPos.x - unitDir.x * 28, endPos.y - unitDir.y * 28 };
                DrawPoly(arrowPos, 3, 7, atan2f(dy, dx) * RAD2DEG, edgeColor);
                Vector2 weightPos = {
                    startPos.x + unitDir.x * (length * 0.33f),
                    startPos.y + unitDir.y * (length * 0.33f)
                };

                char weightStr[12];
                sprintf(weightStr, "%d km", temp->weight);
                int fontSize = 14;
                int textWidth = MeasureText(weightStr, fontSize);

                DrawRectangle(weightPos.x - (textWidth/2 + 3), weightPos.y - 9, textWidth + 6, 18, (Color){ 231, 76, 60, 255 });
                DrawText(weightStr, weightPos.x - textWidth/2, weightPos.y - 7, fontSize, WHITE);
            }
            temp = temp->next;
        }
    }

    for (int i = 0; i < graph->numVertices; i++) {
        Vector2 pos = graph->positions[i];
        float radius = 22.0f;

        DrawCircleV(pos, radius, (Color){ 44, 62, 80, 255 });
        DrawCircleLinesV(pos, radius, (Color){ 52, 152, 219, 255 });

        char idStr[16];
        sprintf(idStr, "%d", i);
        int idWidth = MeasureText(idStr, 18);
        DrawText(idStr, pos.x - idWidth / 2, pos.y - 9, 18, WHITE);

        if (i < 7) {
            int cityFontSize = 16;
            int cityNameWidth = MeasureText(cityNames[i], cityFontSize);
            float yOffset = (pos.y < 300) ? -45 : 30;

            DrawText(cityNames[i], pos.x - (cityNameWidth / 2), pos.y + yOffset, cityFontSize, (Color){ 44, 62, 80, 255 });
        }
    }
}

Path reconstructPath(int* parent, int src, int dst) {
    Path p;
    p.count = 0;
    p.active = false;

    if (dst == -1 || (parent[dst] == -1 && src != dst)) {
        return p;
    }

    int current = dst;
    int tempPath[100];
    int tempCount = 0;

    while (current != -1) {
        tempPath[tempCount++] = current;
        if (current == src) break;
        current = parent[current];
    }

    for (int i = 0; i < tempCount; i++) {
        p.nodes[i] = tempPath[tempCount - 1 - i];
    }

    p.count = tempCount;
    p.active = true;

    return p;
}


// =================================================================
// NEW: Bat 1 Centralized API Implementation (Queue Management & Scheduling)
// =================================================================

// Initializes the tracking arrays for all node queues
void initNodeQueues(int numVertices) {
    globalArrivalCounter = 0;
    for (int i = 0; i < numVertices; i++) {
        nodeQueues[i].count = 0;
        nodeOwner[i] = -1; // -1 means the node is currently free
    }
}

// Adds a vehicle to the waiting queue of a specific node
// Adds a vehicle to the waiting queue of a specific node
void addToNodeQueue(int node, int agentIndex, int priority) {
    if (node < 0 || node >= 1000) return;

    NodeQueue* nq = &nodeQueues[node];

    for (int i = 0; i < nq->count; i++) {
        if (nq->queue[i].agentIndex == agentIndex) {
            return;
        }
    }

    int c = nq->count;
    if (c >= MAX_PASSENGERS) return; // Prevent overflow

    nq->queue[c].agentIndex = agentIndex;
    nq->queue[c].priority = priority;
    nq->queue[c].arrivalOrder = globalArrivalCounter++; // Important for FCFS tie-breaking
    nq->count++;
}

// The core logic function: decides who enters the intersection next
int scheduleNextAgent(int node, int algorithmType) {
    if (node < 0 || node >= 1000 || nodeQueues[node].count == 0) return -1;

    int bestIndex = 0;
    NodeQueue* nq = &nodeQueues[node];

    // --- הדפסת רשימת הממתינים בצומת ---
    printf("\n[Scheduler Decision] At Node %d, current queue: ", node);
    for (int k = 0; k < nq->count; k++) {
        printf("Agent_%d (Priority: %d, Order: %d) ",
               nq->queue[k].agentIndex, nq->queue[k].priority, nq->queue[k].arrivalOrder);
    }
    printf("\n");

    for (int i = 1; i < nq->count; i++) {
        if (algorithmType == SCHEDULING_FCFS) {
            // First-Come, First-Served Logic
            if (nq->queue[i].arrivalOrder < nq->queue[bestIndex].arrivalOrder) {
                bestIndex = i;
            }
        } else if (algorithmType == SCHEDULING_PRIORITY) { // שונה ל-PRIORITY
            // Priority Scheduling Logic (Smaller numbers = higher priority)
            if (nq->queue[i].priority < nq->queue[bestIndex].priority) {
                bestIndex = i;
            } else if (nq->queue[i].priority == nq->queue[bestIndex].priority) {
                // Break ties using FCFS
                if (nq->queue[i].arrivalOrder < nq->queue[bestIndex].arrivalOrder) {
                    bestIndex = i;
                }
            }
        }
    }

    int selectedAgent = nq->queue[bestIndex].agentIndex;

    // --- הדפסת מי נבחר ולמה  ---
    printf(">> Chosen Agent: Agent_%d\n", selectedAgent);
    if (algorithmType == SCHEDULING_FCFS) {
        printf(">> Reason: Selected based on FCFS (arrived first with arrival order %d).\n",
               nq->queue[bestIndex].arrivalOrder);
    } else if (algorithmType == SCHEDULING_PRIORITY) {
        printf(">> Reason: Selected based on SJF/Priority (had the smallest burst time/priority of %d).\n",
               nq->queue[bestIndex].priority);
    }
    printf("==========================================\n");

    // Remove the selected vehicle from the queue and shift others down
    for (int i = bestIndex; i < nq->count - 1; i++) {
        nq->queue[i] = nq->queue[i + 1];
    }
    nq->count--;

    nodeOwner[node] = selectedAgent; // Lock the node for the selected vehicle
    return selectedAgent;
}
// Marks a node as free when a vehicle leaves
void releaseNode(int node) {
    if (node >= 0 && node < 1000) nodeOwner[node] = -1;
}

// Checks who currently occupies a node
int getNodeOwner(int node) {
    if (node >= 0 && node < 1000) return nodeOwner[node];
    return -1;
}

// --- NEW: Dummy implementations for visual updates (Now handled centrally in main.c) ---
void updateEntity(Entity* entity, Graph* graph, Path* path) {
    (void)entity; (void)graph; (void)path;
}
void drawEntity(Entity* entity) {
    (void)entity;
}
void updateAllPassengers(Graph* graph, Passenger passengers[], int count, bool isRunning) {
    // This is intentionally left empty. Bat 1 milestone requires the parent process (main.c)
    // to handle all updates centrally, replacing this distributed approach.
    (void)graph; (void)passengers; (void)count; (void)isRunning;
}
void calculatePassengerRoute(Graph* graph, Passenger* passenger, int src, int dst) {
    // Intentionally empty. Handled locally inside the child process now.
    (void)graph; (void)passenger; (void)src; (void)dst;
}
