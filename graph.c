#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "raylib.h"
#include "graph.h"

// Dynamically allocates memory for a new graph structure and initializes its adjacency list
Graph* createGraph(int vertices) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->numVertices = vertices;

    // Allocate memory for an array of pointers representing head nodes of the adjacency lists
    graph->adjLists = (Node**)malloc(vertices * sizeof(Node*));
    for (int i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
    }

    // Allocate memory for storing the 2D visual screen coordinates of each graph vertex
    graph->positions = (Vector2*)malloc(vertices * sizeof(Vector2));
    return graph;
}

// Instantiates a new node and inserts it at the beginning of the source vertex's adjacency list
void addEdge(Graph* graph, int src, int dest, int weight) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->dest = dest;
    newNode->weight = weight;

    // Link the new node to point to the current head of the adjacency list
    newNode->next = graph->adjLists[src];

    // Move the head of the adjacency list to point to this new node
    graph->adjLists[src] = newNode;
}

// Loads graph structural architecture and parses dynamic travelers queries from input configuration file
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int* numTravelers) {
    // Open the input configuration file for reading operations only
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file.\n");
        return NULL;
    }

    // Read the total number of vertices (n) and edges (m) from the configuration file layout
    int n, m;
    if (fscanf(file, "%d %d", &n, &m) != 2) {
        fclose(file);
        return NULL;
    }

    // Allocate memory and initialize the master graph structure metadata
    Graph* graph = createGraph(n);

    // Loop through all defined structural edges and link them to the adjacency list maps
    for (int i = 0; i < m; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) == 3) {
            // Validate that edge weights are non-negative to ensure core Dijkstra calculation correctness
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

    // Scan the configuration template line-by-line to locate the specific travelers section header
    while (fgets(line, sizeof(line), file)) {
        // Match the travelers tracking keyword layout to begin parallel queries parsing
        if (strstr(line, "travelers") != NULL) {
            foundTravelers = true;
            break;
        }
    }

    // Handle structural errors gracefully if the required travelers context block is missing
    if (!foundTravelers) {
        printf("Error: No Travelers found.\n");
        *numTravelers = 0;
        *sourcesArray = NULL;
        *destsArray = NULL;
        fclose(file);
        return graph;
    }

    // Read total discrete tracking target passenger agent entries declared right under header
    if (fscanf(file, "%d", numTravelers) != 1) {
        printf("Error: could not read number of travelers.\n");
        *numTravelers = 0;
        fclose(file);
        return graph;
    }

    // Dynamic memory allocation on heap to establish distinct tracking coordinate arrays
    *sourcesArray = (int*)malloc((*numTravelers) * sizeof(int));
    *destsArray = (int*)malloc((*numTravelers) * sizeof(int));

    // Verify system memory context allocations succeeded cleanly without throwing segments fault
    if (*sourcesArray == NULL || *destsArray == NULL) {
        printf("Error: Memory allocation failed for travelers arrays.\n");
        *numTravelers = 0;
        fclose(file);
        return graph;
    }

    // Map source and destination query pairs directly into target logical array layouts
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

// Safely deallocates all heap-allocated graph memory, including linked list nodes and inner arrays
void freeGraph(Graph* graph) {
    if (!graph) return;

    // Traverse each vertex and free all nodes allocated in its adjacency linked list
    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Node* toDelete = temp;
            temp = temp->next;
            free(toDelete);
        }
    }

    // Free the master pointer arrays and the structural container itself
    free(graph->adjLists);
    free(graph->positions);
    free(graph);
}

