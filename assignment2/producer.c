// Name: Eimaz Khan
// CS 3502 - Assignment 2
// producer.c - Producer

#include "buffer.h"

static shared_buffer_t* buffer = NULL;
static sem_t* mutex = SEM_FAILED;
static sem_t* empty = SEM_FAILED;
static sem_t* full  = SEM_FAILED;
static int shm_id = -1;

// If producer_id == 0, we treat it as "the creator" process.
static int is_creator = 0;

void cleanup() {
    if (buffer != NULL) {
        shmdt(buffer);
        buffer = NULL;
    }

    if (mutex != SEM_FAILED) { sem_close(mutex); mutex = SEM_FAILED; }
    if (empty != SEM_FAILED) { sem_close(empty); empty = SEM_FAILED; }
    if (full  != SEM_FAILED) { sem_close(full);  full  = SEM_FAILED; }

    // Only unlink if we are the creator (so we don't nuke semaphores mid-run)
    if (is_creator) {
        sem_unlink(SEM_MUTEX);
        sem_unlink(SEM_EMPTY);
        sem_unlink(SEM_FULL);
        // NOTE: shared memory segment will remain until removed; many classes don't require shmctl(IPC_RMID)
        // If your prof wants it removed, uncomment this:
        // if (shm_id != -1) shmctl(shm_id, IPC_RMID, NULL);
    }
}

void signal_handler(int sig) {
    printf("\nProducer: Caught signal %d, cleaning up...\n", sig);
    cleanup();
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <producer_id> <num_items>\n", argv[0]);
        exit(1);
    }

    int producer_id = atoi(argv[1]);
    int num_items = atoi(argv[2]);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    srand(time(NULL) + producer_id);

    // Producer 0 creates shared memory + semaphores
    if (producer_id == 0) {
        is_creator = 1;

        // Create shared memory
        shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT | 0666);
        if (shm_id < 0) {
            perror("shmget (create) failed");
            exit(1);
        }

        buffer = (shared_buffer_t*) shmat(shm_id, NULL, 0);
        if (buffer == (void*)-1) {
            perror("shmat failed");
            exit(1);
        }

        // Initialize buffer
        buffer->head = 0;
        buffer->tail = 0;
        buffer->count = 0;

        // Clean up any old semaphores left behind
        sem_unlink(SEM_MUTEX);
        sem_unlink(SEM_EMPTY);
        sem_unlink(SEM_FULL);

        mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
        empty = sem_open(SEM_EMPTY, O_CREAT, 0666, BUFFER_SIZE);
        full  = sem_open(SEM_FULL,  O_CREAT, 0666, 0);

        if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
            perror("sem_open (create) failed");
            cleanup();
            exit(1);
        }
    } else {
        // Other producers: attach/open existing
        shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), 0666);
        if (shm_id < 0) {
            perror("shmget (attach) failed (did you start producer 0 first?)");
            exit(1);
        }

        buffer = (shared_buffer_t*) shmat(shm_id, NULL, 0);
        if (buffer == (void*)-1) {
            perror("shmat failed");
            exit(1);
        }

        mutex = sem_open(SEM_MUTEX, 0);
        empty = sem_open(SEM_EMPTY, 0);
        full  = sem_open(SEM_FULL,  0);

        if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
            perror("sem_open (open existing) failed");
            cleanup();
            exit(1);
        }
    }

    printf("Producer %d: Starting to produce %d items\n", producer_id, num_items);

    for (int i = 0; i < num_items; i++) {
        item_t item;
        item.value = producer_id * 1000 + i;
        item.producer_id = producer_id;

        // Wait for an empty slot
        sem_wait(empty);

        // Enter critical section
        sem_wait(mutex);

        // Add item
        buffer->buffer[buffer->head] = item;
        buffer->head = (buffer->head + 1) % BUFFER_SIZE;
        buffer->count++;

        printf("Producer %d: Produced value %d (count=%d)\n", producer_id, item.value, buffer->count);

        // Exit critical section
        sem_post(mutex);

        // Signal item available
        sem_post(full);

        usleep(rand() % 100000);
    }

    printf("Producer %d: Finished producing %d items\n", producer_id, num_items);
    cleanup();
    return 0;
}
