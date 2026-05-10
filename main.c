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
    // Load graph and query from file
    Graph* graph = loadGraphFromFile("input.txt", &src, &dst);
    if (graph == NULL) return 1;

    computePosition(graph);
    int* parent = (int*)malloc(graph->numVertices * sizeof(int));
    Path shortestPath = { .active = false, .count = 0 };

    // Calculate shortest path if nodes are valid
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

    // Initialize the Entity according to the struct
    Entity movingEntity = { 0 };
    if (shortestPath.active && shortestPath.count > 0) {
        movingEntity.currentPos = graph->positions[shortestPath.nodes[0]];
        movingEntity.currentPathIndex = 0;
        movingEntity.isMoving = false;
        movingEntity.isWaiting = true; // Wait at the first node
    }

    InitWindow(800, 600, "SO Project - Final Milestone 3");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // --- Input Handling ---
        Vector2 mousePoint = GetMousePosition();

        // Handle Play/Replay button
        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Reset simulation if it has already finished
            if (simulationFinished) {
                movingEntity.currentPathIndex = 0;
                movingEntity.currentStep = 0;
                movingEntity.frameCounter = 0;
                movingEntity.isWaiting = true;
                movingEntity.currentPos = graph->positions[shortestPath.nodes[0]];
                simulationFinished = false;
            }
            movingEntity.isMoving = true;
            isRunning = true;
        }

        // Handle Stop button
        if (CheckCollisionPointRec(mousePoint, stopBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            movingEntity.isMoving = false;
            isRunning = false;
        }

        // --- Logic Update ---
        if (isRunning && !simulationFinished) {
            // Calculate rotation based on the current direction of movement
            if (movingEntity.currentPathIndex < shortestPath.count - 1) {
                Vector2 current = graph->positions[shortestPath.nodes[movingEntity.currentPathIndex]];
                Vector2 next = graph->positions[shortestPath.nodes[movingEntity.currentPathIndex + 1]];
                carRotation = atan2f(next.y - current.y, next.x - current.x) * (180.0f / PI);
            }

            // Update entity movement using the time-based or frame-based logic
            updateEntity(&movingEntity, graph, &shortestPath);

            // Mark simulation as finished when destination is reached
            if (!movingEntity.isMoving && movingEntity.currentPathIndex >= shortestPath.count - 1) {
                simulationFinished = true;
                isRunning = false;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw graph nodes and edges
        drawGraph(graph, shortestPath);

        // Draw the vehicle if path is active
        if (shortestPath.active) {
            DrawCar(movingEntity.currentPos, carRotation, MAROON);
        }

        // GUI Buttons Rendering
        DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
        DrawRectangleLinesEx(playBtn, 2, DARKGREEN);
        const char* playText = simulationFinished ? "REPLAY" : "PLAY";
        DrawText(playText, playBtn.x + (simulationFinished ? 20 : 35), playBtn.y + 10, 20, BLACK);

        DrawRectangleRec(stopBtn, RED);
        DrawRectangleLinesEx(stopBtn, 2, MAROON);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        // Display completion message
        if (simulationFinished) {
            DrawRectangle(200, 250, 400, 100, Fade(GOLD, 0.9f));
            DrawText("SIMULATION FINISHED!", 225, 285, 25, BLACK);
        }

        EndDrawing();
    }

    // Cleanup
    free(parent);
    freeGraph(graph);
    CloseWindow();
    return 0;
}