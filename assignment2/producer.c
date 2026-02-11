// Name: Eimaz Khan
// CS 3502 - Assignment 2
// producer.c - Producer

#include "buffer.h"

// Globals for cleanup
shared_buffer_t* buffer = NULL;
sem_t* mutex = SEM_FAILED;
sem_t* empty = SEM_FAILED;
sem_t* full  = SEM_FAILED;
int shm_id = -1;

static int producer_id_g = -1;

void cleanup(int is_creator) {
    if (buffer != NULL) {
        shmdt(buffer);
        buffer = NULL;
    }

    if (mutex != SEM_FAILED) sem_close(mutex);
    if (empty != SEM_FAILED) sem_close(empty);
    if (full  != SEM_FAILED) sem_close(full);

    // Only producer 0 should destroy shared resources
    if (is_creator) {
        sem_unlink(SEM_MUTEX);
        sem_unlink(SEM_EMPTY);
        sem_unlink(SEM_FULL);

        if (shm_id != -1) {
            shmctl(shm_id, IPC_RMID, NULL);
        }
    }
}

void signal_handler(int sig) {
    printf("\nProducer %d: Caught signal %d, cleaning up...\n", producer_id_g, sig);
    cleanup(producer_id_g == 0);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <producer_id> <num_items>\n", argv[0]);
        return 1;
    }

    int producer_id = atoi(argv[1]);
    int num_items   = atoi(argv[2]);
    producer_id_g = producer_id;

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    srand((unsigned int)time(NULL) ^ (unsigned int)(producer_id * 2654435761u));

    int is_creator = (producer_id == 0);

    // --- Shared memory ---
    if (is_creator) {
        shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), 0666 | IPC_CREAT);
        if (shm_id == -1) {
            perror("Producer 0: shmget(create) failed");
            return 1;
        }
    } else {
        shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), 0666);
        if (shm_id == -1) {
            perror("shmget (attach) failed (did you start producer 0 first?)");
            return 1;
        }
    }

    buffer = (shared_buffer_t*)shmat(shm_id, NULL, 0);
    if (buffer == (void*)-1) {
        perror("shmat failed");
        buffer = NULL;
        cleanup(is_creator);
        return 1;
    }

    // --- Semaphores ---
    if (is_creator) {
        // Clean slate in case they already exist from a crash
        sem_unlink(SEM_MUTEX);
        sem_unlink(SEM_EMPTY);
        sem_unlink(SEM_FULL);

        mutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL, 0666, 1);
        empty = sem_open(SEM_EMPTY, O_CREAT | O_EXCL, 0666, BUFFER_SIZE);
        full  = sem_open(SEM_FULL,  O_CREAT | O_EXCL, 0666, 0);

        if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
            perror("Producer 0: sem_open(create) failed");
            cleanup(is_creator);
            return 1;
        }

        // Initialize shared buffer state
        buffer->head = 0;
        buffer->tail = 0;
        buffer->count = 0;
    } else {
        mutex = sem_open(SEM_MUTEX, 0);
        empty = sem_open(SEM_EMPTY, 0);
        full  = sem_open(SEM_FULL,  0);

        if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
            perror("Producer: sem_open(attach) failed (did you start producer 0 first?)");
            cleanup(is_creator);
            return 1;
        }
    }

    printf("Producer %d: Starting to produce %d items\n", producer_id, num_items);

    for (int i = 0; i < num_items; i++) {
        item_t item;
        item.value = producer_id * 1000 + i;
        item.producer_id = producer_id;

        // Wait for empty slot
        sem_wait(empty);

        // Enter critical section
        sem_wait(mutex);

        // Add item
        buffer->buffer[buffer->head] = item;
        buffer->head = (buffer->head + 1) % BUFFER_SIZE;
        buffer->count++;

        printf("Producer %d: Produced value %d (count=%d)\n",
               producer_id, item.value, buffer->count);

        // Exit critical section
        sem_post(mutex);

        // Signal item available
        sem_post(full);

        usleep((useconds_t)(rand() % 100000));
    }

    printf("Producer %d: Finished producing %d items\n", producer_id, num_items);
    cleanup(is_creator);
    return 0;
}
