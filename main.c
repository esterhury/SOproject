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
int total_passengers = 3;

Graph* global_graph = NULL;
Vector2 visual_targets[MAX_PASSENGERS];
float animation_speed = 0.05f;

#define SCHEDULING_FCFS 0
#define SCHEDULING_PRIORITY 1

int current_algorithm = SCHEDULING_FCFS;

void handle_sigint(int sig) {
    printf("\n[Signal %d caught] Gracefully shutting down simulation...\n", sig);
    for (int i = 0; i < total_passengers; i++) {
        if (global_pids[i] > 0) {
            kill(global_pids[i], SIGTERM);
            waitpid(global_pids[i], NULL, 0);
        }
        close(agent_pipes[i][0]);
        close(agent_pipes[i][1]);
    }
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

    IPCMessage msg = {
        .agentIndex = agentIndex,
        .currentNode = myPath.nodes[0],
        .nextNode = (myPath.count > 1) ? myPath.nodes[1] : -1,
        .isWaiting = true,
        .isFinished = false
    };

    int unused_ret;
    unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

    sem_wait(&(graph->agent_semaphores[agentIndex]));

    msg.isWaiting = false;
    msg.isFinished = (myPath.count == 1);
    unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

    for (int i = 0; i < myPath.count - 1; i++) {
        int curr = myPath.nodes[i];
        int next = myPath.nodes[i + 1];

        for (int step = 0; step < 100; step++) {
            usleep(20000);
        }

        msg.currentNode = curr;
        msg.nextNode = next;
        msg.isWaiting = true;
        unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

        sem_wait(&(graph->agent_semaphores[agentIndex]));

        msg.currentNode = next;
        msg.nextNode = (i + 2 < myPath.count) ? myPath.nodes[i + 2] : -1;
        msg.isWaiting = false;
        if (i + 1 == myPath.count - 1) msg.isFinished = true;
        unused_ret = write(agent_pipes[agentIndex][1], &msg, sizeof(IPCMessage));

        sleep(1);
    }

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

int main(int argc, char* argv[]) {
    int algo_idx = 1;
    char* input_filename = "input.txt";

    if (argc > 1 && strcmp(argv[1], "-schd") == 0) {
        algo_idx = 2;
    }

    if (argc > algo_idx) {
        if (strcmp(argv[algo_idx], "fcfs") == 0) {
            current_algorithm = SCHEDULING_FCFS;
            printf("Selected Algorithm: FCFS\n");
        } else if (strcmp(argv[algo_idx], "sjf") == 0) {
            current_algorithm = SCHEDULING_PRIORITY;
            printf("Selected Algorithm: SJF (Priority)\n");
        } else if (strcmp(argv[algo_idx], "priority") == 0) {
            current_algorithm = SCHEDULING_PID_PRIORITY;
            printf("Selected Algorithm: PID Priority\n");
        } else {
            printf("Warning: Unknown algorithm '%s'. Defaulting to FCFS.\n", argv[algo_idx]);
            current_algorithm = SCHEDULING_FCFS;
        }
        if (argc > algo_idx + 1) {
            input_filename = argv[algo_idx + 1];
        }
    } else {
        printf("No algorithm specified. Defaulting to FCFS.\n");
        current_algorithm = SCHEDULING_FCFS;
    }
    printf("Loading input file: %s\n", input_filename);
    fflush(stdout);

    int* sourcesArray = NULL;
    int* destsArray = NULL;
    int* burstTimesArray = NULL;
    int numTravelers = 0;

    global_graph = loadGraphFromFile(input_filename, &sourcesArray, &destsArray, &burstTimesArray, &numTravelers);
    if (global_graph == NULL) {
        printf("Error: Graph context could not be loaded safely.\n");
        return 1;
    }

    signal(SIGINT, handle_sigint);
    computePosition(global_graph);

    if (numTravelers > 0 && numTravelers <= MAX_PASSENGERS) {
        total_passengers = numTravelers;
    }

    int burstTimes[MAX_PASSENGERS];
    for (int i = 0; i < MAX_PASSENGERS; i++) {
        shared_passengers[i].simulationFinished = false;
        shared_passengers[i].shortestPath.active = false;
        shared_passengers[i].movingEntity.isMoving = false;
        shared_passengers[i].movingEntity.isWaiting = false;
        shared_passengers[i].movingEntity.currentPathIndex = 0;
        shared_passengers[i].carRotation = 0.0f;
        shared_passengers[i].priority = 10;
        // משתנה עזר פנימי לספירת זמן השהייה בצומת (עבור האנימציה)
        shared_passengers[i].id = 0;
    }

    int sources[MAX_PASSENGERS];
    int dests[MAX_PASSENGERS];
    for (int i = 0; i < total_passengers; i++) {
        sources[i] = sourcesArray[i];
        dests[i] = destsArray[i];
        burstTimes[i] = burstTimesArray[i];
    }

    for (int i = 0; i < total_passengers; i++) {
        shared_passengers[i].priority = burstTimes[i];
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
    bool summary_printed = false;

    // מערך שעוקב אחרי זמני ה-Burst שנותרו לכל מכונית בצומת הנוכחי (בפריימים)
    int remaining_burst_frames[MAX_PASSENGERS] = {0};
    int current_node_occupied[MAX_PASSENGERS];
    for(int i=0; i<MAX_PASSENGERS; i++) current_node_occupied[i] = -1;

    initNodeQueues(global_graph->numVertices);

    while (!WindowShouldClose()) {
        Vector2 mousePoint = GetMousePosition();

        if (IsKeyPressed(KEY_S)) {
            if (current_algorithm == SCHEDULING_FCFS) {
                current_algorithm = SCHEDULING_PRIORITY;
            } else if (current_algorithm == SCHEDULING_PRIORITY) {
                current_algorithm = SCHEDULING_PID_PRIORITY;
            } else {
                current_algorithm = SCHEDULING_FCFS;
            }
        }

        bool allFinished = true;
        for (int i = 0; i < total_passengers; i++) {
            if (!shared_passengers[i].simulationFinished) {
                allFinished = false;
                break;
            }
        }

        if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (allFinished) {
                initNodeQueues(global_graph->numVertices);
                for (int n = 0; n < global_graph->numVertices; n++) releaseNode(n);
                for (int i = 0; i < total_passengers; i++) {
                    if (global_pids[i] > 0) {
                        kill(global_pids[i], SIGKILL);
                        waitpid(global_pids[i], NULL, 0);
                        global_pids[i] = 0;
                    }
                    shared_passengers[i].simulationFinished = false;
                    shared_passengers[i].movingEntity.isWaiting = false;
                    shared_passengers[i].movingEntity.currentPos = global_graph->positions[sources[i]];
                    shared_passengers[i].movingEntity.currentPathIndex = 0;
                    visual_targets[i] = global_graph->positions[sources[i]];
                    process_logged_finished[i] = false;
                    remaining_burst_frames[i] = 0;
                    current_node_occupied[i] = -1;
                }
                summary_printed = false;
                createTravelerProcesses(global_graph, total_passengers, sources, dests);
                isRunning = true;
            }
            else if (!isRunning) {
                initNodeQueues(global_graph->numVertices);
                for (int n = 0; n < global_graph->numVertices; n++) releaseNode(n);
                createTravelerProcesses(global_graph, total_passengers, sources, dests);
                isRunning = true;
            }
        }

        if (CheckCollisionPointRec(mousePoint, stopBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isRunning = false;
        }

        if (isRunning && !allFinished) {
            // לולאת עדכון זמן ה-Burst של מכוניות שנמצאות כרגע בתוך צומת
            for (int i = 0; i < total_passengers; i++) {
                if (remaining_burst_frames[i] > 0) {
                    remaining_burst_frames[i]--;

                    // אם המכונית סיימה את זמן השהייה שלה בצומת - נשחרר אותו עבור הבאים בתור!
                    if (remaining_burst_frames[i] == 0 && current_node_occupied[i] != -1) {
                        int compNode = current_node_occupied[i];
                        releaseNode(compNode);
                        current_node_occupied[i] = -1;

                        int nextSelected = scheduleNextAgent(compNode, current_algorithm);
                        if (nextSelected != -1) {
                            sem_post(&(global_graph->agent_semaphores[nextSelected]));
                        }
                    }
                }
            }

            for (int i = 0; i < total_passengers; i++) {
                IPCMessage incomingMsg;
                ssize_t bytesRead = read(agent_pipes[i][0], &incomingMsg, sizeof(IPCMessage));

                if (bytesRead == sizeof(IPCMessage)) {
                    Vector2 currentJunctionPos = global_graph->positions[incomingMsg.currentNode];

                    if (incomingMsg.isWaiting) {
                        int requestedNode = (shared_passengers[i].movingEntity.currentPathIndex == 0) ? incomingMsg.currentNode : incomingMsg.nextNode;
                        int releaseNodeNum = (shared_passengers[i].movingEntity.currentPathIndex == 0) ? -1 : incomingMsg.currentNode;

                        if (getNodeOwner(requestedNode) != i) {
                            if (!shared_passengers[i].movingEntity.isWaiting) {
                                shared_passengers[i].movingEntity.isWaiting = true;
                                addToNodeQueue(requestedNode, i, shared_passengers[i].priority);
                            }

                            if (releaseNodeNum != -1 && remaining_burst_frames[i] == 0) {
                                releaseNode(releaseNodeNum);
                                int nextSelected = scheduleNextAgent(releaseNodeNum, current_algorithm);
                                if (nextSelected != -1) {
                                    sem_post(&(global_graph->agent_semaphores[nextSelected]));
                                }
                            }

                            if (getNodeOwner(requestedNode) == -1) {
                                int nextSelected = scheduleNextAgent(requestedNode, current_algorithm);
                                if (nextSelected != -1) {
                                    sem_post(&(global_graph->agent_semaphores[nextSelected]));
                                }
                            }
                        } else {
                            shared_passengers[i].movingEntity.isWaiting = false;
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
                        // המכונית קיבלה אישור ונכנסה לצומת! נפעיל לה את ה-Burst Time הגרפי
                        shared_passengers[i].movingEntity.isWaiting = false;
                        visual_targets[i] = currentJunctionPos;
                        shared_passengers[i].movingEntity.currentPathIndex++;

                        // המרה של ערך ה-Priority/Burst לשניות על המסך (למשל פריוריטי 50 יהיה 150 פריימים = 2.5 שניות)
                        remaining_burst_frames[i] = incomingMsg.isWaiting ? 10 : (shared_passengers[i].priority * 3);
                        current_node_occupied[i] = incomingMsg.currentNode;

                        if (incomingMsg.nextNode != -1) {
                            printf("[PID=%d] arrived at node %d | next node: %d (Burst active)\n", global_pids[i], incomingMsg.currentNode, incomingMsg.nextNode);
                        } else {
                            printf("[PID=%d] arrived at node %d | DESTINATION REACHED\n", global_pids[i], incomingMsg.currentNode);
                            shared_passengers[i].simulationFinished = true;
                        }
                        fflush(stdout);
                    }
                }
            }
        }

        // עדכון אנימציה חלק
        for (int i = 0; i < total_passengers; i++) {
            float dx = visual_targets[i].x - shared_passengers[i].movingEntity.currentPos.x;
            float dy = visual_targets[i].y - shared_passengers[i].movingEntity.currentPos.y;
            if (sqrtf(dx*dx + dy*dy) > 1.0f) {
                shared_passengers[i].movingEntity.currentPos.x += dx * 0.05f;
                shared_passengers[i].movingEntity.currentPos.y += dy * 0.05f;
                shared_passengers[i].carRotation = atan2f(dy, dx) * (180.0f / PI);
            }
        }

        if (allFinished && isRunning) {
            bool physicallyArrived = true;
            for(int i=0; i<total_passengers; i++) {
                float dx = visual_targets[i].x - shared_passengers[i].movingEntity.currentPos.x;
                float dy = visual_targets[i].y - shared_passengers[i].movingEntity.currentPos.y;
                if(sqrtf(dx*dx + dy*dy) > 5.0f) physicallyArrived = false;
            }

            if (physicallyArrived) {
                for (int i = 0; i < total_passengers; i++) {
                    if (!process_logged_finished[i]) {
                        printf("[PID=%d] finished tracking\n", global_pids[i]);
                        kill(global_pids[i], SIGTERM);
                        waitpid(global_pids[i], NULL, 0);
                        process_logged_finished[i] = true;
                    }
                }
                if (!summary_printed) {
                    printf("\n=============================================\n");
                    printf(" SUCCESS: All travelers reached their destination!\n");
                    printf("=============================================\n\n");
                    fflush(stdout);
                    summary_printed = true;
                }
                isRunning = false;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawGraph(global_graph, (Path){.active=false});

        for (int i = 0; i < total_passengers; i++) {
            if (!process_logged_finished[i]) {
                Color carColors[] = { BLUE, GREEN, PURPLE, ORANGE };
                Color myColor = carColors[i % 4];

                DrawCar(shared_passengers[i].movingEntity.currentPos, shared_passengers[i].carRotation, myColor, shared_passengers[i].movingEntity.isWaiting);

                DrawText(TextFormat("PID: %d [Burst: %d]", global_pids[i], shared_passengers[i].priority),
                         shared_passengers[i].movingEntity.currentPos.x - 20,
                         shared_passengers[i].movingEntity.currentPos.y - 25, 12, DARKGRAY);
            }
        }

        if (allFinished) {
            DrawRectangleRec(playBtn, GREEN);
            DrawText("RESTART", playBtn.x + 15, playBtn.y + 10, 20, BLACK);
        } else {
            DrawRectangleRec(playBtn, isRunning ? LIME : GREEN);
            DrawText("PLAY", playBtn.x + 35, playBtn.y + 10, 20, BLACK);
        }

        DrawRectangleRec(stopBtn, RED);
        DrawText("STOP", stopBtn.x + 35, stopBtn.y + 10, 20, WHITE);

        char modeUpper[15];
        Color algoColor = BLUE;

        if (current_algorithm == SCHEDULING_PRIORITY) {
            strcpy(modeUpper, "SJF");
            algoColor = PURPLE;
        } else if (current_algorithm == SCHEDULING_PID_PRIORITY) {
            strcpy(modeUpper, "PID PRIO");
            algoColor = RED;
        } else {
            strcpy(modeUpper, "FCFS");
            algoColor = BLUE;
        }

        DrawRectangle(20, 20, 180, 40, LIGHTGRAY);
        DrawRectangleLines(20, 20, 180, 40, DARKGRAY);
        DrawText("SCHEDULER:", 30, 32, 12, DARKGRAY);
        DrawText(modeUpper, 110, 30, 16, algoColor);
        EndDrawing();
    }

    CloseWindow();
    handle_sigint(0);

    if (sourcesArray) free(sourcesArray);
    if (destsArray) free(destsArray);
    if (burstTimesArray) free(burstTimesArray);
    freeGraph(global_graph);

    return 0;
}