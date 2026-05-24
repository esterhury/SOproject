# Compiler and Flags Configuration
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# System-wide dynamic library linking mapping
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Output binary configuration execution parameters
EXEC_SIM = sim
EXEC_DIJKSTRA = dijkstra

# Default Target Deployment
all: milestone4

# Milestone 4 DevOps Integration Build Rule
milestone4: main.c graph.c
	$(CC) $(CFLAGS) -o $(EXEC_SIM) main.c graph.c $(LIBS)

# Housekeeping garbage cleanup rule
clean:
	rm -f $(EXEC_DIJKSTRA) $(EXEC_SIM) *.o

.PHONY: all clean milestone4
