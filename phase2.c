#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS  8
#define OPS_PER_THREAD 200000

// Shared bank accounts (Phase 2 uses locks)
long long accounts[NUM_ACCOUNTS];

// One lock protects the whole bank (simple + correct)
pthread_mutex_t bank_lock = PTHREAD_MUTEX_INITIALIZER;

void* worker(void* arg) {
    int tid = *(int*)arg;

    // per-thread RNG seed
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(tid * 1234567);

    for (int i = 0; i < OPS_PER_THREAD; i++) {
        int from = rand_r(&seed) % NUM_ACCOUNTS;
        int to = (from + 1) % NUM_ACCOUNTS;

        // LOCK critical section so updates are atomic
        pthread_mutex_lock(&bank_lock);

        accounts[from] -= 1;

        // keep the tiny sleep to increase interleaving (now safe)
        if ((i % 5000) == 0) {
            usleep(1);
        }

        accounts[to] += 1;

        pthread_mutex_unlock(&bank_lock);
    }

    return NULL;
}

int main() {
    accounts[0] = 1000000;
    accounts[1] = 1000000;

    long long initial_total = accounts[0] + accounts[1];

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    printf("Phase 2 (WITH MUTEX)\n");
    printf("Accounts start: A0=%lld A1=%lld Total=%lld\n", accounts[0], accounts[1], initial_total);

    // timing start
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // timing end
    clock_gettime(CLOCK_MONOTONIC, &end);

    long long final_total = accounts[0] + accounts[1];

    printf("Accounts end:   A0=%lld A1=%lld Total=%lld\n", accounts[0], accounts[1], final_total);
    printf("Expected total: %lld\n", initial_total);

    if (final_total != initial_total) {
        printf("RESULT: Still incorrect (should not happen with mutex).\n");
    } else {
        printf("RESULT: Correct (mutex prevented race conditions).\n");
    }

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("Elapsed time: %.6f seconds\n", elapsed);

    return 0;
}
