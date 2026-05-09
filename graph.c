#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "raylib.h"
#include "graph.h"

// Create a new graph with a given number of vertices
Graph* createGraph(int vertices) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->numVertices = vertices;
    graph->adjLists = (Node**)malloc(vertices * sizeof(Node*));

    for (int i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
    }
    graph->positions = (Vector2*)malloc(vertices * sizeof(Vector2));
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

// Load graph data and the query from a file
Graph* loadGraphFromFile(const char* filename, int* startNode, int* endNode) {
    // Open the input file for reading
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file.\n");
        return NULL;
    }
    // Read the number of vertices and edges
    int n, m;
    if (fscanf(file, "%d %d", &n, &m) != 2) {
        fclose(file);
        return NULL;
    }
    // Initialize the graph structure
    Graph* graph = createGraph(n);
    // Read each edge and add it to the adjacency list
    for (int i = 0; i < m; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) == 3) {
            // Validate that weights are non-negative
            if (w < 0) {
                printf("Error: Invalid input. Weights cannot be negative.\n");
                fclose(file);
                // Free graph memory if an error occurs
                freeGraph(graph);
                return NULL;
            }
            addEdge(graph, u, v, w);
        }
    }
    // Read the source and destination nodes for the pathfinding query
    if (fscanf(file, "%d %d", startNode, endNode) != 2) {
        printf("Warning: Could not read start/end nodes. Setting to -1.\n");
        *startNode = -1;
        *endNode = -1;
    }
    fclose(file);
    return graph;
}

//Clean up memory to prevent leaks
void freeGraph(Graph* graph) {
    if (!graph) return;
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

// Initializes distance and visited arrays, setting the source node's distance to 0.
static void initDistances(int distances[], int visited[], int numVertices, int src) {
    for (int i = 0; i < numVertices; i++) {
        distances[i] = INT_MAX;
        visited[i] = 0;
    }
    distances[src] = 0;
}

// Relaxes an edge and updates the shortest path if a better one is found.
static void relax(int u, int v, int weight, int distances[], int visited[], int parent[]) {
    if (!visited[v] && distances[u] != INT_MAX && (distances[u] + weight < distances[v])) {
        distances[v] = distances[u] + weight;
        parent[v] = u;
    }
}

// Finds the shortest path cost from source to destination.
int dijkstra(Graph* graph, int src, int dst, int parent[]) {
    if (graph == NULL || src < 0 || src >= graph->numVertices || dst < 0 || dst >= graph->numVertices) {
        return -1;
    }

    int numVertices = graph->numVertices;

    // Allocate memory for distances and visited status
    int* distances = (int*)malloc(numVertices * sizeof(int));
    int* visited = (int*)malloc(numVertices * sizeof(int));

    // Initialize all nodes: distance to infinity, visited to false, and no parent
    for (int i = 0; i < numVertices; i++) {
        distances[i] = INT_MAX;
        visited[i] = 0;
        parent[i] = -1; // -1 indicates the node has no predecessor yet
    }
    // Distance from source to itself is always 0
    distances[src] = 0;
    for (int i = 0; i < numVertices - 1; i++) {
        int minDistance = INT_MAX;
        int u = -1;
        // Extract the unvisited node with the smallest known distance
        for (int v = 0; v < numVertices; v++) {
            if (visited[v] == 0 && distances[v] <= minDistance) {
                minDistance = distances[v];
                u = v;
            }
        }
        // If no reachable nodes are left or we reached the target, stop
        if (u == -1 || distances[u] == INT_MAX || u == dst) {
            break;
        }
        visited[u] = 1; // Mark node as processed
        // Relaxation step: check all neighbors of the current node 'u'
        Node* current = graph->adjLists[u];
        while (current) {
            int v = current->dest;
            int weight = current->weight;

            // If a shorter path to 'v' is found via 'u', update distance and parent
            if (!visited[v] && distances[u] != INT_MAX && (distances[u] + weight < distances[v])) {
                distances[v] = distances[u] + weight;
                parent[v] = u; // Store 'u' as the parent of 'v' for path tracking
            }
            current = current->next;
        }
    }
    int result = distances[dst];
    // Free local memory to avoid leaks
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

// end of milestone 1


/*Calculates unique X,Y coordinates for each node using a grid-based distribution
to ensure the graph is readable and nodes do not overlap.*/
void computePosition(Graph* graph) {
    if (!graph || graph->numVertices <= 0) {
        return;
    }
    int cols = 4;
    int rows = (graph->numVertices / cols) + 1;
    float cellWidth = 800.0f / cols;
    float cellHeight = 600.0f / rows;
    for (int i = 0; i < graph->numVertices; i++) {
        int r = i / cols;
        int c = i % cols;
        float baseX = (c * cellWidth) + (cellWidth / 2.0f);
        float baseY = (r * cellHeight) + (cellHeight / 2.0f);

        graph->positions[i].x = baseX + GetRandomValue(-20, 20);
        graph->positions[i].y = baseY + GetRandomValue(-20, 20);
    }
}
// Visualizes the graph by drawing edges and then nodes on top
void drawGraph(Graph* graph) {
    if (!graph) return;

    const char* cityNames[] = {"Jerusalem", "Tel Aviv", "Haifa", "Eilat", "Beersheba", "Ashdod", "Tiberias"};
    ClearBackground((Color){ 240, 242, 245, 255 });

    // Edges, Optimized Arrows, and Offset Weights
    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Vector2 startPos = graph->positions[i];
            Vector2 endPos = graph->positions[temp->dest];

            // Draw the main road
            DrawLineEx(startPos, endPos, 2.0f, (Color){ 160, 160, 160, 255 });

            float dx = endPos.x - startPos.x;
            float dy = endPos.y - startPos.y;
            float length = sqrtf(dx * dx + dy * dy);

            if (length > 0) {
                Vector2 unitDir = { dx / length, dy / length };

                // Arrowhead Positioned near the destination node
                Vector2 arrowPos = { endPos.x - unitDir.x * 28, endPos.y - unitDir.y * 28 };
                DrawPoly(arrowPos, 3, 7, atan2f(dy, dx) * RAD2DEG, (Color){ 52, 73, 94, 255 });
                Vector2 weightPos = {
                    startPos.x + unitDir.x * (length * 0.33f),
                    startPos.y + unitDir.y * (length * 0.33f)
                };

                char weightStr[12];
                sprintf(weightStr, "%d km", temp->weight);
                int fontSize = 14;
                int textWidth = MeasureText(weightStr, fontSize);

                // Drawing a clean label for the distance
                DrawRectangle(weightPos.x - (textWidth/2 + 3), weightPos.y - 9, textWidth + 6, 18, (Color){ 231, 76, 60, 255 });
                DrawText(weightStr, weightPos.x - textWidth/2, weightPos.y - 7, fontSize, WHITE);
            }
            temp = temp->next;
        }
    }

    // Nodes and City Labels
    for (int i = 0; i < graph->numVertices; i++) {
        Vector2 pos = graph->positions[i];
        float radius = 22.0f;

        DrawCircleV(pos, radius, (Color){ 44, 62, 80, 255 });
        DrawCircleLinesV(pos, radius, (Color){ 52, 152, 219, 255 });

        char idStr[5];
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
// end of milestone 2