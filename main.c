#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>

int main() {
    int src, dst;

    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &src, &dst);
    if (graph == NULL) return 1;

    computePosition(graph);

    int* parent = (int*)malloc(graph->numVertices * sizeof(int));

    if (src != dst) {
        dijkstra(graph, src, dst, parent);
    }

    InitWindow(800, 600, "SO Project - Milestone 2 & 3");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Call the visualization function
        drawGraph(graph);

        // Display current query info
        if (src != -1 && dst != -1) {
            DrawText(TextFormat("Query: %d to %d", src, dst), 15, 15, 20, DARKBLUE);
        }

        EndDrawing();
    }
    free(parent);
    freeGraph(graph);
    CloseWindow();

    return 0;
}