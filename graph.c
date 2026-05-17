#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "raylib.h"
#include "graph.h"
#include <stdbool.h>
#include <math.h>

// Create a new graph with a given number of vertices
Graph* createGraph(int vertices){
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->numVertices = vertices;
    graph->adjLists = (Node**)malloc(vertices * sizeof(Node*));

    for (int i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
    }
    graph->positions = (Vector2*)malloc(vertices * sizeof(Vector2));
    return graph;
}

// Add an edge to the graph
void addEdge(Graph* graph, int src, int dest, int weight) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->dest = dest;
    newNode->weight = weight;
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;
}

// Load graph structure and dynamic travelers arrays from the input file
Graph* loadGraphFromFile(const char* filename, int** sourcesArray, int** destsArray, int* numTravelers) {

    // Open the input file for reading operations only
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file.\n");
        return NULL;
    }

    // Read the total number of vertices (n) and edges (m) from the first line
    int n, m;
    if (fscanf(file, "%d %d", &n, &m) != 2) {
        fclose(file);
        return NULL;
    }

    // Allocate memory and initialize the graph structure
    Graph* graph = createGraph(n);

    // Loop through all edges and add them to the adjacency list
    for (int i = 0; i < m; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) == 3) {
            // Validate that edge weights are non-negative to ensure Dijkstra correctness
            if (w < 0) {
                printf("Error: Invalid input. Weights cannot be negative.\n");
                fclose(file);
                freeGraph(graph);
                return NULL;
            }
            addEdge(graph, u, v, w);
        }
    }

    char line[256];
    bool foundTravelers = false;

    // Scan the file line by line to locate the travelers header
    while (fgets(line, sizeof(line), file)) {
        // Look for the keyword travelers dynamically to ensure reliable parsing
        if (strstr(line, "travelers") != NULL) {
            foundTravelers = true;
            break;
        }
    }

    // Handle the error case where the travelers section is missing
    if (!foundTravelers) {
        printf("Error: No Travelers found.\n");
        *numTravelers = 0;
        *sourcesArray = NULL;
        *destsArray = NULL;
        fclose(file);
        return graph;
    }

    // Read the total number of travelers located right under the header
    if (fscanf(file, "%d", numTravelers) != 1) {
        printf("Error: could not read number of travelers.\n");
        *numTravelers = 0;
        fclose(file);
        return graph;
    }

    // Allocate dynamic memory on the Heap for source and destination arrays
    *sourcesArray = (int*)malloc((*numTravelers) * sizeof(int));
    *destsArray = (int*)malloc((*numTravelers) * sizeof(int));

    // Verify that memory allocation for the dynamic arrays succeeded completely
    if (*sourcesArray == NULL || *destsArray == NULL) {
        printf("Error: Memory allocation failed for travelers arrays.\n");
        *numTravelers = 0;
        fclose(file);
        return graph;
    }

    // Read all individual source and destination pairs into the allocated arrays
    for (int i = 0; i < *numTravelers; i++) {
        // Extract traveler data directly into the array indices via pointers
        if (fscanf(file, "%d %d", &((*sourcesArray)[i]), &((*destsArray)[i])) != 2) {
            printf("Warning: Error reading data for traveler %d. Setting to -1.\n", i);
            (*sourcesArray)[i] = -1;
            (*destsArray)[i] = -1;
        }
    }

    fclose(file);
    return graph;
}

//Clean up memory to prevent leaks
void freeGraph(Graph* graph) {
    if (!graph) return;
    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Node* toDelete = temp;
            temp = temp->next;
            free(toDelete);
        }
    }
    free(graph->adjLists);
    free(graph->positions);
    free(graph);
}

// Finds the shortest path cost from source to destination.
int dijkstra(Graph* graph, int src, int dst, int parent[]) {
    if (graph == NULL || src < 0 || src >= graph->numVertices || dst < 0 || dst >= graph->numVertices) {
        return -1;
    }

    int numVertices = graph->numVertices;
    int* distances = (int*)malloc(numVertices * sizeof(int));
    int* visited = (int*)malloc(numVertices * sizeof(int));

    for (int i = 0; i < numVertices; i++) {
        distances[i] = INT_MAX;
        visited[i] = 0;
        parent[i] = -1;
    }
    distances[src] = 0;
    for (int i = 0; i < numVertices - 1; i++) {
        int minDistance = INT_MAX;
        int u = -1;
        for (int v = 0; v < numVertices; v++) {
            if (visited[v] == 0 && distances[v] <= minDistance) {
                minDistance = distances[v];
                u = v;
            }
        }
        if (u == -1 || distances[u] == INT_MAX || u == dst) {
            break;
        }
        visited[u] = 1;
        Node* current = graph->adjLists[u];
        while (current) {
            int v = current->dest;
            int weight = current->weight;

            if (!visited[v] && distances[u] != INT_MAX && (distances[u] + weight < distances[v])) {
                distances[v] = distances[u] + weight;
                parent[v] = u;
            }
            current = current->next;
        }
    }
    int result = distances[dst];
    free(distances);
    free(visited);

    return result;
}

// printing path
void printPath(int* parent, int src, int dst) {
    if (dst == src) {
        printf("%d", src);
        return;
    }
    printPath(parent, src, parent[dst]);
    printf(" -> %d", dst);
}

void computePosition(Graph* graph) {
    if (!graph || graph->numVertices <= 0) {
        return;
    }
    int cols = 4;
    int rows = (graph->numVertices / cols) + 1;
    float cellWidth = 800.0f / cols;
    float cellHeight = 600.0f / rows;
    for (int i = 0; i < graph->numVertices; i++) {
        int r = i / cols;
        int c = i % cols;
        float baseX = (c * cellWidth) + (cellWidth / 2.0f);
        float baseY = (r * cellHeight) + (cellHeight / 2.0f);

        graph->positions[i].x = baseX + GetRandomValue(-20, 20);
        graph->positions[i].y = baseY + GetRandomValue(-20, 20);
    }
}

