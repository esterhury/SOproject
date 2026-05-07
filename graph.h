#ifndef GRAPH_H
#define GRAPH_H
#include <raylib.h>

typedef struct Graph Graph;
int dijkstra(Graph* graph, int src, int dst, int* parent);
void printPath(int* parent, int src, int dst);

/* Structure for a node in the adjacency list */
typedef struct Node {
    int dest;
    int weight;
    struct Node* next;
} Node;

/* Structure for the graph */
typedef struct Graph {
    int numVertices;
    Node** adjLists;
    Vector2* positions;
} Graph;

/* Function prototypes */
Graph* createGraph(int vertices);
void addEdge(Graph* graph, int src, int dest, int weight);
Graph* loadGraphFromFile(const char* filename, int* startNode, int* endNode);
void freeGraph(Graph* graph);
void computePosition(Graph* graph);

#endif

