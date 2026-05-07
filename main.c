#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <stdio.h>

int main() {
    // 1. Load your graph from Milestone 1
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
    // 2. Initialize the Window (Member 1 Task)
    InitWindow(800, 600, "SO Project - Milestone 2");
    SetTargetFPS(60);

    // 3. The Visual Loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // This shows the environment is ready for Member 2 & 3
        DrawText("Raylib Framework: READY", 240, 250, 20, DARKGRAY);
        DrawText("Graph Data: LOADED", 280, 290, 20, MAROON);
        
        EndDrawing();
    }

    // 4. Cleanup
    freeGraph(graph);
    CloseWindow();

    return 0;
}