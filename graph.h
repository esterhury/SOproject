#ifndef GRAPH_H
#define GRAPH_H
#include "raylib.h"

// Node structure for adjacency list
typedef struct Node {
    int dest;
    int weight;
    struct Node* next;
} Node;

// Graph structure
typedef struct {
    int numVertices;
    Node** adjLists;
    Vector2* positions;
} Graph;

// Structure to store and manage the calculated shortest path
typedef struct {
    int nodes[100];     // Array of node indices forming the path
    int count;          // Number of nodes in the path
    bool active;        //Flag to indicate if a path is currently loaded
} Path;

Graph* createGraph(int vertices);
void addEdge(Graph* graph, int src, int dest, int weight);
Graph* loadGraphFromFile(const char* filename, int* startNode, int* endNode);
void freeGraph(Graph* graph);
int dijkstra(Graph* graph, int src, int dst, int parent[]);
void printPath(int* parent, int src, int dst);
void computePosition(Graph* graph);
void drawGraph(Graph* graph, Path path);
Path reconstructPath(int* parent, int src, int dst);

#endif