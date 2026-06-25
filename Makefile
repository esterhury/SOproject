# The compiler executable to use
CC = gcc

CFLAGS = -Wall -Wextra -O2

# Using the exact library flags from your working previous milestone
LIBS = -L. -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Shared source files implementing common data structures and helper utilities
SRC_SHARED = graph.c

# Output binary targets for clean command tracking
TARGET_FINAL = sim
TARGET_M1    = milestone1
TARGET_M2    = milestone2
TARGET_M3    = milestone3
TARGET_M4    = milestone4
TARGET_M5    = milestone5
TARGET_M6    = milestone6
TARGET_M7    = milestone7

.PHONY: all clean milestone1 milestone2 milestone3 milestone4 milestone5 milestone6 milestone7

# Default target builds the final milestone 7
all: milestone7

# Individual milestone targets as required by the specifications
milestone1:
	$(CC) $(CFLAGS) -o dijkstra main_milestone1.c $(SRC_SHARED) $(LIBS)
	cp dijkstra $(TARGET_M1)

milestone2:
	$(CC) $(CFLAGS) -o $(TARGET_FINAL) main.c $(SRC_SHARED) $(LIBS)
	cp $(TARGET_FINAL) $(TARGET_M2)

milestone3:
	$(CC) $(CFLAGS) -o $(TARGET_FINAL) main.c $(SRC_SHARED) $(LIBS)
	cp $(TARGET_FINAL) $(TARGET_M3)

milestone4:
	$(CC) $(CFLAGS) -o $(TARGET_FINAL) main.c $(SRC_SHARED) $(LIBS)
	cp $(TARGET_FINAL) $(TARGET_M4)

milestone5:
	$(CC) $(CFLAGS) -o $(TARGET_FINAL) main.c $(SRC_SHARED) $(LIBS)
	cp $(TARGET_FINAL) $(TARGET_M5)

milestone6:
	$(CC) $(CFLAGS) -o $(TARGET_FINAL) main.c $(SRC_SHARED) $(LIBS)
	cp $(TARGET_FINAL) $(TARGET_M6)

milestone7:
	$(CC) $(CFLAGS) -o $(TARGET_FINAL) main.c $(SRC_SHARED) $(LIBS)
	cp $(TARGET_FINAL) $(TARGET_M7)

# Alias handling for typos
mailstone1: milestone1
mailstone2: milestone2
mailstone3: milestone3
mailstone4: milestone4
mailstone5: milestone5
mailstone6: milestone6
mailstone7: milestone7

# Removes compiled binary executables and intermediate object files to ensure a clean rebuild environment
clean:
	rm -f $(TARGET_FINAL) dijkstra *.o $(TARGET_M1) $(TARGET_M2) $(TARGET_M3) $(TARGET_M4) $(TARGET_M5) $(TARGET_M6) $(TARGET_M7)