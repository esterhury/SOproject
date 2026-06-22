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

// ============================================================================
// CORE GRAPH MANAGEMENT
// ============================================================================

// Allocates memory, initializes adjacency lists, positions, and semaphores for each vertex
Graph* createGraph(int vertices) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->numVertices = vertices;

    // Allocate memory for adjacency lists
    graph->adjLists = (Node**)malloc(vertices * sizeof(Node*));
    for (int i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
    }

    // Allocate memory for rendering coordinates
    graph->positions = (Vector2*)malloc(vertices * sizeof(Vector2));

    // Allocate and initialize semaphores for each vertex to manage junction access
    graph->semaphores = (sem_t*)malloc(vertices * sizeof(sem_t));
    for (int i = 0; i < vertices; i++) {
        sem_init(&(graph->semaphores[i]), 0, 1); // 1 = Node is initially available
    }

    return graph;
}

// Inserts a new directed edge into the adjacency list
void addEdge(Graph* graph, int src, int dest, int weight) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->dest = dest;
    newNode->weight = weight;
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;
}

// Safely deallocates all memory and destroys semaphores to prevent leaks
void freeGraph(Graph* graph) {
    if (!graph) return;

    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Node* toDelete = temp;
            temp = temp->next;
            free(toDelete);
        }
        sem_destroy(&(graph->semaphores[i])); // Destroy semaphore for each node
    }

    free(graph->adjLists);
    free(graph->positions);
    free(graph->semaphores); // Free semaphore array
    free(graph);
}

// ============================================================================
// ALGORITHMIC PATH-FINDING
// ============================================================================

// Standard Dijkstra's algorithm to compute shortest path distances
int dijkstra(Graph* graph, int src, int dst, int parent[]) {
    if (graph == NULL || src < 0 || src >= graph->numVertices || dst < 0 || dst >= graph->numVertices) return -1;

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
        if (u == -1 || distances[u] == INT_MAX || u == dst) break;
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

// Reconstructs the path from parent array after Dijkstra's execution
Path reconstructPath(int* parent, int src, int dst) {
    Path p;
    p.count = 0;
    p.active = false;
    if (dst == -1 || (parent[dst] == -1 && src != dst)) return p;

    int current = dst;
    int tempPath[100];
    int tempCount = 0;
    while (current != -1) {
        tempPath[tempCount++] = current;
        if (current == src) break;
        current = parent[current];
    }
    for (int i = 0; i < tempCount; i++) p.nodes[i] = tempPath[tempCount - 1 - i];
    p.count = tempCount;
    p.active = true;
    return p;
}

// ============================================================================
// VISUAL RENDERING
// ============================================================================

// Draws the visual representation of an entity (bus)
void drawEntity(Entity* entity) {
    if (!entity->isMoving && entity->currentPathIndex == 0) return;

    Color bodyColor = RED;
    Color lineColor = MAROON;
    const char* labelText = "BUS";
    Color textColor = DARKGRAY;

    if (entity->isWaiting) {
        bodyColor = ORANGE;
        lineColor = ORANGE;
        labelText = "Waiting...";
        textColor = RED;
    }
    DrawCircleV(entity->currentPos, 12, bodyColor);
    DrawCircleLinesV(entity->currentPos, 12, lineColor);
    DrawText(labelText, entity->currentPos.x - 15, entity->currentPos.y - 25, 12, textColor);
}

// ============================================================================
// ENTITY & PASSENGER MOVEMENT LOGIC
// ============================================================================

// Updates all active passengers positions and manages junction semaphores
void updateAllPassengers(Graph* graph, Passenger passengers[], int count, bool isRunning) {
    if (!isRunning || graph == NULL || passengers == NULL) return;

    for (int i = 0; i < count; i++) {
        Passenger* p = &passengers[i];
        if (p->simulationFinished || !p->shortestPath.active) continue;

        if (p->movingEntity.currentPathIndex < p->shortestPath.count - 1) {
            int currNode = p->shortestPath.nodes[p->movingEntity.currentPathIndex];
            int nextNode = p->shortestPath.nodes[p->movingEntity.currentPathIndex + 1];

            // Get weight for speed calculation
            float weight = 1.0f;
            Node* temp = graph->adjLists[currNode];
            while (temp) {
                if (temp->dest == nextNode) {
                    weight = (float)temp->weight;
                    break;
                }
                temp = temp->next;
            }

            float dynamicSpeed = 5.0f / (weight > 0 ? weight : 1.0f);
            Vector2 targetPos = graph->positions[nextNode];
            float dx = targetPos.x - p->movingEntity.currentPos.x;
            float dy = targetPos.y - p->movingEntity.currentPos.y;
            float distance = sqrtf(dx * dx + dy * dy);

            // Move vehicle towards destination
            if (distance > dynamicSpeed) {
                p->movingEntity.isWaiting = false;
                p->movingEntity.currentPos.x += (dx / distance) * dynamicSpeed;
                p->movingEntity.currentPos.y += (dy / distance) * dynamicSpeed;
            } else {
                // Reached next node - handle junction release
                p->movingEntity.currentPos = targetPos;
                p->movingEntity.isWaiting = true;
            }
        } else {
            // Final destination cleanup: release the semaphore
            int finalNode = p->shortestPath.nodes[p->movingEntity.currentPathIndex];
            sem_post(&(graph->semaphores[finalNode]));
            p->simulationFinished = true;
            p->movingEntity.isMoving = false;
            p->movingEntity.isWaiting = false;
        }
    }
}


// Load graph structure and dynamic travelers arrays from the input file
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int** prioritiesArray, int* numTravelers) {

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
        if (strstr(line, "travelers") != NULL) {
            foundTravelers = true;
            break;
        }
    }

    if (!foundTravelers) {
        printf("Error: No Travelers found.\n");
        *numTravelers = 0;
        *sourcesArray = NULL;
        *destsArray = NULL;
        *prioritiesArray = NULL;
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

    // Allocate dynamic memory on the Heap for source, destination, and priorities arrays
    *sourcesArray = (int*)malloc((*numTravelers) * sizeof(int));
    *destsArray = (int*)malloc((*numTravelers) * sizeof(int));
    *prioritiesArray = (int*)malloc((*numTravelers) * sizeof(int)); // <-- הקצאה עבור אבן דרך 7!

    if (*sourcesArray == NULL || *destsArray == NULL || *prioritiesArray == NULL) {
        printf("Error: Memory allocation failed for travelers arrays.\n");
        *numTravelers = 0;
        fclose(file);
        return graph;
    }

    // Read all individual source, destination, and priority triplets
    for (int i = 0; i < *numTravelers; i++) {
        // קורא 3 מספרים מהקובץ: מקור, יעד ועדיפות/זמן
        if (fscanf(file, "%d %d %d", &((*sourcesArray)[i]), &((*destsArray)[i]), &((*prioritiesArray)[i])) != 3) {
            printf("Warning: Error reading data for traveler %d. Setting default priority to 1.\n", i);
            (*sourcesArray)[i] = -1;
            (*destsArray)[i] = -1;
            (*prioritiesArray)[i] = 1; // עדיפות ברירת מחדל אם אין נתון
        }
    }

    fclose(file);
    return graph;
}