#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>

int main() {
    int src, dst;

    // Load graph and query from file
    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &src, &dst);
    if (graph == NULL) return 1;

    // Compute node positions for visualization
    computePosition(graph);

    // Allocate memory for Dijkstra's parent array
    int* parent = (int*)malloc(graph->numVertices * sizeof(int));

    // Milestone 3- Initialize the path structure
    Path shortestPath = { .active = false, .count = 0 };

    if (src != -1 && dst != -1) {
        // Run Dijkstra to find the shortest distances and parents
        dijkstra(graph, src, dst, parent);

        // Milestone 3- Reconstruct the path for visualization
        shortestPath = reconstructPath(parent, src, dst);
    }

    InitWindow(800, 600, "SO Project - Milestone 3");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Milestone 3- Pass the graph and the path to the draw function
        drawGraph(graph, shortestPath);

        // Display current query info
        if (src != -1 && dst != -1) {
            DrawText(TextFormat("Query: %d to %d", src, dst), 15, 15, 20, DARKBLUE);

            // Display path status
            if (!shortestPath.active && src != dst) {
                DrawText("No Path Found", 15, 40, 18, RED);
            } else {
                DrawText("Path Found - Highlighting Active", 15, 40, 18, GREEN);
            }
        }

        EndDrawing();
    }

    // Cleanup memory
    free(parent);
    freeGraph(graph);
    CloseWindow();

    return 0;
}