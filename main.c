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
    // Arrays required by Member 1's updated configuration
    int* sourcesArray = NULL;
    int* destsArray = NULL;
    int numTravelers = 0;

    // Load graph structural data using Member 1's updated system interface
    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &sourcesArray, &destsArray, &numTravelers);
    if (graph == NULL) return 1;

    // Distribute node positions visually across the screen coordinates
    computePosition(graph);

    // Array to manage and track multiple concurrent passengers
    Passenger passengers[MAX_PASSENGERS];

    // Fallback constants to guarantee cars appear even if file array is empty
    int default_src = 0;
    int default_dst = 2;

    // If Member 1 successfully read data from the file, extract the first query
    if (numTravelers > 0 && sourcesArray != NULL && destsArray != NULL) {
        default_src = sourcesArray[0];
        default_dst = destsArray[0];
    }

    // Task 2: Maintain multiple car configurations exactly as practiced before
    int total_passengers = 3;

    // Passenger 0 uses the active file query boundaries
    passengers[0].id = 1001;
    calculatePassengerRoute(graph, &passengers[0], default_src, default_dst);

    // Passenger 1 operates on a secondary concurrent tracking loop
    passengers[1].id = 1002;
    calculatePassengerRoute(graph, &passengers[1], 1, 4);

    // Passenger 2 operates on a third concurrent tracking loop
    passengers[2].id = 1003;
    calculatePassengerRoute(graph, &passengers[2], 5, 1);

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
                                            i == 0 ? default_src : i == 1 ? 1 : 5,
                                            i == 0 ? default_dst : i == 1 ? 4 : 1);
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

        // Render base graph network paths on screen (using passenger 0's path for route tracking highlights)
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
        DrawText("SYSTEM: MULTI-AGENT TRACKING ACTIVE", 15, 15, 20, DARKBLUE);
        DrawText(isRunning ? "STATUS: RUNNING" : "STATUS: PAUSED", 15, 40, 18, isRunning ? LIME : DARKGRAY);

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
    if (sourcesArray) free(sourcesArray);
    if (destsArray) free(destsArray);
    freeGraph(graph);
    CloseWindow();
    return 0;
}