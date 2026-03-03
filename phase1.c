#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS  8
#define OPS_PER_THREAD 200000

// Shared bank accounts (NO LOCKS in Phase 1)
long long accounts[NUM_ACCOUNTS];

void* worker(void* arg) {
    int tid = *(int*)arg;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(tid * 1234567);

    for (int i = 0; i < OPS_PER_THREAD; i++) {
        int from = rand_r(&seed) % NUM_ACCOUNTS;
        int to = (from + 1) % NUM_ACCOUNTS;

        // withdraw
        accounts[from] -= 1;

        // force more thread interleaving so the bug shows up
        if ((i % 5000) == 0) {
            usleep(1);
        }

        // deposit
        accounts[to] += 1;
    }

    return NULL;
}

int main() {
    accounts[0] = 1000000;
    accounts[1] = 1000000;

    long long initial_total = accounts[0] + accounts[1];

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    printf("Phase 1 (NO LOCKS)\n");
    printf("Accounts start: A0=%lld A1=%lld Total=%lld\n", accounts[0], accounts[1], initial_total);

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

    long long final_total = accounts[0] + accounts[1];

    printf("Accounts end:   A0=%lld A1=%lld Total=%lld\n", accounts[0], accounts[1], final_total);
    printf("Expected total: %lld\n", initial_total);

    if (final_total != initial_total) {
        printf("RESULT: Race condition detected (total changed).\n");
    } else {
        printf("RESULT: Total matched this run. Run again.\n");
    }

    return 0;
}
