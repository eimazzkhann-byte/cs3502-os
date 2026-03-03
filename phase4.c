#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_ACCOUNTS 2
#define TRANSFER_AMOUNT 1

typedef struct {
    int from;
    int to;
    int loops;
} transfer_args_t;

long long accounts[NUM_ACCOUNTS];
pthread_mutex_t account_lock[NUM_ACCOUNTS];

// Used by watchdog + optional progress prints
volatile long long progress_counter = 0;

// Deadlock FIX: enforce lock ordering (always lock lower id first)
static void transfer_ordered(int from, int to, int amount) {
    int first  = (from < to) ? from : to;
    int second = (from < to) ? to   : from;

    pthread_mutex_lock(&account_lock[first]);
    pthread_mutex_lock(&account_lock[second]);

    // critical section
    accounts[from] -= amount;
    accounts[to]   += amount;

    progress_counter++;

    pthread_mutex_unlock(&account_lock[second]);
    pthread_mutex_unlock(&account_lock[first]);
}

static void* transfer_thread(void* arg) {
    transfer_args_t* t = (transfer_args_t*)arg;

    printf("Thread started: transfers %d -> %d (loops=%d)\n", t->from, t->to, t->loops);
    fflush(stdout);

    for (int i = 1; i <= t->loops; i++) {
        transfer_ordered(t->from, t->to, TRANSFER_AMOUNT);

        // progress print every 25k ops so you can see it's alive
        if ((i % 25000) == 0) {
            printf("Thread %d->%d progress: i=%d progress_counter=%lld\n",
                   t->from, t->to, i, progress_counter);
            fflush(stdout);
        }

        // tiny yield occasionally (NOT inside lock) to stress interleaving lightly
        if ((i % 50000) == 0) {
            sched_yield();
        }
    }

    printf("Thread finished: %d -> %d\n", t->from, t->to);
    fflush(stdout);
    return NULL;
}

static void* watchdog_thread(void* arg) {
    (void)arg;
    long long last = progress_counter;

    // check 6 times (about 12 seconds total)
    for (int checks = 0; checks < 6; checks++) {
        sleep(2);
        long long now = progress_counter;

        if (now == last) {
            printf("\nWATCHDOG: No progress for 2 seconds.\n");
            printf("WATCHDOG: This would smell like deadlock, but Phase 4 should avoid it.\n\n");
            fflush(stdout);
            return NULL;
        }

        last = now;
    }

    printf("WATCHDOG: Progress looks good (no deadlock).\n");
    fflush(stdout);
    return NULL;
}

int main() {
    accounts[0] = 1000000;
    accounts[1] = 1000000;

    long long initial_total = accounts[0] + accounts[1];

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_init(&account_lock[i], NULL);
    }

    printf("Phase 4 (DEADLOCK FIX: LOCK ORDERING)\n");
    printf("Accounts start: A0=%lld A1=%lld Total=%lld\n", accounts[0], accounts[1], initial_total);

    pthread_t t1, t2, wd;

    // Reduced loops so it finishes fast but still proves the point
    transfer_args_t a = {0, 1, 100000};
    transfer_args_t b = {1, 0, 100000};

    pthread_create(&wd, NULL, watchdog_thread, NULL);
    pthread_create(&t1, NULL, transfer_thread, &a);
    pthread_create(&t2, NULL, transfer_thread, &b);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(wd, NULL);

    long long final_total = accounts[0] + accounts[1];

    printf("Accounts end:   A0=%lld A1=%lld Total=%lld\n", accounts[0], accounts[1], final_total);
    printf("Expected total: %lld\n", initial_total);

    if (final_total == initial_total) {
        printf("RESULT: Total correct and no deadlock occurred.\n");
    } else {
        printf("RESULT: Total changed (should not happen here).\n");
    }

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&account_lock[i]);
    }

    return 0;
}
