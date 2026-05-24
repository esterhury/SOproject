#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // Required for sending system signals (kill)

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
            exit(0); // Safeguard (execution should not reach here unless signaled)
        }
    }
}

// Function to draw a car with rotation using standard C math capabilities
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

    // Convert rotation from degrees to radians for standard cosf/sinf functions
    float angleRad = rotation * (PI / 180.0f);
    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);

    // Hardcoded relative offsets for the 4 wheels from the center of the car
    Vector2 wheelOffsets[4] = {
        {-15.0f, -10.0f},
        {15.0f, -10.0f},
        {-15.0f, 10.0f},
        {15.0f, 10.0f}
    };

    // Calculate and draw each wheel position using manual 2D rotation matrix formulas
    for (int i = 0; i < 4; i++) {
        // Standard 2D Rotation transformation formulas:
        // X' = X * cos(A) - Y * sin(A)
        // Y' = X * sin(A) + Y * cos(A)
        float rotatedX = wheelOffsets[i].x * cosA - wheelOffsets[i].y * sinA;
        float rotatedY = wheelOffsets[i].x * sinA + wheelOffsets[i].y * cosA;

        // Add the rotated local offset to the absolute screen position of the car
        Vector2 finalWheelPos = { position.x + rotatedX, position.y + rotatedY };
        DrawCircleV(finalWheelPos, 4, BLACK);
    }
}

