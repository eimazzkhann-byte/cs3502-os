// Name: Eimaz Khan
// CS 3502 - Assignment 2

#include "buffer.h"

shared_buffer_t* buffer = NULL;
sem_t* mutex = SEM_FAILED;
sem_t* empty = SEM_FAILED;
sem_t* full  = SEM_FAILED;
int shm_id = -1;

static void cleanup() {
    if (buffer != NULL) {
        shmdt(buffer);
        buffer = NULL;
    }
    if (mutex != SEM_FAILED) sem_close(mutex);
    if (empty != SEM_FAILED) sem_close(empty);
    if (full  != SEM_FAILED) sem_close(full);
}

static void signal_handler(int sig) {
    printf("\nProducer: Caught signal %d, cleaning up...\n", sig);
    cleanup();
    exit(0);
}

static void attach_shared_memory() {
    shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    buffer = (shared_buffer_t*) shmat(shm_id, NULL, 0);
    if (buffer == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Lazy init (safe enough for this assignment)
    if (buffer->initialized != 1) {
        buffer->head = 0;
        buffer->tail = 0;
        buffer->count = 0;
        buffer->initialized = 1;
    }
}

static void open_semaphores() {
    // Create-or-open named semaphores
    mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    if (mutex == SEM_FAILED) { perror("sem_open mutex"); exit(1); }

    empty = sem_open(SEM_EMPTY, O_CREAT, 0666, BUFFER_SIZE);
    if (empty == SEM_FAILED) { perror("sem_open empty"); exit(1); }

    full = sem_open(SEM_FULL, O_CREAT, 0666, 0);
    if (full == SEM_FAILED) { perror("sem_open full"); exit(1); }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <producer_id> <num_items>\n", argv[0]);
        return 1;
    }

    int producer_id = atoi(argv[1]);
    int num_items   = atoi(argv[2]);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    srand(time(NULL) + producer_id);

    attach_shared_memory();
    open_semaphores();

    printf("Producer %d: Starting to produce %d items\n", producer_id, num_items);

    for (int i = 0; i < num_items; i++) {
        item_t item;
        item.value = producer_id * 1000 + i;
        item.producer_id = producer_id;

        sem_wait(empty);
        sem_wait(mutex);

        buffer->buffer[buffer->head] = item;
        buffer->head = (buffer->head + 1) % BUFFER_SIZE;
        buffer->count++;

        printf("Producer %d: Produced value %d\n", producer_id, item.value);

        sem_post(mutex);
        sem_post(full);

        usleep(rand() % 100000);
    }

    printf("Producer %d: Finished producing %d items\n", producer_id, num_items);
    cleanup();
    return 0;
}
