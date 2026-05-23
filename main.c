#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>
#include "raymath.h"

// Function to draw a car that rotates towards its target position
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Function to handle child process spawning and lifetime management
void createTravelerProcesses(int numTravelers, pid_t* pids) {
    for (int i = 0; i < numTravelers; i++) {
        pids[i] = fork(); // Spawn a dedicated child process for each traveler via fork()
        if (pids[i] < 0) {
            perror("Error: Fork failed");
            exit(1);
        }

        if (pids[i] == 0) {
            // Child process execution path
            printf("[%d] started\n", getpid()); // Print starting message with unique PID immediately

            // Children sleep and remain stationary during Milestone 4
            while (1) {
                sleep(1);
            }
            exit(0);
        }
    }
}

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
}

int main() {
    // Pointer arrays and counter to hold dynamic traveler details from file
    int* sourcesArray = NULL;
    int* destsArray = NULL;
    int numTravelers = 0;

    // Load graph structure and dynamic travelers arrays from input file
    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &sourcesArray, &destsArray, &numTravelers);
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
    // Temporary backup to keep existing single-car logic active without breaking dijkstra
    int src = (numTravelers > 0) ? sourcesArray[0] : -1;
    int dst = (numTravelers > 0) ? destsArray[0] : -1;

    // Allocate dynamic memory on the Heap to store child process IDs
    pid_t* pids = (pid_t*)malloc(sizeof(pid_t) * numTravelers);
    if (pids == NULL) {
        printf("Error: Memory allocation failed for PIDs.\n");
        freeGraph(graph);
        free(sourcesArray);
        free(destsArray);
        free(parent);
        return 1;
    }

    // Call helper function to fork traveler processes and print their PIDs
    createTravelerProcesses(numTravelers, pids);

    // Calculate shortest path if nodes are valid
    if (src != -1 && dst != -1) {
        dijkstra(graph, src, dst, parent);
        shortestPath = reconstructPath(parent, src, dst);
    }

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

    // Main real-time execution loop
    while (!WindowShouldClose()) {
        // UI Navigation and Button Interaction logic
        // --- Input Handling ---
        Vector2 mousePoint = GetMousePosition();

        // Handle Play/Replay button
        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = true;

            // Automatically reset and recalculate routing states if simulation was completed
            for (int i = 0; i < total_passengers; i++) {
                if (passengers[i].simulationFinished) {
                    calculatePassengerRoute(graph, &passengers[i],
                                            i == 0 ? src : i == 1 ? 0 : 2,
                                            i == 0 ? dst : i == 1 ? 4 : 6);
                }
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

        // Advance positions simultaneously for all active tracking agents based on runtime state
        updateAllPassengers(graph, passengers, total_passengers, isRunning);
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

        // System state text overlays
        if (src != -1 && dst != -1) {
            DrawText("SYSTEM: MULTI-AGENT TRACKING ACTIVE", 15, 15, 20, DARKBLUE);
            DrawText(isRunning ? "STATUS: RUNNING" : "STATUS: PAUSED", 15, 40, 18, isRunning ? LIME : DARKGRAY);
        }

        // Display completion panel when all tracking activities are finished
        if (allFinished && isRunning) {
            isRunning = false;
        // Display completion message
        if (simulationFinished) {
            DrawRectangle(200, 250, 400, 100, Fade(GOLD, 0.9f));
            DrawText("SIMULATION FINISHED!", 225, 285, 25, BLACK);
        }

        EndDrawing();
    }

    // Release dynamically allocated resources before system shutdown
    // Cleanup
    free(parent);
    freeGraph(graph);
    CloseWindow();

    // Loop through all child PIDs and reap them to prevent zombie processes
    for (int i = 0; i < numTravelers; i++) {
        waitpid(pids[i], NULL, 0); // Wait for each child process to finish completely
    }

    // Free all dynamically allocated travelers buffers to avoid leaks
    free(sourcesArray);
    free(destsArray);
    free(pids);
    return 0;
}