#ifndef GRAPH_H
#define GRAPH_H

typedef struct Graph Graph;
int dijkstra(Graph* graph, int src, int dst, int parent[]);

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
} Graph;

/* Function prototypes */
Graph* createGraph(int vertices);
void addEdge(Graph* graph, int src, int dest, int weight);
Graph* loadGraphFromFile(const char* filename, int* startNode, int* endNode);
void freeGraph(Graph* graph);

#endif

