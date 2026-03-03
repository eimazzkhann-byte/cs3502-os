CC=gcc
CFLAGS=-O2 -Wall -Wextra -pthread

all: phase1 phase2 phase3 phase4

phase1: phase1.c
	$(CC) $(CFLAGS) -o phase1 phase1.c

phase2: phase2.c
	$(CC) $(CFLAGS) -o phase2 phase2.c

phase3: phase3.c
	$(CC) $(CFLAGS) -o phase3 phase3.c

phase4: phase4.c
	$(CC) $(CFLAGS) -o phase4 phase4.c

clean:
	rm -f phase1 phase2 phase3 phase4 *.o
