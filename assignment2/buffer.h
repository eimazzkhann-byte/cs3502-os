// Name: Eimaz Khan
// CS 3502 - Assignment 2
// buffer.h - Shared definitions

#ifndef BUFFER_H
#define BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define BUFFER_SIZE 10
#define SHM_KEY 0x1234

// Named semaphores must start with '/'
#define SEM_MUTEX "/sem_mutex"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL  "/sem_full"

typedef struct {
    int value;
    int producer_id;
} item_t;

typedef struct {
    item_t buffer[BUFFER_SIZE];
    int head;   // next write index
    int tail;   // next read index
    int count;  // number of items in buffer
} shared_buffer_t;

#endif
