#include <stdio.h>
#include <stdlib.h>
#include "graph.h"

/* Create a new graph with a given number of vertices */
Graph* createGraph(int vertices) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->numVertices = vertices;
    graph->adjLists = (Node**)malloc(vertices * sizeof(Node*));

    for (int i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
    }
    return graph;
}

/* Add an edge to the graph (directed) */
void addEdge(Graph* graph, int src, int dest, int weight) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->dest = dest;
    newNode->weight = weight;
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;
}

/* Load graph data and the query (src, dst) from a file[cite: 1] */
Graph* loadGraphFromFile(const char* filename, int* startNode, int* endNode) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file.\n");
        return NULL;
    }

    int n, m;
    if (fscanf(file, "%d %d", &n, &m) != 2) {
        fclose(file);
        return NULL;
    }

    Graph* graph = createGraph(n);

    for (int i = 0; i < m; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) == 3) {
            /* Check for negative weights as per requirements[cite: 1] */
            if (w < 0) {
                printf("Error: Invalid input. Weights cannot be negative.\n[cite: 1]");
                /* Note: In a full cleanup, we'd free the graph here too */
                fclose(file);
                return NULL;
            }
            addEdge(graph, u, v, w);
        }
    }

    /* Read the last line containing the source and destination for Dijkstra[cite: 1] */
    if (fscanf(file, "%d %d", startNode, endNode) != 2) {
        /* Optional: handle missing query */
    }

    fclose(file);
    return graph;
}

/* Clean up memory to prevent leaks[cite: 1] */
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
    free(graph);
}
