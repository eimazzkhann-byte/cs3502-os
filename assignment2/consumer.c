// Name: Eimaz Khan
// CS 3502 - Assignment 2
// consumer.c - Consumer

#include "buffer.h"

static shared_buffer_t* buffer = NULL;
static sem_t* mutex = SEM_FAILED;
static sem_t* empty = SEM_FAILED;
static sem_t* full  = SEM_FAILED;
static int shm_id = -1;

void cleanup() {
    if (buffer != NULL) {
        shmdt(buffer);
        buffer = NULL;
    }

    if (mutex != SEM_FAILED) { sem_close(mutex); mutex = SEM_FAILED; }
    if (empty != SEM_FAILED) { sem_close(empty); empty = SEM_FAILED; }
    if (full  != SEM_FAILED) { sem_close(full);  full  = SEM_FAILED; }
}

void signal_handler(int sig) {
    printf("\nConsumer: Caught signal %d, cleaning up...\n", sig);
    cleanup();
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <consumer_id> <num_items>\n", argv[0]);
        exit(1);
    }

    int consumer_id = atoi(argv[1]);
    int num_items = atoi(argv[2]);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    srand(time(NULL) + consumer_id * 100);

    // Attach to existing shared memory (producer 0 must create first)
    shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), 0666);
    if (shm_id < 0) {
        perror("shmget failed (did you start producer 0 first?)");
        exit(1);
    }

    buffer = (shared_buffer_t*) shmat(shm_id, NULL, 0);
    if (buffer == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Open existing semaphores (no O_CREAT here)
    mutex = sem_open(SEM_MUTEX, 0);
    empty = sem_open(SEM_EMPTY, 0);
    full  = sem_open(SEM_FULL,  0);

    if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
        perror("sem_open failed");
        cleanup();
        exit(1);
    }

    printf("Consumer %d: Starting to consume %d items\n", consumer_id, num_items);

    for (int i = 0; i < num_items; i++) {
        // Wait for a full slot
        sem_wait(full);

        // Enter critical section
        sem_wait(mutex);

        // Remove item
        item_t item = buffer->buffer[buffer->tail];
        buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
        buffer->count--;

        printf("Consumer %d: Consumed value %d from Producer %d (count=%d)\n",
               consumer_id, item.value, item.producer_id, buffer->count);

        // Exit critical section
        sem_post(mutex);

        // Signal empty slot
        sem_post(empty);

        usleep(rand() % 100000);
    }

    printf("Consumer %d: Finished consuming %d items\n", consumer_id, num_items);
    cleanup();
    return 0;
}
