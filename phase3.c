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

// One mutex per account (this is what allows deadlock)
pthread_mutex_t account_lock[NUM_ACCOUNTS];

// Used to detect “no progress”
volatile long long progress_counter = 0;

// Deadlocking transfer: lock from, then lock to (NO ordering rule)
void transfer_deadlock(int from, int to, int amount) {
    pthread_mutex_lock(&account_lock[from]);

    // tiny delay so the other thread can grab the other lock
    usleep(5000);

    pthread_mutex_lock(&account_lock[to]);

    // critical section
    accounts[from] -= amount;
    accounts[to] += amount;

    progress_counter++;

    pthread_mutex_unlock(&account_lock[to]);
    pthread_mutex_unlock(&account_lock[from]);
}

void* transfer_thread(void* arg) {
    transfer_args_t* t = (transfer_args_t*)arg;

    printf("Thread started: trying transfers %d -> %d\n", t->from, t->to);
    fflush(stdout);

    for (int i = 0; i < t->loops; i++) {
        transfer_deadlock(t->from, t->to, TRANSFER_AMOUNT);

        if ((i % 1000) == 0) {
            usleep(1000);
        }
    }

    printf("Thread finished: %d -> %d\n", t->from, t->to);
    return NULL;
}

// Watchdog: if progress stops increasing, deadlock probably happened
void* watchdog_thread(void* arg) {
    (void)arg;
    long long last = progress_counter;

    while (1) {
        sleep(2);
        long long now = progress_counter;

        if (now == last) {
            printf("\nWATCHDOG: No progress for 2 seconds.\n");
            printf("WATCHDOG: Deadlock is very likely happening (this is expected in Phase 3).\n\n");
            fflush(stdout);
            return NULL;
        }

        last = now;
    }
}

int main() {
    accounts[0] = 1000000;
    accounts[1] = 1000000;

    long long initial_total = accounts[0] + accounts[1];

    // init locks
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_init(&account_lock[i], NULL);
    }

    printf("Phase 3 (DEADLOCK ON PURPOSE)\n");
    printf("Accounts start: A0=%lld A1=%lld Total=%lld\n", accounts[0], accounts[1], initial_total);

    pthread_t t1, t2, wd;

    // Thread A locks 0 then 1
    transfer_args_t a = {0, 1, 1000000};
    // Thread B locks 1 then 0 (opposite order)
    transfer_args_t b = {1, 0, 1000000};

    pthread_create(&wd, NULL, watchdog_thread, NULL);
    pthread_create(&t1, NULL, transfer_thread, &a);
    pthread_create(&t2, NULL, transfer_thread, &b);

    // These likely never finish due to deadlock
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(wd, NULL);

    long long final_total = accounts[0] + accounts[1];
    printf("Accounts end:   A0=%lld A1=%lld Total=%lld\n", accounts[0], accounts[1], final_total);
    printf("Expected total: %lld\n", initial_total);

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&account_lock[i]);
    }

    return 0;
}
