#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"

int main() {
    int src, dst;

    Graph* graph = loadGraphFromFile("input.txt", &src, &dst);
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

        // continue the graph!
        DrawText("Raylib Framework: READY", 240, 250, 20, DARKGRAY);
        DrawText("Graph Data: LOADED", 280, 290, 20, MAROON);

        EndDrawing();
    }
    free(parent);
    freeGraph(graph);
    CloseWindow();

    return 0;
}