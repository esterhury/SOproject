#ifndef GRAPH_H
#define GRAPH_H

#include "raylib.h"
#include <stdbool.h>

// ============================================================================
// CONSTANTS & MACROS
// ============================================================================

// Timing constants for entity movement rendering animations
#define NODE_WAIT_TIME 60   // Number of frames to pause at a vertex intersection (1 second at 60 FPS)
#define EDGE_STEP_TIME 18   // Frame interval duration before executing a linear interpolation step
#define MAX_PASSENGERS 20   // Maximum administrative array bound capacity for passenger tracking

// ============================================================================
// DATA STRUCTURES
// ============================================================================

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

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// --- Core Graph Management ---

// Allocates memory and initializes a new graph with the specified number of vertices
Graph* createGraph(int vertices);

// Adds a directed edge between a source and destination vertex with a specific weight
void addEdge(Graph* graph, int src, int dest, int weight);

// Parses the input file to build the graph and extract traveler configurations
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int* numTravelers);

// Safely deallocates all memory associated with the graph to prevent memory leaks
void freeGraph(Graph* graph);

// Calculates and assigns visual 2D screen coordinates for each graph vertex
void computePosition(Graph* graph);

// Renders the graph layout, edges, weights, and visually highlights active paths
void drawGraph(Graph* graph, Path path);


// --- Algorithmic Path-Finding Utilities ---

// Executes Dijkstra's algorithm to compute the shortest path distances and parent tree
int dijkstra(Graph* graph, int src, int dst, int parent[]);

// Backtracks through the parent array to build a sequential Path structure
Path reconstructPath(int* parent, int src, int dst);

// Prints the sequence of nodes forming the path to standard output
void printPath(int* parent, int src, int dst);


// --- Multi-Agent Tracking and Movement ---

// Advances a single entity's physical translation along its assigned path
void updateEntity(Entity* entity, Graph* graph, Path* path);

// Initializes routing and state parameters for a specific passenger agent
void calculatePassengerRoute(Graph* graph, Passenger* passenger, int src, int dst);

// Iterates over all active passenger agents to compute and update their visual coordinates
void updateAllPassengers(Graph* graph, Passenger passengers[], int count, bool isRunning);

#endif // GRAPH_H