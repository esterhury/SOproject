#include <signal.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// Global array for real OS child processes mapping
pid_t global_pids[MAX_PASSENGERS] = {0};

// Function to handle child process spawning via fork() (Milestone 4 OS Requirement)
void createTravelerProcesses(int numTravelers, pid_t* pids) {
    for (int i = 0; i < numTravelers; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Error: Fork failed");
            exit(1);
        }

        if (pids[i] == 0) {
            // Child process execution path
            printf("[%d] started\n", getpid());
            while (1) {
                sleep(1);
            }
            exit(0);
        }
    }
}

// Function to draw a car with rotation using standard C math capabilities (No raymath.h dependency)
void DrawCar(Vector2 position, float rotation, Color color) {
    float width = 40.0f;
    float height = 20.0f;

    // Standard core drawing using pure velocity angle rotation
    Rectangle rec = { position.x, position.y, width, height };
    Vector2 origin = { width / 2, height / 2 };
    DrawRectanglePro(rec, origin, rotation, color);

    // --- FIXED: MOVED WINDOW TO THE OPPOSITE SIDE TO FIX REVERSE DRIVING ---
    // Instead of adding angles, we physically moved the front window to the leading edge (+width/4)
    // and updated the window origin offset so it always faces FORWARD along the velocity vector.
    Rectangle window = { position.x, position.y, width / 4, height - 6 };
    Vector2 windowOrigin = { width / 4, (height - 6) / 2 };
    DrawRectanglePro(window, windowOrigin, rotation, SKYBLUE);

    float angleRad = rotation * (PI / 180.0f);
    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);

    Vector2 wheelOffsets[4] = {
        {-15.0f, -10.0f},
        {15.0f, -10.0f},
        {-15.0f, 10.0f},
        {15.0f, 10.0f}
    };

    for (int i = 0; i < 4; i++) {
        float rotatedX = wheelOffsets[i].x * cosA - wheelOffsets[i].y * sinA;
        float rotatedY = wheelOffsets[i].x * sinA + wheelOffsets[i].y * cosA;
        Vector2 finalWheelPos = { position.x + rotatedX, position.y + rotatedY };
        DrawCircleV(finalWheelPos, 4, BLACK);
    }
}

