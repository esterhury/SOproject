#include <stdio.h>
#include "graph.h"

int main() {
    int src, dst;
    
    Graph* graph = loadGraphFromFile("input.txt", &src, &dst);

    if (graph == NULL) {
        return 1;
    }

    if (src == dst) {
        printf("0\n");
    } else {
        printf("Graph loaded successfully. Ready for Dijkstra calculations.\n");
    }

    freeGraph(graph);
    
    return 0;
}
