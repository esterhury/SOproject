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

// Define the structure for IPC messages used for communication between child processes and the parent
typedef struct {
    int agentIndex;
    int currentNode;
    int nextNode;
    bool isWaiting;
    bool isFinished;
} IPCMessage;

// Global variables for process management and shared simulation state
pid_t global_pids[MAX_PASSENGERS] = {0};
int agent_pipes[MAX_PASSENGERS][2];
Passenger shared_passengers[MAX_PASSENGERS];
int total_passengers = 3; // Matched to 3 travelers from the professor's sample

// Member 1 - Milestone 6: Global graph pointer needed for the signal handler to free resources
Graph* global_graph = NULL;
Vector2 visual_targets[MAX_PASSENGERS];
float animation_speed = 0.04f;

// NEW: Variable to track the active centralized scheduling algorithm (FCFS or SJF)
int current_algorithm = SCHEDULING_FCFS;

// Member 1 - Milestone 6: Signal handler for graceful simulation shutdown
// This function catches interrupt signals (like Ctrl+C) to prevent zombie processes
void handle_sigint(int sig) {
    printf("\n[Signal %d caught] Gracefully shutting down simulation...\n", sig);

    // 1. Terminate all child processes politely using SIGTERM to allow cleanup
    for (int i = 0; i < total_passengers; i++) {
        if (global_pids[i] > 0) {
            kill(global_pids[i], SIGTERM);
            waitpid(global_pids[i], NULL, 0); // Reaping processes to prevent zombies
        }
        // 2. Close communication pipes to release file descriptors
        close(agent_pipes[i][0]);
        close(agent_pipes[i][1]);
    }

    // 3. Clean up the shared graph resources and memory safely
    if (global_graph != NULL) {
        freeGraph(global_graph);
    }

    printf("Cleanup complete. Exiting.\n");
    exit(0);
}

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
        // NEW: Updated text to reflect that waiting is now managed by the parent scheduler
        const char* waitText = "SCHEDULING...";
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

    // NEW: The child no longer locks the graph node directly. It waits for the parent's permission via private semaphore
    // sem_wait(&(graph->semaphores[myPath.nodes[0]])); // OLD

    IPCMessage msg = {
        .agentIndex = agentIndex,
        .currentNode = myPath.nodes[0],
        .nextNode = (myPath.count > 1) ? myPath.nodes[1] : -1,
        .isWaiting = true, // NEW: Start in waiting state to ask parent for permission
        .isFinished = false
    };

    int unused_ret;
    unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

    // NEW: Wait centrally until the parent grants access to the starting node
    sem_wait(&(graph->agent_semaphores[agentIndex]));

    // NEW: Notify parent we entered successfully
    msg.isWaiting = false;
    msg.isFinished = (myPath.count == 1);
    unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

    // Traverse the computed path
    for (int i = 0; i < myPath.count - 1; i++) {
        int curr = myPath.nodes[i];
        int next = myPath.nodes[i + 1];

        // Simulate transit time along the edge
        for (int step = 0; step < 150; step++) {
            usleep(30000);
        }

        // Notify parent that the agent is waiting outside the next node
        msg.currentNode = curr;
        msg.nextNode = next;
        msg.isWaiting = true;
        unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

        // NEW: Enforce Mutual Exclusion centrally - block on private agent semaphore and let the parent schedule entry
        // sem_wait(&(graph->semaphores[next])); // OLD
        // sem_post(&(graph->semaphores[curr])); // OLD
        sem_wait(&(graph->agent_semaphores[agentIndex])); // NEW

        // Update status and notify parent
        msg.currentNode = next;
        msg.nextNode = (i + 2 < myPath.count) ? myPath.nodes[i + 2] : -1;
        msg.isWaiting = false;
        if (i + 1 == myPath.count - 1) msg.isFinished = true;
        unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

        // Critical Section: Hold the node for exactly 1 second
        sleep(1);
    }

    // Release the final destination node
    // sem_post(&(graph->semaphores[myPath.nodes[myPath.count - 1]])); // OLD
    // NEW: Removed blind sem_post. The parent process handles releasing the final node centrally.

    (void)unused_ret;

    // Keep child alive until main simulation shutdown
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
    int* burstTimesArray = NULL; // NEW: Pointer to hold burst times loaded from file
    int numTravelers = 0;

    // Load initial simulation configuration
    // NEW: Updated function call to include burstTimesArray
    global_graph = loadGraphFromFile("input.txt", &sourcesArray, &destsArray, &burstTimesArray, &numTravelers);
    if (global_graph == NULL) {
        printf("Error: Graph context could not be loaded safely.\n");
        return 1;
    }

    // Member 1 - Milestone 6: Register signal handler for Ctrl+C (SIGINT)
    signal(SIGINT, handle_sigint);

    computePosition(global_graph);

    if (numTravelers > 0 && numTravelers <= MAX_PASSENGERS) {
        total_passengers = numTravelers;
    }

    // NEW: Array to hold burst times for the active passengers
    int burstTimes[MAX_PASSENGERS];

    for (int i = 0; i < MAX_PASSENGERS; i++) {
        shared_passengers[i].simulationFinished = false;
        shared_passengers[i].shortestPath.active = false;
        shared_passengers[i].movingEntity.isMoving = false;
        shared_passengers[i].movingEntity.isWaiting = false;

        // --- FIXED: Reset the path index so cars don't get stuck at node 0! ---
        shared_passengers[i].movingEntity.currentPathIndex = 0;

        shared_passengers[i].carRotation = 0.0f;
        shared_passengers[i].burstTime = 10; // NEW: Default burst time
    }

    int sources[MAX_PASSENGERS];
    int dests[MAX_PASSENGERS];
    for (int i = 0; i < total_passengers; i++) {
        sources[i] = sourcesArray[i];
        dests[i] = destsArray[i];
        burstTimes[i] = burstTimesArray[i]; // NEW: Transfer loaded burst times
    }

    for (int i = 0; i < total_passengers; i++) {
        shared_passengers[i].id = 1000 + i;
        shared_passengers[i].burstTime = burstTimes[i]; // NEW: Assign burst time to passenger struct
        shared_passengers[i].shortestPath.active = true;
        shared_passengers[i].movingEntity.currentPos = global_graph->positions[sources[i]];
        visual_targets[i] = global_graph->positions[sources[i]];
    }

    bool isRunning = false;
    Rectangle playBtn = { 650, 20, 120, 40 };
    Rectangle stopBtn = { 650, 70, 120, 40 };

    InitWindow(800, 600, "Centralized Scheduling Intersection System - Milestone 7");
    SetTargetFPS(60);

    bool process_logged_finished[MAX_PASSENGERS] = {false};

    // NEW: Initialize the centralized node queues in the parent process
    initNodeQueues(global_graph->numVertices);

    while (!WindowShouldClose()) {
        Vector2 mousePoint = GetMousePosition();

        // NEW: Toggle between FCFS and SJF scheduling algorithms by pressing 'S'
        if (IsKeyPressed(KEY_S)) {
            current_algorithm = (current_algorithm == SCHEDULING_FCFS) ? SCHEDULING_SJF : SCHEDULING_FCFS;
        }

        bool allFinished = true;
        for (int i = 0; i < total_passengers; i++) {
            if (!shared_passengers[i].simulationFinished) {
                allFinished = false;
                break;
            }
        }

        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (!isRunning) {
                initNodeQueues(global_graph->numVertices); // NEW: Reset queues on restart
                if (allFinished) {
                    for (int i = 0; i < total_passengers; i++) {
                        waitpid(global_pids[i], NULL, WNOHANG);
                        shared_passengers[i].simulationFinished = false;
                        shared_passengers[i].movingEntity.isWaiting = false;
                        shared_passengers[i].movingEntity.currentPos = global_graph->positions[sources[i]];

                        // --- FIXED: Reset the path index on restart! ---
                        shared_passengers[i].movingEntity.currentPathIndex = 0;

                        visual_targets[i] = global_graph->positions[sources[i]];
                        process_logged_finished[i] = false;
                    }
                    createTravelerProcesses(global_graph, total_passengers, sources, dests);
                } else if (global_pids[0] == 0) {
                    createTravelerProcesses(global_graph, total_passengers, sources, dests);
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

                    Vector2 currentJunctionPos = global_graph->positions[incomingMsg.currentNode];

                    if (incomingMsg.isWaiting) {
                        // NEW: Parent identifies which node the vehicle wants to enter and which node it left
                        int requestedNode = (incomingMsg.nextNode == -1 || shared_passengers[i].movingEntity.currentPathIndex == 0) ? incomingMsg.currentNode : incomingMsg.nextNode;
                        int releaseNodeNum = (requestedNode == incomingMsg.currentNode) ? -1 : incomingMsg.currentNode;

                        // NEW: Add the requesting vehicle to the centralized queue of the target node
                        addToNodeQueue(requestedNode, i, shared_passengers[i].burstTime);

                        // NEW: Free up the previous node and wake up the next waiting vehicle in its queue
                        if (releaseNodeNum != -1) {
                            releaseNode(releaseNodeNum);
                            int nextSelected = scheduleNextAgent(releaseNodeNum, current_algorithm);
                            if (nextSelected != -1) {
                                sem_post(&(global_graph->agent_semaphores[nextSelected]));
                            }
                        }

                        // NEW: Check if the newly requested node is currently free. If yes, schedule and grant access immediately
                        if (getNodeOwner(requestedNode) == -1) {
                            int nextSelected = scheduleNextAgent(requestedNode, current_algorithm);
                            if (nextSelected != -1) {
                                sem_post(&(global_graph->agent_semaphores[nextSelected]));
                            }
                        }

                        Vector2 targetJunctionPos = global_graph->positions[requestedNode];
                        float dx = targetJunctionPos.x - currentJunctionPos.x;
                        float dy = targetJunctionPos.y - currentJunctionPos.y;
                        float len = sqrtf(dx*dx + dy*dy);
                        if (len > 0) {
                            visual_targets[i].x = targetJunctionPos.x - (dx / len) * 35.0f;
                            visual_targets[i].y = targetJunctionPos.y - (dy / len) * 35.0f;
                        }
                    } else {
                        visual_targets[i] = currentJunctionPos;

                        // --- FIXED: Progress the path index so the car knows it advanced! ---
                        shared_passengers[i].movingEntity.currentPathIndex++;

                        if (incomingMsg.nextNode != -1) {
                            printf("[PID=%d] arrived at node %d | next node: %d\n", global_pids[i], incomingMsg.currentNode, incomingMsg.nextNode);
                        } else {
                            printf("[PID=%d] arrived at node %d | DESTINATION\n", global_pids[i], incomingMsg.currentNode);
                            shared_passengers[i].simulationFinished = true;

                            // NEW: Centralized release of the final destination node and waking up queued vehicles
                            releaseNode(incomingMsg.currentNode);
                            int nextSelected = scheduleNextAgent(incomingMsg.currentNode, current_algorithm);
                            if (nextSelected != -1) {
                                sem_post(&(global_graph->agent_semaphores[nextSelected]));
                            }
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
            if (allFinished) {
                for (int i = 0; i < total_passengers; i++) {
                    if (!process_logged_finished[i]) {
                        printf("[PID=%d] finished\n", global_pids[i]);
                        fflush(stdout);
                        // Member 1 - Milestone 6: Replace SIGKILL with SIGTERM for graceful exit
                        kill(global_pids[i], SIGTERM);
                        waitpid(global_pids[i], NULL, 0);
                        process_logged_finished[i] = true;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawGraph(global_graph, (Path){.active=false});

        for (int i = 0; i < total_passengers; i++) {
            if (!process_logged_finished[i]) {
                Color carColors[] = { BLUE, GREEN, PURPLE, ORANGE };
                Color myColor = carColors[i % 4]; //

                DrawCar(shared_passengers[i].movingEntity.currentPos, shared_passengers[i].carRotation, myColor, shared_passengers[i].movingEntity.isWaiting);

                // NEW: Show Burst Time next to the PID on the screen for SJF verification
                DrawText(TextFormat("PID: %d [Burst: %d]", shared_passengers[i].id, shared_passengers[i].burstTime),
                         shared_passengers[i].movingEntity.currentPos.x - 20,
                         shared_passengers[i].movingEntity.currentPos.y - 25, 12, DARKGRAY);
            }
        }

        DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
        DrawText(allFinished ? "RESTART" : "PLAY", playBtn.x + 15, playBtn.y + 10, 20, BLACK);
        DrawRectangleRec(stopBtn, RED);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        // NEW: Draw UI text to show the current Scheduler Mode
        DrawText(TextFormat("Scheduler Mode (Press 'S' to Switch): %s", (current_algorithm == SCHEDULING_FCFS) ? "FCFS (First-Come)" : "SJF (Shortest-Job)"), 20, 20, 18, BLACK);

        EndDrawing();
    }

    CloseWindow();
    // Final cleanup call
    handle_sigint(0);

    if (sourcesArray) free(sourcesArray);
    if (destsArray) free(destsArray);
    if (burstTimesArray) free(burstTimesArray); // NEW: Free the dynamically allocated burst times array
    freeGraph(global_graph);

    return 0;
}