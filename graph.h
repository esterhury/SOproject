#ifndef GRAPH_H
#define GRAPH_H

#include "raylib.h"
#include <stdbool.h>
#include <semaphore.h> // Ensure semaphore library is included

// ============================================================================
// CONSTANTS & MACROS
// ============================================================================

#define NODE_WAIT_TIME 60
#define EDGE_STEP_TIME 18
#define MAX_PASSENGERS 20

// ============================================================================
// DATA STRUCTURES
// ============================================================================

typedef struct Node {
    int dest;
    int weight;
    struct Node* next;
} Node;

// Graph master configuration structure
typedef struct {
    int numVertices;
    Node** adjLists;
    Vector2* positions;
    sem_t* semaphores; // Added semaphore array for synchronization
} Graph;

typedef struct {
    int nodes[100];
    int count;
    bool active;
} Path;

typedef struct {
    Vector2 currentPos;
    int currentPathIndex;
    int frameCounter;
    int currentStep;
    bool isWaiting;
    bool isMoving;
} Entity;

typedef struct {
    int id;
    Path shortestPath;
    Entity movingEntity;
    float carRotation;
    bool simulationFinished;
    int priority;
} Passenger;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// --- Core Graph Management ---
Graph* createGraph(int vertices);
void addEdge(Graph* graph, int src, int dest, int weight);
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int** prioritiesArray, int* numTravelers);
void freeGraph(Graph* graph);
void computePosition(Graph* graph);
void drawGraph(Graph* graph, Path path);

// --- Algorithmic Path-Finding Utilities ---
int dijkstra(Graph* graph, int src, int dst, int parent[]);
Path reconstructPath(int* parent, int src, int dst);
void printPath(int* parent, int src, int dst);

// --- Multi-Agent Tracking and Movement ---
void drawEntity(Entity* entity); // Added missing prototype
void updateEntity(Entity* entity, Graph* graph, Path* path);
void calculatePassengerRoute(Graph* graph, Passenger* passenger, int src, int dst);
void updateAllPassengers(Graph* graph, Passenger passengers[], int count, bool isRunning);

#endif // GRAPH_H