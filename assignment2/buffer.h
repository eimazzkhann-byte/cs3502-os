// Name: Eimaz Khan
// CS 3502 - Assignment 2
// buffer.h - Shared definitions

#ifndef BUFFER_H
#define BUFFER_H

// Required includes for both producer and consumer
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Constants for shared memory and semaphores
#define BUFFER_SIZE 10
#define SHM_KEY 0x1234

// Named semaphores (must start with '/')
#define SEM_MUTEX "/sem_mutex"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL  "/sem_full"

// Item stored in the bounded buffer
typedef struct {
    int value;        // The data
    int producer_id;  // Which producer created this item
} item_t;

// Shared buffer structure in shared memory
typedef struct {
    item_t buffer[BUFFER_SIZE];
    int head;   // Next write position (producer)
    int tail;   // Next read position (consumer)
    int count;  // Current number of items
} shared_buffer_t;

#endif
