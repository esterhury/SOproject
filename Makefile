
# The compiler executable to use
CC = gcc

CFLAGS = -Wall -Wextra -O2

LIBS = -L. -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Shared source files implementing common data structures and helper utilities
SRC_SHARED = graph.c

# Default target
all: milestone6

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


mailstone1: milestone1
mailstone2: milestone2
mailstone3: milestone3
mailstone4: milestone4
mailstone5: milestone5
mailstone6: milestone6


# Removes compiled binary executables and intermediate object files to ensure a clean rebuild environment
clean:
	rm -f sim dijkstra *.o