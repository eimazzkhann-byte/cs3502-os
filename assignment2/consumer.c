// Name: Eimaz Khan
// CS 3502 - Assignment 2
// consumer.c - Consumer

#include "buffer.h"

// Globals for cleanup
shared_buffer_t* buffer = NULL;
sem_t* mutex = SEM_FAILED;
sem_t* empty = SEM_FAILED;
sem_t* full  = SEM_FAILED;
int shm_id = -1;

static int consumer_id_g = -1;

void cleanup(void) {
    if (buffer != NULL) {
        shmdt(buffer);
        buffer = NULL;
    }

    if (mutex != SEM_FAILED) sem_close(mutex);
    if (empty != SEM_FAILED) sem_close(empty);
    if (full  != SEM_FAILED) sem_close(full);
}

void signal_handler(int sig) {
    printf("\nConsumer %d: Caught signal %d, cleaning up...\n", consumer_id_g, sig);
    cleanup();
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <consumer_id> <num_items>\n", argv[0]);
        return 1;
    }

    int consumer_id = atoi(argv[1]);
    int num_items   = atoi(argv[2]);
    consumer_id_g = consumer_id;

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    srand((unsigned int)time(NULL) ^ (unsigned int)(consumer_id * 97531u));

    // Attach shared memory (consumer never creates it)
    shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), 0666);
    if (shm_id == -1) {
        perror("shmget (attach) failed (did you start producer 0 first?)");
        return 1;
    }

    buffer = (shared_buffer_t*)shmat(shm_id, NULL, 0);
    if (buffer == (void*)-1) {
        perror("shmat failed");
        buffer = NULL;
        cleanup();
        return 1;
    }

    // Open semaphores (consumer never creates)
    mutex = sem_open(SEM_MUTEX, 0);
    empty = sem_open(SEM_EMPTY, 0);
    full  = sem_open(SEM_FULL,  0);

    if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
        perror("Consumer: sem_open failed (did you start producer 0 first?)");
        cleanup();
        return 1;
    }

    printf("Consumer %d: Starting to consume %d items\n", consumer_id, num_items);

    for (int i = 0; i < num_items; i++) {
        // Wait for item available
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

        usleep((useconds_t)(rand() % 100000));
    }

    printf("Consumer %d: Finished consuming %d items\n", consumer_id, num_items);
    cleanup();
    return 0;
}
