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
        // NOTE: shared memory segmen