// Implements Dijkstra's algorithm to compute shortest paths and records structural parents for backtracking
int dijkstra(Graph* graph, int src, int dst, int parent[]) {
    if (graph == NULL || src < 0 || src >= graph->numVertices || dst < 0 || dst >= graph->numVertices) {
        return -1;
    }

    int numVertices = graph->numVertices;
    int* distances = (int*)malloc(numVertices * sizeof(int));
    int* visited = (int*)malloc(numVertices * sizeof(int));

    // Initialize all tracking metrics: paths are undiscovered (infinity) and nodes are unvisited
    for (int i = 0; i < numVertices; i++) {
        distances[i] = INT_MAX;
        visited[i] = 0;
        parent[i] = -1;
    }

    // Distance from the origin vertex to itself is always zero
    distances[src] = 0;

    // Find the shortest path for all vertices sequentially
    for (int i = 0; i < numVertices - 1; i++) {
        int minDistance = INT_MAX;
        int u = -1;

        // Select the unvisited vertex with the minimum distance metric calculated so far
        for (int v = 0; v < numVertices; v++) {
            if (visited[v] == 0 && distances[v] <= minDistance) {
                minDistance = distances[v];
                u = v;
            }
        }

        // Terminate parsing if remaining nodes are unreachable or destination is safely resolved
        if (u == -1 || distances[u] == INT_MAX || u == dst) {
            break;
        }

        // Mark the selected vertex as processed
        visited[u] = 1;

        // Relax adjacent vertices by updating weight metrics through the selected node 'u'
        Node* current = graph->adjLists[u];
        while (current) {
            int v = current->dest;
            int weight = current->weight;

            if (!visited[v] && distances[u] != INT_MAX && (distances[u] + weight < distances[v])) {
                distances[v] = distances[u] + weight;
                parent[v] = u; // Store predecessor mapping for path reconstruction
            }
            current = current->next;
        }
    }

    int result = distances[dst];
    free(distances);
    free(visited);

    return result; // Returns total cost of shortest path, or INT_MAX if no path exists
}

// Backtracks through the parent array from destination to source to construct a sequential node array
Path reconstructPath(int* parent, int src, int dst) {
    Path p;
    p.count = 0;
    p.active = false;

    // Return an inactive path container if endpoints are invalid or disconnected
    if (dst == -1 || (parent[dst] == -1 && src != dst)) {
        return p;
    }

    int current = dst;
    int tempPath[100];
    int tempCount = 0;

    // Collect routing elements backward from target destination back to origin source
    while (current != -1) {
        tempPath[tempCount++] = current;
        if (current == src) break;
        current = parent[current];
    }

    // Reverse the temporary path list into the final path object to order it from source to destination
    for (int i = 0; i < tempCount; i++) {
        p.nodes[i] = tempPath[tempCount - 1 - i];
    }

    p.count = tempCount;
    p.active = true;

    return p;
}

// Distributes graph vertices logically across a grid topology layout with mild randomized offsets
void computePosition(Graph* graph) {
    if (!graph || graph->numVertices <= 0) {
        return;
    }

    int cols = 4;
    int rows = (graph->numVertices / cols) + 1;
    float cellWidth = 800.0f / cols;
    float cellHeight = 600.0f / rows;

    // Assign pixel locations on screen based on matrix grid calculations with spatial noise
    for (int i = 0; i < graph->numVertices; i++) {
        int r = i / cols;
        int c = i % cols;
        float baseX = (c * cellWidth) + (cellWidth / 2.0f);
        float baseY = (r * cellHeight) + (cellHeight / 2.0f);

        // Apply controlled variations to avoid linear overlapping alignments
        graph->positions[i].x = baseX + GetRandomValue(-20, 20);
        graph->positions[i].y = baseY + GetRandomValue(-20, 20);
    }
}

