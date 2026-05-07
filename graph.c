#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "graph.h"

/* Create a new graph with a given number of vertices */
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

/* Add an edge to the graph (directed) */
void addEdge(Graph* graph, int src, int dest, int weight) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->dest = dest;
    newNode->weight = weight;
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;
}

/* Load graph data and the query (src, dst) from a file */
Graph* loadGraphFromFile(const char* filename, int* startNode, int* endNode) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file.\n");
        return NULL;
    }

    int n, m;
    if (fscanf(file, "%d %d", &n, &m) != 2) {
        fclose(file);
        return NULL;
    }

    Graph* graph = createGraph(n);

    for (int i = 0; i < m; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) == 3) {
            /* Check for negative weights as per requirements */
            if (w < 0) {
                printf("Error: Invalid input. Weights cannot be negative.\n");
                fclose(file);
                return NULL;
            }
            addEdge(graph, u, v, w);
        }
    }

    /* Read the last line containing the source and destination for Dijkstra */
    if (fscanf(file, "%d %d", startNode, endNode) != 2) {
        /* Optional: handle missing query */
    }

    fclose(file);
    return graph;
}

/* Clean up memory to prevent leaks */
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
        parent[v] = u; // שמירת ההורה לצורך שחזור מסלול
    }
}
// Finds the shortest path cost from source to destination.
int dijkstra(Graph* graph, int src, int dst, int* parent) {
    // Validate arguments to prevent memory errors
    if (graph == NULL || src < 0 || src >= graph->numVertices || dst < 0 || dst >= graph->numVertices) {
        return -1;
    }

    int numVertices = graph->numVertices;

    // Allocate memory safely and check allocations
    int* distances = (int*)malloc(numVertices * sizeof(int));
    if (distances == NULL) {
        return -1;
    }
    int* visited = (int*)malloc(numVertices * sizeof(int));
    if (visited == NULL) {
        free(distances);
        return -1;
    }
    // אתחול מערך parent ב-1- (מסמן שאין הורה עדיין)
    for (int i = 0; i < numVertices; i++) {parent[i] = -1;}

    // Initialize data structures
    initDistances(distances, visited, numVertices, src);

    for (int i = 0; i < numVertices - 1; i++) {
        int minDistance = INT_MAX;
        int u = -1;

        // Find the closest unvisited vertex
        for (int v = 0; v < numVertices; v++) {
            if (visited[v] == 0 && distances[v] <= minDistance) {
                minDistance = distances[v];
                u = v;
            }
        }

        // If the remaining vertices are unreachable or we reached the destination
        if (u == -1 || distances[u] == INT_MAX || u == dst) {
            break;
        }

        // Mark vertex as visited
        visited[u] = 1;

        // Update shortest path estimates to neighbors
        Node* current = graph->adjLists[u];
        while (current) {
            relax(u, current->dest, current->weight, distances, visited, parent);
            current = current->next;
        }
    }

    int result = distances[dst];

    // Free allocated memory before returning
    free(distances);
    free(visited);

    return result;
}

//  הוספת פונקציית הדפסת המסלול
void printPath(int* parent, int src, int dst) {
    if (dst == src) {
        printf("%d", src);
        return;
    }
    printPath(parent, src, parent[dst]);
    printf(" -> %d", dst);
}

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