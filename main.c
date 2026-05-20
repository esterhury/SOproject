#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>
#include "raymath.h"

// Function to draw a car that rotates towards its target position
void DrawCar(Vector2 position, float rotation, Color color) {
    float width = 40.0f;
    float height = 20.0f;

    // Main body of the car
    Rectangle rec = { position.x, position.y, width, height };
    Vector2 origin = { width / 2, height / 2 };
    DrawRectanglePro(rec, origin, rotation, color);

    // Front window to show direction
    Rectangle window = { position.x, position.y, width / 4, height - 6 };
    Vector2 windowOrigin = { -width / 4, (height - 6) / 2 };
    DrawRectanglePro(window, windowOrigin, rotation, SKYBLUE);

    // Small wheels configuration
    DrawCircleV(Vector2Add(position, Vector2Rotate((Vector2){-15, -10}, rotation * DEG2RAD)), 4, BLACK);
    DrawCircleV(Vector2Add(position, Vector2Rotate((Vector2){15, -10}, rotation * DEG2RAD)), 4, BLACK);
    DrawCircleV(Vector2Add(position, Vector2Rotate((Vector2){-15, 10}, rotation * DEG2RAD)), 4, BLACK);
    DrawCircleV(Vector2Add(position, Vector2Rotate((Vector2){15, 10}, rotation * DEG2RAD)), 4, BLACK);
}

int main() {
    int src, dst;
    // Load graph structural data and the initial pathfinding query from the configuration file
    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &src, &dst);
    if (graph == NULL) return 1;

    // Distribute node positions visually across the screen coordinates
    computePosition(graph);

    // Array to manage and track multiple concurrent passengers within the father process
    Passenger passengers[MAX_PASSENGERS];
    int total_passengers = 3; // Set active simulation agent count

    // Calculate shortest paths using Dijkstra's algorithm for each concurrent agent
    // First passenger directly processes the source and destination read from the input file
    passengers[0].id = 1001;
    calculatePassengerRoute(graph, &passengers[0], src, dst);

    // Second passenger initialization with a distinct source and destination segment
    passengers[1].id = 1002;
    calculatePassengerRoute(graph, &passengers[1], 0, 4);

    // Third passenger initialization for parallel movement tracking
    passengers[2].id = 1003;
    calculatePassengerRoute(graph, &passengers[2], 2, 6);

    // Animation control and UI state tracking variables
    bool isRunning = false;
    Rectangle playBtn = { 650, 20, 120, 40 };
    Rectangle stopBtn = { 650, 70, 120, 40 };

    // Initialize Raylib layout window configurations
    InitWindow(800, 600, "Multi-Agent Graph Simulation");
    SetTargetFPS(60);

    // Main real-time execution loop
    while (!WindowShouldClose()) {
        // UI Navigation and Button Interaction logic
        Vector2 mousePoint = GetMousePosition();
        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = true;

            // Automatically reset and recalculate routing states if simulation was completed
            for (int i = 0; i < total_passengers; i++) {
                if (passengers[i].simulationFinished) {
                    calculatePassengerRoute(graph, &passengers[i],
                                            i == 0 ? src : i == 1 ? 0 : 2,
                                            i == 0 ? dst : i == 1 ? 4 : 6);
                }
            }
        }
        if (CheckCollisionPointRec(mousePoint, stopBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = false;
        }

        // Advance positions simultaneously for all active tracking agents based on runtime state
        updateAllPassengers(graph, passengers, total_passengers, isRunning);

        // Graphics Rendering Stage
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Render base graph network paths on screen
        drawGraph(graph, passengers[0].shortestPath);

        // Iterate through tracking array to render each active entity dynamically
        bool allFinished = true;
        for (int i = 0; i < total_passengers; i++) {
            if (passengers[i].shortestPath.active) {
                if (!passengers[i].simulationFinished) {
                    allFinished = false;

                    // Render cars using distinct color templates for visual isolation
                    Color carColor = (i == 0) ? MAROON : (i == 1) ? DARKBLUE : PURPLE;
                    DrawCar(passengers[i].movingEntity.currentPos, passengers[i].carRotation, carColor);

                    // Display specific process identification context next to the moving vehicle
                    DrawText(TextFormat("PID: %d", passengers[i].id),
                             passengers[i].movingEntity.currentPos.x - 20,
                             passengers[i].movingEntity.currentPos.y - 25, 12, DARKGRAY);
                }
            }
        }

        // Draw Interactive UI Buttons
        DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
        DrawText("PLAY", playBtn.x + 35, playBtn.y + 10, 20, BLACK);
        DrawRectangleRec(stopBtn, RED);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        // System state text overlays
        if (src != -1 && dst != -1) {
            DrawText("SYSTEM: MULTI-AGENT TRACKING ACTIVE", 15, 15, 20, DARKBLUE);
            DrawText(isRunning ? "STATUS: RUNNING" : "STATUS: PAUSED", 15, 40, 18, isRunning ? LIME : DARKGRAY);
        }

        // Display completion panel when all tracking activities are finished
        if (allFinished && isRunning) {
            isRunning = false;
            DrawRectangle(200, 250, 400, 100, Fade(GOLD, 0.9f));
            DrawRectangleLines(200, 250, 400, 100, BLACK);
            DrawText("SIMULATION FINISHED!", 230, 285, 25, BLACK);
        }

        EndDrawing();
    }

    // Release dynamically allocated resources before system shutdown
    freeGraph(graph);
    CloseWindow();
    return 0;
}