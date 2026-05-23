# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
# Use the local libraylib.a found in the current directory
LIBS = ./libraylib.a -lm -lpthread -ldl -lrt -lX11

# Executable names
EXEC_SIM = sim
EXEC_DIJKSTRA = dijkstra

# Default target
all: milestone3

# Milestone 1: Terminal-based pathfinding
milestone1: main.c graph.c
	$(CC) $(CFLAGS) -o $(EXEC_DIJKSTRA) main.c graph.c $(LIBS)

# Milestone 2 & 3: GUI and Animation
milestone2: main.c graph.c
	$(CC) $(CFLAGS) -o $(EXEC_SIM) main.c graph.c $(LIBS)

milestone3: main.c graph.c
	$(CC) $(CFLAGS) -o $(EXEC_SIM) main.c graph.c $(LIBS)

# Clean build files
clean:
	rm -f $(EXEC_DIJKSTRA) $(EXEC_SIM) *.o

.PHONY: all clean milestone1 milestone2 milestone3