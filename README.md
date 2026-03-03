CS 3502 - Project 1: Multi-Threaded Programming

How to build:
  gcc -O2 -Wall -Wextra -pthread -o phase1 phase1.c
  gcc -O2 -Wall -Wextra -pthread -o phase2 phase2.c
  gcc -O2 -Wall -Wextra -pthread -o phase3 phase3.c
  gcc -O2 -Wall -Wextra -pthread -o phase4 phase4.c

How to run:
  ./phase1
  ./phase2
  ./phase3
  ./phase4

Notes:
- Phase 1 demonstrates race conditions (no locks).
- Phase 2 uses a mutex to prevent race conditions.
- Phase 3 intentionally creates deadlock with opposite lock ordering.
- Phase 4 prevents deadlock using lock ordering.
