#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>
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
}

int main() {
    // Pointer arrays and counter to hold dynamic traveler details from file
    int* sourcesArray = NULL;
    int* destsArray = NULL;
    int numTravelers = 0;

    // Load graph structure and dynamic travelers arrays from input file
    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &sourcesArray, &destsArray, &numTravelers);
    if (graph == NULL) return 1;

    computePosition(graph);
    int* parent = (int*)malloc(graph->numVertices * sizeof(int));
    Path shortestPath = { .active = false, .count = 0 };

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