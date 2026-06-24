# The compiler executable to use
CC = gcc

# NEW: Added -pthread and -D_GNU_SOURCE for Milestone 7 semaphores and shared memory
CFLAGS = -Wall -Wextra -O2 -pthread -D_GNU_SOURCE

LIBS = -L. -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Shared source files implementing common data structures and helper utilities
SRC_SHARED = graph.c

# Default target updated to milestone 7
all: milestone7

milestone1:
	$(CC) $(CFLAGS) -o dijkstra main_milestone1.c $(SRC_SHARED) $(LIBS)

milestone2:
	$(CC) $(CFLAGS) -o sim main.c $(SRC_SHARED) $(LIBS)

milestone3:
	$(CC) $(CFLAGS) -o sim main.c $(SRC_SHARED) $(LIBS)

milestone4:
	$(CC) $(CFLAGS) -o sim main.c $(SRC_SHARED) $(LIBS)

milestone5:
	$(CC) $(CFLAGS) -o sim main.c $(SRC_SHARED) $(LIBS)

milestone6:
	$(CC) $(CFLAGS) -o sim main.c $(SRC_SHARED) $(LIBS)

# NEW: Added milestone7 target
milestone7:
	$(CC) $(CFLAGS) -o sim main.c $(SRC_SHARED) $(LIBS)

mailstone1: milestone1
mailstone2: milestone2
mailstone3: milestone3
mailstone4: milestone4
mailstone5: milestone5
mailstone6: milestone6
mailstone7: milestone7

# Removes compiled binary executables and intermediate object files to ensure a clean rebuild environment
clean:
	rm -f sim dijkstra *.o