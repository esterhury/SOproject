#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>

// Function to draw a car with rotation (Member 3 Visuals)
void DrawCar(Vector2 position, float rotation, Color color) {
    float width = 40.0f;
    float height = 20.0f;
    Rectangle rec = { position.x, position.y, width, height };
    Vector2 origin = { width / 2, height / 2 };
    DrawRectanglePro(rec, origin, rotation, color);

    // Front window to show direction
    Rectangle window = { position.x, position.y, width / 4, height - 6 };
    Vector2 windowOrigin = { -width / 4, (height - 6) / 2 };
    DrawRectanglePro(window, windowOrigin, rotation, SKYBLUE);
}

int main() {
    int src, dst;
    // Load graph and query
    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &src, &dst);
    if (graph == NULL) return 1;

    computePosition(graph);
    int* parent = (int*)malloc(graph->numVertices * sizeof(int));
    Path shortestPath = { .active = false, .count = 0 };

    if (src != -1 && dst != -1) {
        dijkstra(graph, src, dst, parent);
        shortestPath = reconstructPath(parent, src, dst);
    }

    // --- Milestone 3: Control Variables ---
    bool isRunning = false;
    bool simulationFinished = false;
    float carRotation = 0.0f;
    Rectangle playBtn = { 650, 20, 120, 40 };
    Rectangle stopBtn = { 650, 70, 120, 40 };

    // Initialize the Entity according to your struct
    Entity movingEntity = { 0 };
    if (shortestPath.active && shortestPath.count > 0) {
        movingEntity.currentPos = graph->positions[shortestPath.nodes[0]];
        movingEntity.currentPathIndex = 0;
        movingEntity.isMoving = false; // Starts stationary
        movingEntity.isWaiting = true; // Wait at the first node
    }

    InitWindow(800, 600, "SO Project - Final Milestone 3");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // --- Input Handling ---
        Vector2 mousePoint = GetMousePosition();
        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            movingEntity.isMoving = true;
            isRunning = true;
        }
        if (CheckCollisionPointRec(mousePoint, stopBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            movingEntity.isMoving = false;
            isRunning = false;
        }

        // --- Logic Update: Tick-based movement and Wait times ---
        if (isRunning && !simulationFinished) {
            // Calculate rotation for the current edge
            if (movingEntity.currentPathIndex < shortestPath.count - 1) {
                Vector2 current = graph->positions[shortestPath.nodes[movingEntity.currentPathIndex]];
                Vector2 next = graph->positions[shortestPath.nodes[movingEntity.currentPathIndex + 1]];
                carRotation = atan2f(next.y - current.y, next.x - current.x) * (180.0f / PI);
            }

            // Use your updateEntity function logic
            updateEntity(&movingEntity, graph, &shortestPath);

            // Check if reached destination
            if (!movingEntity.isMoving && movingEntity.currentPathIndex >= shortestPath.count - 1) {
                simulationFinished = true;
                isRunning = false;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawGraph(graph, shortestPath);

        // --- Visual Representation ---
        if (shortestPath.active) {
            DrawCar(movingEntity.currentPos, carRotation, MAROON);
        }

        // GUI Buttons
        DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
        DrawText("PLAY", playBtn.x + 35, playBtn.y + 10, 20, BLACK);
        DrawRectangleRec(stopBtn, RED);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        // Finish Message
        if (simulationFinished) {
            DrawRectangle(200, 250, 400, 100, Fade(GOLD, 0.9f));
            DrawText("SIMULATION FINISHED!", 225, 285, 25, BLACK);
        }

        EndDrawing();
    }

    free(parent);
    freeGraph(graph);
    CloseWindow();
    return 0;
}