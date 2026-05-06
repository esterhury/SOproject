#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "graph.h"


 //Recursively prints the shortest path from source to destination.
void printPath(int parent[], int j) {
    // Base case: If node is the source (has no predecessor)
    if (parent[j] == -1) {
        printf("%d", j);
        return;
    }
    printPath(parent, parent[j]);
    printf(" -> %d", j);
}

int main(int argc, char* argv[]) {
    int src, dst;

    // Use command line argument for the filename as per technical requirements
    char* filename = (argc > 1) ? argv[1] : "input.txt";

    // Load graph data and the query (src, dst) from the file
    Graph* graph = loadGraphFromFile(filename, &src, &dst);

    if (graph == NULL) {
        return 1; // Error message is already handled inside loadGraphFromFile
    }

    // Allocate memory for the parent array to store path predecessors
    int* parent = (int*)malloc(graph->numVertices * sizeof(int));
    if (parent == NULL) {
        freeGraph(graph);
        return 1;
    }

    // Execute Dijkstra's algorithm and get the total distance
    int totalDistance = dijkstra(graph, src, dst, parent);
    if (totalDistance == INT_MAX || totalDistance == -1) {
        printf("No path found\n");
    } else if (src == dst) {
        // Source and destination are the same
        printf("0\n");
    } else {
        // Path found - print the route and the total weight
        printPath(parent, dst);
        printf("\n%d\n", totalDistance);
    }

    // Clean up memory to prevent leaks
    free(parent);
    freeGraph(graph);

    return 0;
}