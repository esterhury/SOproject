#ifndef GRAPH_H
#define GRAPH_H

#include "raylib.h"
#include <stdbool.h>
#include <semaphore.h>

// Timing constants for entity movement rendering animations
#define NODE_WAIT_TIME 60   // Number of frames to pause at a vertex intersection (1 second at 60 FPS)
#define EDGE_STEP_TIME 18   // Frame interval duration before executing a linear interpolation step
#define MAX_PASSENGERS 20   // Maximum administrative array bound capacity for passenger tracking

// Node structure for adjacency list representations of the graph network
typedef struct Node {
    int dest;                  // Destination vertex index of the directed edge
    int weight;                // Non-negative travel cost/distance weight of the edge
    struct Node* next;         // Pointer to the subsequent node in the adjacency linked list
} Node;

// Graph master configuration structure
typedef struct {
    int numVertices;           // Total number of vertices populated in the system
    Node** adjLists;           // Dynamic array of head pointers forming individual adjacency lists
    Vector2* positions;        // Screen coordinate locations (X, Y pixels) assigned to vertices for rendering
    sem_t* semaphores;         // Pointer to a contiguous shared memory array of vertex intersection semaphores
} Graph;


// Container layout storing computed shortest-path route arrays
typedef struct {
    int nodes[100];            // Sequential list of vertex indices tracking the calculated path
    int count;                 // Total count of active nodes mapped within the current trajectory
    bool active;               // System status flag indicating if this path profile contains valid data
} Path;

// Dynamic simulation runtime metrics of a rendering entity
typedef struct {
    Vector2 currentPos;        // Current interpolated screen coordinates of the moving vehicle
    int currentPathIndex;      // Index of the current segment origin node within the path tracker
    int frameCounter;          // Frame register tracking animation step and waiting interval states
    int currentStep;           // Integer tracking current linear interpolation progression units
    bool isWaiting;            // State flag indicating if the vehicle is temporarily paused at an intersection
    bool isMoving;             // State flag indicating if the vehicle is actively translating along an edge
} Entity;

// Unified tracking profile mapping operating system attributes onto visual entities
typedef struct {
    int id;                    // System process identifier (PID assigned to the dedicated child process)
    Path shortestPath;         // Individual shortest-path routing breakdown computed via Dijkstra
    Entity movingEntity;       // Mechanical visual translation state profile of the corresponding vehicle
    float carRotation;         // Current vehicle orientation angle (degrees) relative to its direction vector
    bool simulationFinished;   // Boolean flag indicating if this agent has reached its target terminal destination
} Passenger;

// Core graph management and structural modification interfaces
Graph* createGraph(int vertices);
void addEdge(Graph* graph, int src, int dest, int weight);
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int* numTravelers);
void freeGraph(Graph* graph);
void computePosition(Graph* graph);
void drawGraph(Graph* graph, Path path);

// Algorithmic path-finding and routing utilities
int dijkstra(Graph* graph, int src, int dst, int parent[]);
Path reconstructPath(int* parent, int src, int dst);
void printPath(int* parent, int src, int dst);

// Multi-agent tracking and movement updates
void updateEntity(Entity* entity, Graph* graph, Path* path);
void calculatePassengerRoute(Graph* graph, Passenger* passenger, int src, int dst);
void updateAllPassengers(Graph* graph, Passenger passengers[], int count, bool isRunning);

#endif // GRAPH_H