// Iterates through the graph network mapping connections, directional arrows, and weights onto the display
void drawGraph(Graph* graph, Path path) {
    if (!graph) return;

    const char* cityNames[] = {"Jerusalem", "Tel Aviv", "Haifa", "Eilat", "Beersheba", "Ashdod", "Tiberias"};

    // Render directed edges connecting vertices
    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Vector2 startPos = graph->positions[i];
            Vector2 endPos = graph->positions[temp->dest];

            Color edgeColor = (Color){ 160, 160, 160, 255 }; // Default structural edge color
            float thickness = 2.0f;

            // =================================================================
            // MILESTONE 4 FEATURE: Concurrent Path Highlight Check
            // =================================================================
            // Highlight the path if the current edge is part of an active agent's trajectory
            if (path.active) {
                for (int k = 0; k < path.count - 1; k++) {
                    if (path.nodes[k] == i && path.nodes[k+1] == temp->dest) {
                        edgeColor = YELLOW;
                        thickness = 4.5f;
                        break;
                    }
                }
            }

            // Draw the structural link line between node centers
            DrawLineEx(startPos, endPos, thickness, edgeColor);

            // Compute vector metrics for directional arrowhead markers and overlay text indicators
            float dx = endPos.x - startPos.x;
            float dy = endPos.y - startPos.y;
            float length = sqrtf(dx * dx + dy * dy);

            if (length > 0) {
                Vector2 unitDir = { dx / length, dy / length };

                // Place directional arrowheads outside the bounding radius circumference of target node circles
                Vector2 arrowPos = { endPos.x - unitDir.x * 28, endPos.y - unitDir.y * 28 };
                DrawPoly(arrowPos, 3, 7, atan2f(dy, dx) * RAD2DEG, edgeColor);

                // Calculate position offsets for drawing edge cost weights at one-third of segment length
                Vector2 weightPos = {
                    startPos.x + unitDir.x * (length * 0.33f),
                    startPos.y + unitDir.y * (length * 0.33f)
                };

                char weightStr[12];
                sprintf(weightStr, "%d km", temp->weight);
                int fontSize = 14;
                int textWidth = MeasureText(weightStr, fontSize);

                // Draw background box for legible edge cost reading
                DrawRectangle(weightPos.x - (textWidth/2 + 3), weightPos.y - 9, textWidth + 6, 18, (Color){ 231, 76, 60, 255 });
                DrawText(weightStr, weightPos.x - textWidth/2, weightPos.y - 7, fontSize, WHITE);
            }
            temp = temp->next;
        }
    }

    // Render nodes visually on screen with embedded index value IDs and label tags
    for (int i = 0; i < graph->numVertices; i++) {
        Vector2 pos = graph->positions[i];
        float radius = 22.0f;

        // Draw node circles
        DrawCircleV(pos, radius, (Color){ 44, 62, 80, 255 });
        DrawCircleLinesV(pos, radius, (Color){ 52, 152, 219, 255 });

        // Print vertex identifier numerical tag centered within the circle
        char idStr[16];
        sprintf(idStr, "%d", i);
        int idWidth = MeasureText(idStr, 18);
        DrawText(idStr, pos.x - idWidth / 2, pos.y - 9, 18, WHITE);

        // Map representative geographical name labels to the initial 7 structural nodes
        if (i < 7) {
            int cityFontSize = 16;
            int cityNameWidth = MeasureText(cityNames[i], cityFontSize);
            float yOffset = (pos.y < 300) ? -45 : 30; // Flip label positions based on quadrant elevation to avoid overlap

            DrawText(cityNames[i], pos.x - (cityNameWidth / 2), pos.y + yOffset, cityFontSize, (Color){ 44, 62, 80, 255 });
        }
    }
}

// Advances an entity's linear translation coordinates step-by-step between tracking path intersections
void updateEntity(Entity* entity, Graph* graph, Path* path) {
    // =================================================================
    // MILESTONE 4 START: Concurrent Multi-Agent Position Interpolation
    // =================================================================
    if (!entity->isMoving || !path->active || entity->currentPathIndex >= path->count - 1) {
        return;
    }

    // Fetch endpoints of current edge segment traversal path
    int u = path->nodes[entity->currentPathIndex];
    int v = path->nodes[entity->currentPathIndex + 1];

    // Read weight value from graph properties to dictate time-based distance scales
    int weight = 1;
    Node* temp = graph->adjLists[u];
    while (temp) {
        if (temp->dest == v) {
            weight = temp->weight;
            break;
        }
        temp = temp->next;
    }

    // Handle station pause delay when intersection boundaries are intersected
    if (entity->isWaiting) {
        entity->frameCounter++;
        if (entity->frameCounter >= 60) { // Unblock moving states after 60 frames (1 second at 60FPS)
            entity->isWaiting = false;
            entity->frameCounter = 0;
            entity->currentStep = 0;
        }
        return;
    }

    entity->frameCounter++;

    // Increment movement calculations every 18 frames
    if (entity->frameCounter >= 18) {
        entity->currentStep++;
        entity->frameCounter = 0;

        // Calculate interpolation factor 't' relative to edge weight
        float t = (float)entity->currentStep / weight;
        if (t > 1.0f) t = 1.0f;

        // Perform linear interpolation to find intermediate vehicle location coordinates
        entity->currentPos.x = graph->positions[u].x + t * (graph->positions[v].x - graph->positions[u].x);
        entity->currentPos.y = graph->positions[u].y + t * (graph->positions[v].y - graph->positions[u].y);

        // Check if traveler has arrived at the target end node of the current edge
        if (entity->currentStep >= weight) {
            entity->currentPathIndex++;

            // Shift index to next edge node or finalize movement if total route concludes
            if (entity->currentPathIndex < path->count - 1) {
                entity->isWaiting = true; // Trigger station delay state
            } else {
                entity->isMoving = false;  // Destination achieved
            }
        }
    }
}