#ifndef GRAPH_H
#define GRAPH_H

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
} Graph;

Graph* createGraph(int vertices);
void addEdge(Graph* graph, int src, int dest, int weight);
Graph* loadGraphFromFile(const char* filename, int* startNode, int* endNode);
void freeGraph(Graph* graph);

// Algorithm functions
int dijkstra(Graph* graph, int src, int dst, int parent[]);
void printPath(int* parent, int src, int dst);

#endif