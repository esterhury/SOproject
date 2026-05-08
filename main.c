#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "graph.h"

int main() {
    int src, dst;
    Graph* graph = loadGraphFromFile("input.txt", &src, &dst);

    if (graph == NULL) return 1;

    int* parent = (int*)malloc(graph->numVertices * sizeof(int));
    if (parent == NULL) {
        freeGraph(graph);
        return 1;
    }

    if (src == dst) {
        printf("0\n");
    } else {
        int weight = dijkstra(graph, src, dst, parent);

        if (weight == INT_MAX || weight == -1) {
            printf("No path found\n");
        } else {
            printPath(parent, src, dst);
            printf("\n%d\n", weight);
        }
    }

    free(parent);
    freeGraph(graph);

    return 0;
}