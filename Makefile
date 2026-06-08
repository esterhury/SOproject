# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 $(shell pkg-config --libs raylib 2>/dev/null)

# Source Files
SRCS = main.c graph.c
OBJS = $(SRCS:.c=.o)
TARGET = sim

# Default Target
all: milestone5

# Milestone 4 Target
milestone4: clean $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

# Milestone 5 Target
milestone5: clean $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

# Object Files Compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cleanup Environment Target
clean:
	rm -f $(TARGET) *.o

.PHONY: all milestone4 milestone5 clean