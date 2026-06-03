#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "raylib.h"
#include "graph.h"

typedef struct {
    int agentIndex;
    int currentNode;
    int nextNode;
    bool isWaiting;
    bool isFinished;
} IPCMessage;

pid_t global_pids[MAX_PASSENGERS] = {0};
int agent_pipes[MAX_PASSENGERS][2];
Passenger shared_passengers[MAX_PASSENGERS];
int total_passengers = 2; // Matched to 2 travelers from the professor's sample

Vector2 visual_targets[MAX_PASSENGERS];
float animation_speed = 0.04f;

void DrawCar(Vector2 position, float rotation, Color color, bool isWaiting) {
    float width = 40.0f;
    float height = 20.0f;
    Color colorToDraw = color;

    if (isWaiting) {
        if ((int)(GetTime() * 3) % 2 == 0) colorToDraw = ORANGE;
        else colorToDraw = YELLOW;
    }

    Rectangle rec = { position.x, position.y, width, height };
    Vector2 origin = { width / 2, height / 2 };
    DrawRectanglePro(rec, origin, rotation, colorToDraw);

    Rectangle window = { position.x, position.y, width / 4, height - 6 };
    Vector2 windowOrigin = { width / 4, (height - 6) / 2 };
    DrawRectanglePro(window, windowOrigin, rotation, SKYBLUE);

    float angleRad = rotation * (PI / 180.0f);
    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);

    Vector2 wheelOffsets[4] = {
        {-15.0f, -10.0f}, {15.0f, -10.0f}, {-15.0f, 10.0f}, {15.0f, 10.0f}
    };

    for (int i = 0; i < 4; i++) {
        float rotatedX = wheelOffsets[i].x * cosA - wheelOffsets[i].y * sinA;
        float rotatedY = wheelOffsets[i].x * sinA + wheelOffsets[i].y * cosA;
        Vector2 finalWheelPos = { position.x + rotatedX, position.y + rotatedY };
        DrawCircleV(finalWheelPos, 4, BLACK);
    }

    if (isWaiting) {
        int fontSize = 11;
        const char* waitText = "WAITING FOR JUNCTION";
        int textWidth = MeasureText(waitText, fontSize);
        DrawText(waitText, position.x - (textWidth / 2), position.y - 35, fontSize, ORANGE);
    }
}

void runChildAgentLogic(Graph* graph, int agentIndex, int src, int dst) {
    if (graph == NULL) exit(1);

    int* parentArr = (int*)malloc(graph->numVertices * sizeof(int));
    if (!parentArr) exit(1);

    dijkstra(graph, src, dst, parentArr);
    Path myPath = reconstructPath(parentArr, src, dst);
    free(parentArr);

    if (!myPath.active || myPath.count <= 0) exit(1);

    sem_wait(&(graph->semaphores[myPath.nodes[0]]));

    IPCMessage msg = {
        .agentIndex = agentIndex,
        .currentNode = myPath.nodes[0],
        .nextNode = (myPath.count > 1) ? myPath.nodes[1] : -1,
        .isWaiting = false,
        .isFinished = (myPath.count == 1)
    };

    // Captured return value to strictly satisfy compiler restrictions
    int unused_ret;
    unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

    for (int i = 0; i < myPath.count - 1; i++) {
        int curr = myPath.nodes[i];
        int next = myPath.nodes[i + 1];

        for (int step = 0; step < 40; step++) {
            usleep(25000);
        }

        msg.currentNode = curr;
        msg.nextNode = next;
        msg.isWaiting = true;
        unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

        sem_wait(&(graph->semaphores[next]));
        sem_post(&(graph->semaphores[curr]));

        msg.currentNode = next;
        msg.nextNode = (i + 2 < myPath.count) ? myPath.nodes[i + 2] : -1;
        msg.isWaiting = false;
        if (i + 1 == myPath.count - 1) msg.isFinished = true;
        unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));
    }

    // Explicitly use the variable to suppress any set-but-unused warning flags
    (void)unused_ret;

    while (1) {
        sleep(1);
    }
}

