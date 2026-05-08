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

Graph* createGraph(int vertices);
void addEdge(Graph* graph, int src, int dest, int weight);
Graph* loadGraphFromFile(const char* filename, int* startNode, int* endNode);
void freeGraph(Graph* graph);
void computePosition(Graph* graph);

// Algorithm functions
int dijkstra(Graph* graph, int src, int dst, int parent[]);
void printPath(int* parent, int src, int dst);

#endif