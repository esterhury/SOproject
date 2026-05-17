#ifndef GRAPH_H
#define GRAPH_H
#include "raylib.h"

#define NODE_WAIT_MS 1000
#define EDGE_STEP_MS 300
#define  NODE_WAIT_TIME 60
#define EDGE_STEP_TIME 18

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

typedef struct {
    Vector2 currentPos;
    int currentPathIndex;
    int frameCounter;
    int currentStep;
    int isWaiting;
    int isMoving;
}Entity;

Graph* createGraph(int vertices);
void addEdge(Graph* graph, int src, int dest, int weight);
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int* numTravelers);
void freeGraph(Graph* graph);
int dijkstra(Graph* graph, int src, int dst, int parent[]);
void printPath(int* parent, int src, int dst);
void computePosition(Graph* graph);
void drawGraph(Graph* graph, Path path);
Path reconstructPath(int* parent, int src, int dst);
void updateEntity(Entity* entity, Graph* graph, Path* path);
void drawEntity(Entity* entity);

#endif