void drawGraph(Graph* graph, Path path) {
    if (!graph) return;

    const char* cityNames[] = {"Jerusalem", "Tel Aviv", "Haifa", "Eilat", "Beersheba", "Ashdod", "Tiberias"};
    ClearBackground((Color){ 240, 242, 245, 255 });

    for (int i = 0; i < graph->numVertices; i++) {
        Node* temp = graph->adjLists[i];
        while (temp) {
            Vector2 startPos = graph->positions[i];
            Vector2 endPos = graph->positions[temp->dest];

            Color edgeColor = (Color){ 160, 160, 160, 255 };
            float thickness = 2.0f;

            if (path.active) {
                for (int k = 0; k < path.count - 1; k++) {
                    if (path.nodes[k] == i && path.nodes[k+1] == temp->dest) {
                        edgeColor = YELLOW;
                        thickness = 4.5f;
                        break;
                    }
                }
            }
            DrawLineEx(startPos, endPos, thickness, edgeColor);

            float dx = endPos.x - startPos.x;
            float dy = endPos.y - startPos.y;
            float length = sqrtf(dx * dx + dy * dy);

            if (length > 0) {
                Vector2 unitDir = { dx / length, dy / length };

                Vector2 arrowPos = { endPos.x - unitDir.x * 28, endPos.y - unitDir.y * 28 };
                DrawPoly(arrowPos, 3, 7, atan2f(dy, dx) * RAD2DEG, edgeColor);
                Vector2 weightPos = {
                    startPos.x + unitDir.x * (length * 0.33f),
                    startPos.y + unitDir.y * (length * 0.33f)
                };

                char weightStr[12];
                sprintf(weightStr, "%d km", temp->weight);
                int fontSize = 14;
                int textWidth = MeasureText(weightStr, fontSize);

                DrawRectangle(weightPos.x - (textWidth/2 + 3), weightPos.y - 9, textWidth + 6, 18, (Color){ 231, 76, 60, 255 });
                DrawText(weightStr, weightPos.x - textWidth/2, weightPos.y - 7, fontSize, WHITE);
            }
            temp = temp->next;
        }
    }

    for (int i = 0; i < graph->numVertices; i++) {
        Vector2 pos = graph->positions[i];
        float radius = 22.0f;

        DrawCircleV(pos, radius, (Color){ 44, 62, 80, 255 });
        DrawCircleLinesV(pos, radius, (Color){ 52, 152, 219, 255 });

        char idStr[16];
        sprintf(idStr, "%d", i);
        int idWidth = MeasureText(idStr, 18);
        DrawText(idStr, pos.x - idWidth / 2, pos.y - 9, 18, WHITE);

        if (i < 7) {
            int cityFontSize = 16;
            int cityNameWidth = MeasureText(cityNames[i], cityFontSize);
            float yOffset = (pos.y < 300) ? -45 : 30;

            DrawText(cityNames[i], pos.x - (cityNameWidth / 2), pos.y + yOffset, cityFontSize, (Color){ 44, 62, 80, 255 });
        }
    }
}

Path reconstructPath(int* parent, int src, int dst) {
    Path p;
    p.count = 0;
    p.active = false;

    if (dst == -1 || (parent[dst] == -1 && src != dst)) {
        return p;
    }

    int current = dst;
    int tempPath[100];
    int tempCount = 0;

    while (current != -1) {
        tempPath[tempCount++] = current;
        if (current == src) break;
        current = parent[current];
    }

    for (int i = 0; i < tempCount; i++) {
        p.nodes[i] = tempPath[tempCount - 1 - i];
    }

    p.count = tempCount;
    p.active = true;

    return p;
}

void updateEntity(Entity* entity, Graph* graph, Path* path) {
    if (!entity->isMoving || !path->active || entity->currentPathIndex >= path->count - 1) {
        return;
    }

    int u = path->nodes[entity->currentPathIndex];
    int v = path->nodes[entity->currentPathIndex + 1];

    int weight = 1;
    Node* temp = graph->adjLists[u];
    while (temp) {
        if (temp->dest == v) {
            weight = temp->weight;
            break;
        }
        temp = temp->next;
    }

    if (entity->isWaiting) {
        entity->frameCounter++;
        if (entity->frameCounter >= 60) {
            entity->isWaiting = false;
            entity->frameCounter = 0;
            entity->currentStep = 0;
        }
        return;
    }

    entity->frameCounter++;

    if (entity->frameCounter >= 18) {
        entity->currentStep++;
        entity->frameCounter = 0;

        float t = (float)entity->currentStep / weight;

        if (t > 1.0f) t = 1.0f;

        entity->currentPos.x = graph->positions[u].x + t * (graph->positions[v].x - graph->positions[u].x);
        entity->currentPos.y = graph->positions[u].y + t * (graph->positions[v].y - graph->positions[u].y);

        if (entity->currentStep >= weight) {
            entity->currentPathIndex++;

            if (entity->currentPathIndex < path->count - 1) {
                entity->isWaiting = true;
            } else {
                entity->isMoving = false;
            }
        }
    }
}

void drawEntity(Entity* entity) {
    if (!entity->isMoving && entity->currentPathIndex == 0) return;

    DrawCircleV(entity->currentPos, 12, RED);
    DrawCircleLinesV(entity->currentPos, 12, MAROON);

    DrawText("BUS", entity->currentPos.x - 15, entity->currentPos.y - 25, 12, DARKGRAY);
}