void createTravelerProcesses(Graph* graph, int numTravelers, int sources[], int dests[]) {
    for (int i = 0; i < numTravelers; i++) {
        if (pipe(agent_pipes[i]) < 0) {
            perror("Error: pipe failed");
            exit(1);
        }

        int flags = fcntl(agent_pipes[i][0], F_GETFL, 0);
        fcntl(agent_pipes[i][0], F_SETFL, flags | O_NONBLOCK);

        global_pids[i] = fork();
        if (global_pids[i] < 0) {
            perror("Error: Fork failed");
            exit(1);
        }

        if (global_pids[i] == 0) {
            close(agent_pipes[i][0]);
            runChildAgentLogic(graph, i, sources[i], dests[i]);
        } else {
            close(agent_pipes[i][1]);
        }
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

    // Dynamic scale adaptation based on file travelers count
    if (numTravelers > 0 && numTravelers <= MAX_PASSENGERS) {
        total_passengers = numTravelers;
    }

    for (int i = 0; i < MAX_PASSENGERS; i++) {
        shared_passengers[i].simulationFinished = false;
        shared_passengers[i].shortestPath.active = false;
        shared_passengers[i].movingEntity.isMoving = false;
        shared_passengers[i].movingEntity.isWaiting = false;
        shared_passengers[i].carRotation = 0.0f;
    }

    int sources[MAX_PASSENGERS];
    int dests[MAX_PASSENGERS];
    for (int i = 0; i < total_passengers; i++) {
        sources[i] = sourcesArray[i];
        dests[i] = destsArray[i];
    }

    for (int i = 0; i < total_passengers; i++) {
        shared_passengers[i].id = 1000 + i;
        shared_passengers[i].shortestPath.active = true;
        shared_passengers[i].movingEntity.currentPos = graph->positions[sources[i]];
        visual_targets[i] = graph->positions[sources[i]];
    }

    bool isRunning = false;
    Rectangle playBtn = { 650, 20, 120, 40 };
    Rectangle stopBtn = { 650, 70, 120, 40 };

    InitWindow(800, 600, "Multi-Agent Graph Simulation - Milestone 5 Strict Match");
    SetTargetFPS(60);

    // Track which processes have logged their finished states
    bool process_logged_finished[MAX_PASSENGERS] = {false};

    while (!WindowShouldClose()) {
        Vector2 mousePoint = GetMousePosition();

        bool allFinished = true;
        for (int i = 0; i < total_passengers; i++) {
            if (!shared_passengers[i].simulationFinished) {
                allFinished = false;
                break;
            }
        }

        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (!isRunning) {
                if (allFinished) {
                    for (int i = 0; i < total_passengers; i++) {
                        waitpid(global_pids[i], NULL, WNOHANG);
                        shared_passengers[i].simulationFinished = false;
                        shared_passengers[i].movingEntity.isWaiting = false;
                        shared_passengers[i].movingEntity.currentPos = graph->positions[sources[i]];
                        visual_targets[i] = graph->positions[sources[i]];
                        process_logged_finished[i] = false;
                    }
                    createTravelerProcesses(graph, total_passengers, sources, dests);
                } else if (global_pids[0] == 0) {
                    createTravelerProcesses(graph, total_passengers, sources, dests);
                }
                isRunning = true;
            }
        }

        if (CheckCollisionPointRec(mousePoint, stopBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = false;
        }

        // --- PARENT IPC INPUT TRACKING LOOP ---
        if (isRunning) {
            for (int i = 0; i < total_passengers; i++) {
                IPCMessage incomingMsg;
                ssize_t bytesRead = read(agent_pipes[i][0], &incomingMsg, sizeof(IPCMessage));

                if (bytesRead == sizeof(IPCMessage)) {
                    shared_passengers[i].id = global_pids[i];
                    shared_passengers[i].movingEntity.isWaiting = incomingMsg.isWaiting;

                    Vector2 currentJunctionPos = graph->positions[incomingMsg.currentNode];

                    if (incomingMsg.isWaiting) {
                        Vector2 targetJunctionPos = graph->positions[incomingMsg.nextNode];
                        float dx = targetJunctionPos.x - currentJunctionPos.x;
                        float dy = targetJunctionPos.y - currentJunctionPos.y;
                        float len = sqrtf(dx*dx + dy*dy);
                        if (len > 0) {
                            visual_targets[i].x = targetJunctionPos.x - (dx / len) * 35.0f;
                            visual_targets[i].y = targetJunctionPos.y - (dy / len) * 35.0f;
                        }
                    } else {
                        visual_targets[i] = currentJunctionPos;

                        if (incomingMsg.nextNode != -1) {
                            printf("[PID=%d] arrived at node %d | next node: %d\n", global_pids[i], incomingMsg.currentNode, incomingMsg.nextNode);
                        } else {
                            printf("[PID=%d] arrived at node %d | DESTINATION\n", global_pids[i], incomingMsg.currentNode);
                            shared_passengers[i].simulationFinished = true; // Safe arrival checkpoint mapping
                        }
                        fflush(stdout);
                    }
                }

                float dx = visual_targets[i].x - shared_passengers[i].movingEntity.currentPos.x;
                float dy = visual_targets[i].y - shared_passengers[i].movingEntity.currentPos.y;
                if (sqrtf(dx*dx + dy*dy) > 1.0f) {
                    shared_passengers[i].movingEntity.currentPos.x += dx * animation_speed;
                    shared_passengers[i].movingEntity.currentPos.y += dy * animation_speed;
                    shared_passengers[i].carRotation = atan2f(dy, dx) * (180.0f / PI);
                }
            }

            // --- STRICT REAPING SEQUENCE ORDERING ---
            // Prints the exact cascading 'finished' messages only after arrival criteria are met
            if (allFinished) {
                for (int i = 0; i < total_passengers; i++) {
                    if (!process_logged_finished[i]) {
                        printf("[PID=%d] finished\n", global_pids[i]);
                        fflush(stdout);
                        kill(global_pids[i], SIGKILL);
                        process_logged_finished[i] = true;
                    }
                }
            }
        }

        // Graphics Rendering Stage
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Path emptyPath = { .active = false };
        drawGraph(graph, emptyPath);

        for (int i = 0; i < total_passengers; i++) {
            if (!process_logged_finished[i]) {
                Color carColor = (i == 0) ? MAROON : (i == 1) ? DARKBLUE : PURPLE;
                DrawCar(shared_passengers[i].movingEntity.currentPos, shared_passengers[i].carRotation, carColor, shared_passengers[i].movingEntity.isWaiting);

                int pidYOffset = shared_passengers[i].movingEntity.isWaiting ? -47 : -25;
                DrawText(TextFormat("PID: %d", shared_passengers[i].id),
                         shared_passengers[i].movingEntity.currentPos.x - 20,
                         shared_passengers[i].movingEntity.currentPos.y + pidYOffset, 12, DARKGRAY);
            }
        }

        DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
        DrawText(allFinished ? "RESTART" : "PLAY", playBtn.x + (allFinished ? 15 : 35), playBtn.y + 10, 20, BLACK);
        DrawRectangleRec(stopBtn, RED);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        DrawText("SYSTEM: MULTI-AGENT IPC LOGS ACTIVE", 15, 15, 20, DARKBLUE);
        if (allFinished) {
            DrawText("STATUS: ALL AGENTS ARRIVED (RELEASED)", 15, 40, 18, GOLD);
        } else {
            DrawText(isRunning ? "STATUS: RUNNING" : "STATUS: PAUSED", 15, 40, 18, isRunning ? LIME : DARKGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    for (int i = 0; i < total_passengers; i++) {
        if (global_pids[i] > 0) {
            kill(global_pids[i], SIGKILL);
            waitpid(global_pids[i], NULL, 0);
            close(agent_pipes[i][0]);
        }
    }

    if (sourcesArray) free(sourcesArray);
    if (destsArray) free(destsArray);
    freeGraph(graph);

    return 0;
}