int main() {
    int* sourcesArray = NULL;
    int* destsArray = NULL;
    int numTravelers = 0;

    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &sourcesArray, &destsArray, &numTravelers);
    if (graph == NULL) {
        printf("Error: Graph context could not be loaded safely.\n");
        return 1;
    }

    computePosition(graph);

    // Secure array allocation with safe zero-initialization
    Passenger passengers[MAX_PASSENGERS];
    for (int i = 0; i < MAX_PASSENGERS; i++) {
        passengers[i].simulationFinished = false;
        passengers[i].shortestPath.active = false;
        passengers[i].movingEntity.isMoving = false;
        passengers[i].carRotation = 0.0f;
    }

    // Task 2: Synchronize passengers directly with the file data constraints
    int total_passengers = 3;
    int s0 = (numTravelers > 0) ? sourcesArray[0] : 0;
    int d0 = (numTravelers > 0) ? destsArray[0] : 4;
    int s1 = (numTravelers > 1) ? sourcesArray[1] : 2;
    int d1 = (numTravelers > 1) ? destsArray[1] : 3;
    int s2 = (numTravelers > 2) ? sourcesArray[2] : 1;
    int d2 = (numTravelers > 2) ? destsArray[2] : 4;

    calculatePassengerRoute(graph, &passengers[0], s0, d0);
    calculatePassengerRoute(graph, &passengers[1], s1, d1);
    calculatePassengerRoute(graph, &passengers[2], s2, d2);

    // Ensure all cars are stationary until PLAY is pressed
    for (int i = 0; i < total_passengers; i++) {
        passengers[i].movingEntity.isMoving = false;
    }

    // Fork concurrent OS tasks
    createTravelerProcesses(total_passengers, global_pids);
    for (int i = 0; i < total_passengers; i++) {
        passengers[i].id = global_pids[i];
    }

    bool isRunning = false;
    Rectangle playBtn = { 650, 20, 120, 40 };
    Rectangle stopBtn = { 650, 70, 120, 40 };

    InitWindow(800, 600, "Multi-Agent Graph Simulation - Fixed Windows");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mousePoint = GetMousePosition();

        bool allFinished = true;
        for (int i = 0; i < total_passengers; i++) {
            if (!passengers[i].simulationFinished) {
                allFinished = false;
                break;
            }
        }

        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (allFinished) {
                printf("[PARENT] Restarting simulation. Re-spawning child processes...\n");

                for (int i = 0; i < total_passengers; i++) {
                    waitpid(global_pids[i], NULL, WNOHANG);
                }

                createTravelerProcesses(total_passengers, global_pids);

                for (int i = 0; i < total_passengers; i++) {
                    passengers[i].id = global_pids[i];
                    passengers[i].simulationFinished = false;

                    calculatePassengerRoute(graph, &passengers[i],
                                            i == 0 ? s0 : i == 1 ? s1 : s2,
                                            i == 0 ? d0 : i == 1 ? d1 : d2);
                    passengers[i].movingEntity.isMoving = true;
                }
                allFinished = false;
            } else {
                for (int i = 0; i < total_passengers; i++) {
                    if (!passengers[i].simulationFinished) {
                        passengers[i].movingEntity.isMoving = true;
                    }
                }
            }
            isRunning = true;
        }

        if (CheckCollisionPointRec(mousePoint, stopBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = false;
            for (int i = 0; i < total_passengers; i++) {
                passengers[i].movingEntity.isMoving = false;
            }
        }

        // --- OS Multi-Agent Tracking & Signals ---
        if (isRunning) {
            for (int i = 0; i < total_passengers; i++) {
                if (!passengers[i].simulationFinished && passengers[i].shortestPath.active) {
                    int idx = passengers[i].movingEntity.currentPathIndex;

                    if (!passengers[i].movingEntity.isMoving && idx >= passengers[i].shortestPath.count - 1) {
                        passengers[i].simulationFinished = true;
                        kill(passengers[i].id, SIGKILL);
                        printf("[PARENT] Agent PID %d arrived safely. Process reaped via SIGKILL.\n", passengers[i].id);
                    }
                }
            }
        }

        // Advance positions and calculate rotations internally inside graph.c
        updateAllPassengers(graph, passengers, total_passengers, isRunning);

        // Graphics Rendering Stage
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Render base graph lines
        Path emptyPath = { .active = false };
        drawGraph(graph, emptyPath);

        // Overlay the customized colored shortest paths for each traveler
        for (int i = 0; i < total_passengers; i++) {
            if (passengers[i].shortestPath.active && !passengers[i].simulationFinished) {
                Color pathColor = (i == 0) ? MAROON : (i == 1) ? DARKBLUE : PURPLE;

                // Highlight the edges for this passenger's specific Dijkstra layout
                for (int k = 0; k < passengers[i].shortestPath.count - 1; k++) {
                    int u = passengers[i].shortestPath.nodes[k];
                    int v = passengers[i].shortestPath.nodes[k+1];
                    DrawLineEx(graph->positions[u], graph->positions[v], 3.5f, Fade(pathColor, 0.4f));
                }
            }
        }

        // Render all active traveling vehicle entities on top of the paths
        for (int i = 0; i < total_passengers; i++) {
            if (passengers[i].shortestPath.active && !passengers[i].simulationFinished) {
                Color carColor = (i == 0) ? MAROON : (i == 1) ? DARKBLUE : PURPLE;
                DrawCar(passengers[i].movingEntity.currentPos, passengers[i].carRotation, carColor);

                DrawText(TextFormat("PID: %d", passengers[i].id),
                         passengers[i].movingEntity.currentPos.x - 20,
                         passengers[i].movingEntity.currentPos.y - 25, 12, DARKGRAY);
            }
        }

        DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
        DrawText(allFinished ? "RESTART" : "PLAY", playBtn.x + (allFinished ? 15 : 35), playBtn.y + 10, 20, BLACK);

        DrawRectangleRec(stopBtn, RED);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        DrawText("SYSTEM: MULTI-AGENT TRACKING ACTIVE", 15, 15, 20, DARKBLUE);
        if (allFinished) {
            DrawText("STATUS: ALL AGENTS ARRIVED (PRESS RESTART)", 15, 40, 18, GOLD);
        } else {
            DrawText(isRunning ? "STATUS: RUNNING" : "STATUS: PAUSED", 15, 40, 18, isRunning ? LIME : DARKGRAY);
        }

        EndDrawing();
    }

    // Reaping operations
    CloseWindow();
    for (int i = 0; i < total_passengers; i++) {
        if (!passengers[i].simulationFinished) {
            kill(global_pids[i], SIGKILL);
        }
        waitpid(global_pids[i], NULL, 0);
    }

    if (sourcesArray) free(sourcesArray);
    if (destsArray) free(destsArray);
    freeGraph(graph);

    printf("[PARENT] All child layers cleanly closed. Execution finished.\n");
    return 0;
}