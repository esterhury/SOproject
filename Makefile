CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

EXEC_SIM4 = sim4
EXEC_SIM5 = sim5
EXEC_DIJKSTRA = dijkstra

all: milestone4

milestone4: main.c graph.c
	$(CC) $(CFLAGS) -o $(EXEC_SIM4) main.c graph.c $(LIBS)

milestone5: main.c graph.c
	$(CC) $(CFLAGS) -o $(EXEC_SIM5) main.c graph.c $(LIBS)

clean:
	rm -f $(EXEC_DIJKSTRA) $(EXEC_SIM4) $(EXEC_SIM5) *.o

.PHONY: all clean milestone4 milestone5
milestone5: main.c graph.c
	$(CC) $(CFLAGS) -o sim5 main.c graph.c $(LIBS)