// Unified main function representing the master parent process lifecycle
int main() {
    // Dynamic memory buffers to store traveler queries extracted from file
    int* sourcesArray = NULL;
    int* destsArray = NULL;
    int numTravelers = 0;

    // Load graph structural data and the dynamic travelers arrays from the configuration file
    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &sourcesArray, &destsArray, &numTravelers);
    if (graph == NULL || numTravelers == 0) {
        printf("Error: Failed to load graph or travelers.\n");
        return 1;
    }

    // Distribute node positions visually across the screen coordinates
    computePosition(graph);

    // Color palette configuration for distinct visual isolation of concurrent agents
    Color travelerColors[] = { MAROON, DARKBLUE, PURPLE, GREEN, ORANGE, GOLD, LIME };
    int numAvailableColors = 7;

    // Dynamically allocate tracking arrays based on the total travelers count found in the file
    Passenger* passengers = (Passenger*)malloc(sizeof(Passenger) * numTravelers);
    pid_t* pids = (pid_t*)malloc(sizeof(pid_t) * numTravelers);

    if (passengers == NULL || pids == NULL) {
        printf("Error: Memory allocation failed.\n");
        return 1;
    }

    // Initialize state profiles and compute shortest paths using Dijkstra's algorithm inside the parent
    for (int i = 0; i < numTravelers; i++) {
        passengers[i].carRotation = 0.0f;

        // Execute routing calculations for each distinct passenger segment
        int parent[100]; // Temporary backtracking layout buffer for Dijkstra
        dijkstra(graph, sourcesArray[i], destsArray[i], parent);
        passengers[i].shortestPath = reconstructPath(parent, sourcesArray[i], destsArray[i]);

        // Setup rendering runtime metrics inside the underlying moving entity structure
        if (passengers[i].shortestPath.active && passengers[i].shortestPath.count > 0) {
            passengers[i].movingEntity.currentPos = graph->positions[passengers[i].shortestPath.nodes[0]];
            passengers[i].movingEntity.currentPathIndex = 0;
            passengers[i].movingEntity.currentStep = 0;
            passengers[i].movingEntity.frameCounter = 0;
            passengers[i].movingEntity.isMoving = false;
            passengers[i].movingEntity.isWaiting = true; // Wait stage triggers at the origin node
        }
        passengers[i].simulationFinished = false;
    }

    // Call helper function to fork traveler processes concurrently for the FIRST run
    createTravelerProcesses(numTravelers, pids);

    // Bind the actual system-assigned process identifiers (PIDs) to the corresponding passenger structs
    for (int i = 0; i < numTravelers; i++) {
        passengers[i].id = pids[i];
    }

    // Initialize Raylib runtime display context
    InitWindow(800, 600, "SO Project - Milestone 4 Multi-Agent Simulation");
    SetTargetFPS(60);

    bool isRunning = false;
    Rectangle playBtn = { 650, 20, 120, 40 };
    Rectangle stopBtn = { 650, 70, 120, 40 };

    // Main real-time graphics and simulation loop
    while (!WindowShouldClose()) {
        // --- 1. Input Handling & UI Button Interaction Logic ---
        Vector2 mousePoint = GetMousePosition();

        // Check if all concurrent tracking tasks are finished before handling input
        bool allFinished = true;
        for (int i = 0; i < numTravelers; i++) {
            if (!passengers[i].simulationFinished) {
                allFinished = false;
                break;
            }
        }

        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // If all agents had already arrived, reset everything for a fresh restart
            if (allFinished) {
                printf("[PARENT] Restarting simulation. Re-spawning child processes...\n");

                // Reap old dead processes just in case
                for (int i = 0; i < numTravelers; i++) {
                    waitpid(pids[i], NULL, WNOHANG);
                }

                // Fork brand new child processes for the new run
                createTravelerProcesses(numTravelers, pids);

                // Reset positions, path counters and state flags for all travelers
                for (int i = 0; i < numTravelers; i++) {
                    passengers[i].id = pids[i]; // Bind new PIDs
                    passengers[i].simulationFinished = false;
                    passengers[i].carRotation = 0.0f;

                    if (passengers[i].shortestPath.active && passengers[i].shortestPath.count > 0) {
                        passengers[i].movingEntity.currentPos = graph->positions[passengers[i].shortestPath.nodes[0]];
                        passengers[i].movingEntity.currentPathIndex = 0;
                        passengers[i].movingEntity.currentStep = 0;
                        passengers[i].movingEntity.frameCounter = 0;
                        passengers[i].movingEntity.isMoving = true; // Start moving immediately on restart
                        passengers[i].movingEntity.isWaiting = true;
                    }
                }
                allFinished = false;
            } else {
                // Regular Resume functionality if the simulation was just paused
                for (int i = 0; i < numTravelers; i++) {
                    if (!passengers[i].simulationFinished) {
                        passengers[i].movingEntity.isMoving = true;
                    }
                }
            }
            isRunning = true;
        }

        if (CheckCollisionPointRec(mousePoint, stopBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = false;
            for (int i = 0; i < numTravelers; i++) {
                passengers[i].movingEntity.isMoving = false;
            }
        }

        // --- 2. Concurrent Logic Update ---
        if (isRunning) {
            for (int i = 0; i < numTravelers; i++) {
                if (!passengers[i].simulationFinished && passengers[i].shortestPath.active) {

                    int idx = passengers[i].movingEntity.currentPathIndex;

                    // Calculate rotation based on the vector from the current animated position to the next structural node position
                    if (idx < passengers[i].shortestPath.count - 1) {
                        int nextNode = passengers[i].shortestPath.nodes[idx + 1];
                        Vector2 nextNodePos = graph->positions[nextNode];
                        Vector2 currentPos = passengers[i].movingEntity.currentPos;

                        float angle = atan2f(nextNodePos.y - currentPos.y, nextNodePos.x - currentPos.x) * (180.0f / PI);
                        passengers[i].carRotation = angle;
                    }

                    // Advance position coordinates simultaneously for the active tracking entity
                    updateEntity(&passengers[i].movingEntity, graph, &passengers[i].shortestPath);

                    // Check if the individual agent reached its destination node
                    if (!passengers[i].movingEntity.isMoving && idx >= passengers[i].shortestPath.count - 1) {
                        passengers[i].simulationFinished = true;

                        // Parent sends kill signal to terminate child upon arrival
                        kill(passengers[i].id, SIGKILL);
                        printf("[PARENT] Traveler PID %d reached destination. Signal sent.\n", passengers[i].id);
                    }
                }
            }
        }

        // Re-evaluate if all finished to accurately draw the overlay state
        allFinished = true;
        for (int i = 0; i < numTravelers; i++) {
            if (!passengers[i].simulationFinished) {
                allFinished = false;
                break;
            }
        }
        if (allFinished) {
            isRunning = false; // Turn off running state when everything finishes
        }

        // --- 3. Graphics Rendering Stage ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Render the base graph network paths on screen
        Path emptyPath = { .active = false };
        drawGraph(graph, emptyPath);

        // Render all active traveling entities concurrently using unique color layouts
        for (int i = 0; i < numTravelers; i++) {
            if (passengers[i].shortestPath.active && !passengers[i].simulationFinished) {
                Color carColor = travelerColors[i % numAvailableColors]; // Map distinct color template
                DrawCar(passengers[i].movingEntity.currentPos, passengers[i].carRotation, carColor);

                // Display the real OS process identification context next to the moving vehicle
                DrawText(TextFormat("PID: %d", passengers[i].id),
                         passengers[i].movingEntity.currentPos.x - 20,
                         passengers[i].movingEntity.currentPos.y - 25, 12, DARKGRAY);
            }
        }

        // Draw Interactive UI Buttons
        DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
        DrawText(allFinished ? "RESTART" : "PLAY", playBtn.x + (allFinished ? 15 : 35), playBtn.y + 10, 20, BLACK);

        DrawRectangleRec(stopBtn, RED);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        // System state text overlays
        DrawText("SYSTEM: MULTI-AGENT OS SIMULATION ACTIVE", 15, 15, 20, DARKBLUE);
        if (allFinished) {
            DrawText("STATUS: ALL AGENTS ARRIVED (PRESS RESTART)", 15, 40, 18, GOLD);
        } else {
            DrawText(isRunning ? "STATUS: RUNNING" : "STATUS: PAUSED", 15, 40, 18, isRunning ? LIME : DARKGRAY);
        }

        EndDrawing();
    } // <--- לולאת ה-while הראשית של Raylib נסגרת כאן בצורה תקינה!

    // --- 4. Cleanup & Resource Deallocation Stage ---
    CloseWindow();

    // Loop through all child PIDs and reap them to prevent zombie processes
    for (int i = 0; i < numTravelers; i++) {
        if (!passengers[i].simulationFinished) {
            kill(pids[i], SIGKILL);
        }
        waitpid(pids[i], NULL, 0); // Reaping operation
    }

    // Free all dynamically allocated heap memory buffers to prevent memory leaks
    free(sourcesArray);
    free(destsArray);
    free(pids);
    free(passengers);
    freeGraph(graph);

    printf("[PARENT] All children reaped. Exiting cleanly.\n");
    return 0;
}