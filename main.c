#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "graph.h"
#include <math.h>

// Function to draw a car that rotates towards its target (Member 3 Visuals)
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

    // Small wheels
    DrawCircleV(Vector2Add(position, Vector2Rotate((Vector2){-15, -10}, rotation * DEG2RAD)), 4, BLACK);
    DrawCircleV(Vector2Add(position, Vector2Rotate((Vector2){15, -10}, rotation * DEG2RAD)), 4, BLACK);
    DrawCircleV(Vector2Add(position, Vector2Rotate((Vector2){-15, 10}, rotation * DEG2RAD)), 4, BLACK);
    DrawCircleV(Vector2Add(position, Vector2Rotate((Vector2){15, 10}, rotation * DEG2RAD)), 4, BLACK);
}

int main() {
    int src, dst;
    // Load graph and query - ensure the path to input.txt is correct
    Graph* graph = loadGraphFromFile("/home/student/CLionProjects/SOproject/input.txt", &src, &dst);
    if (graph == NULL) return 1;

    computePosition(graph);
    int* parent = (int*)malloc(graph->numVertices * sizeof(int));
    Path shortestPath = { .active = false, .count = 0 };

    if (src != -1 && dst != -1) {
        dijkstra(graph, src, dst, parent);
        shortestPath = reconstructPath(parent, src, dst);
    }

    // --- Member 3: Animation and UI Variables ---
    bool isRunning = false;
    bool simulationFinished = false;
    float carRotation = 0.0f;

    Entity movingEntity = { 0 };
    if (shortestPath.active && shortestPath.count > 0) {
        movingEntity.currentPos = graph->positions[shortestPath.nodes[0]];
        movingEntity.currentPathIndex = 0;
    }

    Rectangle playBtn = { 650, 20, 120, 40 };
    Rectangle stopBtn = { 650, 70, 120, 40 };

    InitWindow(800, 600, "SO Project - Car Simulation Milestone 3");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // --- Member 3: UI Interaction ---
        Vector2 mousePoint = GetMousePosition();
        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = true;
            if (simulationFinished) { // Reset if already finished
                movingEntity.currentPos = graph->positions[shortestPath.nodes[0]];
                movingEntity.currentPathIndex = 0;
                simulationFinished = false;
            }
        }
        if (CheckCollisionPointRec(mousePoint, stopBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = false;
        }

        // --- Member 3: Car Movement & Rotation Logic ---
        if (isRunning && shortestPath.active && !simulationFinished) {
            if (movingEntity.currentPathIndex < shortestPath.count - 1) {
                int currNode = shortestPath.nodes[movingEntity.currentPathIndex];
                int nextNode = shortestPath.nodes[movingEntity.currentPathIndex + 1];

                // Get Weight from adjacency list to adjust speed
                float weight = 1.0f;
                Node* temp = graph->adjLists[currNode];
                while (temp) {
                    if (temp->dest == nextNode) { weight = (float)temp->weight; break; }
                    temp = temp->next;
                }

                // Speed is inversely proportional to weight
                float baseSpeed = 5.0f;
                float dynamicSpeed = baseSpeed / weight;

                Vector2 targetPos = graph->positions[nextNode];
                float dx = targetPos.x - movingEntity.currentPos.x;
                float dy = targetPos.y - movingEntity.currentPos.y;
                float distance = sqrtf(dx*dx + dy*dy);

                // Calculate rotation angle in degrees
                carRotation = atan2f(dy, dx) * (180.0f / PI);

                if (distance > dynamicSpeed) {
                    movingEntity.currentPos.x += (dx / distance) * dynamicSpeed;
                    movingEntity.currentPos.y += (dy / distance) * dynamicSpeed;
                } else {
                    // Snap to node and move to next segment
                    movingEntity.currentPos = targetPos;
                    movingEntity.currentPathIndex++;
                }
            } else {
                simulationFinished = true;
                isRunning = false;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Base graph rendering (Members 1 & 2)
        drawGraph(graph, shortestPath);

        // --- Member 3: Car and UI Rendering ---
        if (shortestPath.active) {
            DrawCar(movingEntity.currentPos, carRotation, MAROON);
        }

        // GUI Buttons
        DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
        DrawText("PLAY", playBtn.x + 35, playBtn.y + 10, 20, BLACK);
        DrawRectangleRec(stopBtn, RED);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        // Status Overlay
        if (src != -1 && dst != -1) {
            DrawText(TextFormat("Query: %d to %d", src, dst), 15, 15, 20, DARKBLUE);
            DrawText(isRunning ? "STATUS: RUNNING" : "STATUS: PAUSED", 15, 40, 18, isRunning ? LIME : DARKGRAY);
        }

        if (simulationFinished) {
            DrawRectangle(200, 250, 400, 100, Fade(GOLD, 0.9f));
            DrawRectangleLines(200, 250, 400, 100, BLACK);
            DrawText("SIMULATION FINISHED!", 230, 285, 25, BLACK);
        }

        EndDrawing();
    }

    free(parent);
    freeGraph(graph);
    CloseWindow();
    return 0;
}