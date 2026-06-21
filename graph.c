#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "raylib.h"
#include "graph.h"
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include  <sys/mman.h>

// ============================================================================
// CORE GRAPH MANAGEMENT
// ============================================================================

// Allocates memory and initializes a new Graph structure with the specified number of vertices
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

    return graph;
}

// Inserts a new directed edge into the graph's adjacency list
void addEdge(Graph* graph, int src, int dest, int weight) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->dest = dest;
    newNode->weight = weight;

    // Insert at the beginning of the list (Head insertion)
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;
}

// Parses a formatted text file to instantiate the graph and extract traveler routes
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int* numTravelers) {
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
            // Validate that edge weights are non-negative to ensure Dijkstra's correctness
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

    // Scan the file line by line to locate the 'travelers' header dynamically
    while (fgets(line, sizeof(line), file)) {
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

    // Allocate dynamic memory on the Heap for source and destination arrays
    *sourcesArray = (int*)malloc((*numTravelers) * sizeof(int));
    *destsArray = (int*)malloc((*numTravelers) * sizeof(int));

    // Verify that memory allocation for the dynamic arrays succeeded
    if (*sourcesArray == NULL || *destsArray == NULL) {
        printf("Error: Memory allocation failed for travelers arrays.\n");
        *numTravelers = 0;
        fclose(file);
        return graph;
    }

    // Read all individual source and destination pairs into the allocated arrays
    for (int i = 0; i < *numTravelers; i++) {
        if (fscanf(file, "%d %d", &((*sourcesArray)[i]), &((*destsArray)[i])) != 2) {
            printf("Warning: Error reading data for traveler %d. Setting to -1.\n", i);
            (*sourcesArray)[i] = -1;
            (*destsArray)[i] = -1;
        }
    }

    fclose(file);
    return graph;
}

// Safely deallocates all dynamically allocated memory tied to the graph to prevent memory leaks
void freeGraph(Graph* graph) {
    if (!graph) return;

    // Free adjacency lists
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

// ============================================================================
// ALGORITHMIC PATH-FINDING
// ============================================================================

// Executes Dijkstra's algorithm to find the shortest path cost from source to destination
int dijkstra(Graph* graph, int src, int dst, int parent[]) {
    // Validate boundaries and pointer integrity
    if (graph == NULL || src < 0 || src >= graph->numVertices || dst < 0 || dst >= graph->numVertices) {
        return -1;
    }

    int numVertices = graph->numVertices;
    int* distances = (int*)malloc(numVertices * sizeof(int));
    int* visited = (int*)malloc(numVertices * sizeof(int));

    // Initialization phase
    for (int i = 0; i < numVertices; i++) {
        distances[i] = INT_MAX;
        visited[i] = 0;
        parent[i] = -1;
    }
    distances[src] = 0;

    // Main Dijkstra loop
    for (int i = 0; i < numVertices - 1; i++) {
        int minDistance = INT_MAX;
        int u = -1;

        // Find the unvisited vertex with the smallest known distance
        for (int v = 0; v < numVertices; v++) {
            if (visited[v] == 0 && distances[v] <= minDistance) {
                minDistance = distances[v];
                u = v;
            }
        }

        // Break early if the remaining vertices are unreachable or the destination is found
        if (u == -1 || distances[u] == INT_MAX || u == dst) {
            break;
        }

        visited[u] = 1;
        Node* current = graph->adjLists[u];

        // Relax neighboring edges
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

// Recursively prints the computed shortest path to standard output
void printPath(int* parent, int src, int dst) {
    if (dst == src) {
        printf("%d", src);
        return;
    }
    printPath(parent, src, parent[dst]);
    printf(" -> %d", dst);
}

// Reverses the parent array output from Dijkstra to form a sequential, traversable Path struct
Path reconstructPath(int* parent, int src, int dst) {
    Path p;
    p.count = 0;
    p.active = false;

    // Validate path existence
    if (dst == -1 || (parent[dst] == -1 && src != dst)) {
        return p;
    }

    int current = dst;
    int tempPath[100];
    int tempCount = 0;

    // Trace backwards from destination to source
    while (current != -1) {
        tempPath[tempCount++] = current;
        if (current == src) break;
        current = parent[current];
    }

    // Reverse the temporary array to get chronological order (source to destination)
    for (int i = 0; i < tempCount; i++) {
        p.nodes[i] = tempPath[tempCount - 1 - i];
    }

    p.count = tempCount;
    p.active = true;

    return p;
}

// ============================================================================
// VISUAL RENDERING (RAYLIB)
// ============================================================================

// Calculates 2D screen coordinates for rendering graph nodes
void computePosition(Graph* graph) {
    if (!graph || graph->numVertices <= 0) {
        return;
    }

    // Hardcoded layout for the 5-city map scenario
    if (graph->numVertices == 5) {
        graph->positions[0] = (Vector2){ 150.0f, 150.0f }; // Jerusalem (Top Left)
        graph->positions[1] = (Vector2){ 350.0f, 250.0f }; // Tel Aviv (Middle)
        graph->positions[2] = (Vector2){ 550.0f, 150.0f }; // Haifa (Top Right)
        graph->positions[3] = (Vector2){ 700.0f, 320.0f }; // Node 3 (Far Right)
        graph->positions[4] = (Vector2){ 250.0f, 480.0f }; // Beersheba (Bottom Left)
        return;
    }

    // Dynamic circular layout for general graphs
    Vector2 center = { 400.0f, 300.0f };
    float radius = 180.0f;
    for (int i = 0; i < graph->numVertices; i++) {
        float angle = (float)i * (2.0f * PI / graph->numVertices);
        graph->positions[i].x = center.x + cosf(angle) * radius;
        graph->positions[i].y = center.y + sinf(angle) * radius;
    }
}

// Renders the visual representation of the graph using raylib
void drawGraph(Graph* graph, Path path) {
    if (!graph) return;

    const char* cityNames[] = {"Jerusalem", "Tel Aviv", "Haifa", "Eilat", "Beersheba", "Ashdod", "Tiberias"};
    ClearBackground((Color){ 240, 242, 245, 255 });

    // Pass 1: Draw all edges and weights
    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Vector2 startPos = graph->positions[i];
            Vector2 endPos = graph->positions[temp->dest];

            Color edgeColor = (Color){ 160, 160, 160, 255 };
            float thickness = 2.0f;

            // Highlight the active shortest path in yellow
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

            // Compute vector components for drawing arrows and weight labels
            float dx = endPos.x - startPos.x;
            float dy = endPos.y - startPos.y;
            float length = sqrtf(dx * dx + dy * dy);

            if (length > 0) {
                Vector2 unitDir = { dx / length, dy / length };

                // Draw directional arrow near the destination node
                Vector2 arrowPos = { endPos.x - unitDir.x * 28, endPos.y - unitDir.y * 28 };
                DrawPoly(arrowPos, 3, 7, atan2f(dy, dx) * RAD2DEG, edgeColor);

                // Position and draw the edge weight text box
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

    // Pass 2: Draw all vertices to render them above the edges
    for (int i = 0; i < graph->numVertices; i++) {
        Vector2 pos = graph->positions[i];
        float radius = 22.0f;

        DrawCircleV(pos, radius, (Color){ 44, 62, 80, 255 });
        DrawCircleLinesV(pos, radius, (Color){ 52, 152, 219, 255 });

        // Draw node ID number
        char idStr[16];
        sprintf(idStr, "%d", i);
        int idWidth = MeasureText(idStr, 18);
        DrawText(idStr, pos.x - idWidth / 2, pos.y - 9, 18, WHITE);

        // Draw descriptive city names if applicable
        if (i < 7) {
            int cityFontSize = 16;
            int cityNameWidth = MeasureText(cityNames[i], cityFontSize);
            float yOffset = (pos.y < 300) ? -45 : 30;

            DrawText(cityNames[i], pos.x - (cityNameWidth / 2), pos.y + yOffset, cityFontSize, (Color){ 44, 62, 80, 255 });
        }
    }
}

// ============================================================================
// ENTITY & PASSENGER MOVEMENT LOGIC
// ============================================================================

// Updates the visual interpolation parameters of a single moving entity
void updateEntity(Entity* entity, Graph* graph, Path* path) {
    if (!entity->isMoving || !path->active || entity->currentPathIndex >= path->count - 1) {
        return;
    }

    int u = path->nodes[entity->currentPathIndex];
    int v = path->nodes[entity->currentPathIndex + 1];

    int weight = 1;
    Node* temp = graph->adjLists[u];

    // Find the edge weight to determine movement speed/duration
    while (temp) {
        if (temp->dest == v) {
            weight = temp->weight;
            break;
        }
        temp = temp->next;
    }

    // Handle waiting state at intersections
    if (entity->isWaiting) {
        entity->frameCounter++;
        if (entity->frameCounter >= 60) {
            entity->isWaiting = false;
            entity->frameCounter = 0;
            entity->currentStep = 0;
        }
        return;
    }

    entity->frameCounter++;

    // Execute an interpolation step based on the frame interval
    if (entity->frameCounter >= 18) {
        entity->currentStep++;
        entity->frameCounter = 0;

        float t = (float)entity->currentStep / weight;
        if (t > 1.0f) t = 1.0f;

        // Linear interpolation formula: P = P0 + t*(P1 - P0)
        entity->currentPos.x = graph->positions[u].x + t * (graph->positions[v].x - graph->positions[u].x);
        entity->currentPos.y = graph->positions[u].y + t * (graph->positions[v].y - graph->positions[u].y);

        // Check if destination node is reached
        if (entity->currentStep >= weight) {
            entity->currentPathIndex++;

            if (entity->currentPathIndex < path->count - 1) {
                entity->isWaiting = true;
            } else {
                entity->isMoving = false;
            }
        }
    }
}

<<<<<<< HEAD
// Calculates and initializes the routing state and data for a single passenger agent
=======
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

/* Dynamic path calculation for a single passenger using Dijkstra's output */
>>>>>>> 4b300ac321072b3c65de9355f7d120d36651b71a
void calculatePassengerRoute(Graph* graph, Passenger* passenger, int src, int dst) {
    // Validate that graph and passenger pointers are safely allocated
    if (graph == NULL || passenger == NULL) {
        printf("[ERROR] calculatePassengerRoute received a NULL graph or passenger pointer!\n");
        return;
    }

    // Validate node boundary constraints to prevent out-of-bounds array access
    if (src < 0 || src >= graph->numVertices || dst < 0 || dst >= graph->numVertices) {
        printf("[WARNING] Invalid src (%d) or dst (%d) for passenger route calculation.\n", src, dst);
        passenger->shortestPath.active = false;
        passenger->simulationFinished = true;
        return;
    }

    int* parent = (int*)malloc(graph->numVertices * sizeof(int));
    if (parent == NULL) {
        printf("[ERROR] Memory allocation failed for parent array in route calculation.\n");
        return;
    }

    // Compute path and update agent structures
    dijkstra(graph, src, dst, parent);
    passenger->shortestPath = reconstructPath(parent, src, dst);

    passenger->simulationFinished = false;
    passenger->carRotation = 0.0f;

    // Configure starting physical coordinates if a valid path exists
    if (passenger->shortestPath.active && passenger->shortestPath.count > 0) {
        int firstNode = passenger->shortestPath.nodes[0];

        if (graph->positions != NULL) {
            passenger->movingEntity.currentPos = graph->positions[firstNode];
        } else {
            passenger->movingEntity.currentPos = (Vector2){ 0.0f, 0.0f };
        }

        passenger->movingEntity.currentPathIndex = 0;
        passenger->movingEntity.frameCounter = 0;
        passenger->movingEntity.currentStep = 0;
        passenger->movingEntity.isWaiting = false;
    } else {
        passenger->simulationFinished = true; // Mark as finished immediately if unreachable
    }

    free(parent);
}

// Iterates through all active passengers and updates their physical and rotational tracking metrics
void updateAllPassengers(Graph* graph, Passenger passengers[], int count, bool isRunning) {
    if (!isRunning || graph == NULL || passengers == NULL) return;

    for (int i = 0; i < count; i++) {
        Passenger* p = &passengers[i];

        if (p->simulationFinished || !p->shortestPath.active) continue;

        // Check if the passenger is still traversing the path
        if (p->movingEntity.currentPathIndex < p->shortestPath.count - 1) {
            int currNode = p->shortestPath.nodes[p->movingEntity.currentPathIndex];
            int nextNode = p->shortestPath.nodes[p->movingEntity.currentPathIndex + 1];

            // Safety boundary check
            if (currNode < 0 || currNode >= graph->numVertices || nextNode < 0 || nextNode >= graph->numVertices) {
                p->simulationFinished = true;
                continue;
            }

            // Extract the edge weight to dictate dynamic traversal speed
            float weight = 1.0f;
            Node* temp = graph->adjLists[currNode];
            while (temp) {
                if (temp->dest == nextNode) {
                    weight = (float)temp->weight;
                    break;
                }
                temp = temp->next;
            }

            if (weight <= 0.0f) weight = 1.0f; // Division by zero protection

            float baseSpeed = 5.0f;
            float dynamicSpeed = baseSpeed / weight;

            if (graph->positions == NULL) continue;

            // Calculate directional vectors and distance to target node
            Vector2 targetPos = graph->positions[nextNode];
            float dx = targetPos.x - p->movingEntity.currentPos.x;
            float dy = targetPos.y - p->movingEntity.currentPos.y;
            float distance = sqrtf(dx * dx + dy * dy);

            // Dynamically set vehicle rendering rotation based on trajectory vector
            p->carRotation = (atan2f(dy, dx) * (180.0f / PI)) + 180.0f;

            // Move vehicle towards destination or snap to target if close enough
            if (distance > dynamicSpeed) {
                p->movingEntity.isWaiting = false;
                p->movingEntity.currentPos.x += (dx / distance) * dynamicSpeed;
                p->movingEntity.currentPos.y += (dy / distance) * dynamicSpeed;
            } else {
                p->movingEntity.currentPos = targetPos;
<<<<<<< HEAD
                p->movingEntity.currentPathIndex++;
                p->movingEntity.isWaiting = false;
            }
        } else {
            p->simulationFinished = true; // Destination reached
=======
                p->movingEntity.isWaiting = true;
            }
        } else {
            // Free the final destination node semaphore when the passenger permanently finishes its journey
            int finalNode = p->shortestPath.nodes[p->movingEntity.currentPathIndex];
            sem_post(&(graph->semaphores[finalNode]));
            p->simulationFinished = true;

            // --- FIX TO PREVENT DEADLOCK: Reset entity status so it doesn't block behind-vehicles ---
            p->movingEntity.isMoving = false;
            p->movingEntity.isWaiting = false;
            p->movingEntity.frameCounter = 0;
>>>>>>> 4b300ac321072b3c65de9355f7d120d36651b71a
        }
    }
}
