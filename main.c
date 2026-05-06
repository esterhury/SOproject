#include <stdio.h>
#include <stdlib.h>
#include "graph.h"

int main() {
    int src, dst;
    Graph* graph = loadGraphFromFile("input.txt", &src, &dst);
    if (graph == NULL) return 1;

    // הקצאת מערך parent שישמש אותנו
    int* parent = (int*)malloc(graph->numVertices * sizeof(int));

    if (src == dst) {
        printf("0\n");
    } else {
        int weight = dijkstra(graph, src, dst, parent);

        if (weight == -1 || parent[dst] == -1) {
            // בודק אם אין מסלול (INT_MAX או לא ניתן להגיע)
            printf("No path found\n");
        } else {
            printPath(parent, src, dst);
            printf("\n%d\n", weight); // הדפסת המשקל בסוף
        }
    }

    free(parent); // חשוב מאוד כדי למנוע Memory Leak
    freeGraph(graph);
    return 0;
}