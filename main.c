#include "raylib.h"
#include "graph.h"
#include <stdio.h>

int main() {
    // 1. Load your graph from Milestone 1
    int src, dst;
    Graph* graph = loadGraphFromFile("input.txt", &src, &dst);
    if (graph == NULL) return 1;

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
