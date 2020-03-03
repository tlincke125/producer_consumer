/**
 * @author      : theo (theo.j.lincke@gmail.com)
 * @file        : multi-lookup
 * @created     : Wednesday Feb 26, 2020 18:39:34 MST
 */

#ifndef MULTI_LOOKUP_H

#define MULTI_LOOKUP_H

#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>

#include "util.h"

#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MAX_REQUESTER_THREADS 5
#define MAX_NAME_LENGTH 100
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define BUFFER_SIZE 200


////////////////// SOME USEFULL DATA STRUCTURES ////////////////////////
/*
 * A linked list queue of file names
 */
typedef struct file_node_s file_node;
struct file_node_s {
        file f;
        size_t f_name_size;
        char filename[256];
        file_node* next_file;
};

// The file queue wrapper
typedef struct {
        pthread_mutex_t file_queue_lock;
        size_t size;
        file_node* head;
} file_queue;

// File queue methods
file_queue create_file_queue(pthread_mutex_t f_queue_lock);
void push_file(file_queue* f_queue, const char* filename);
void pop_file(file_queue* node);
void destroy_file_queue(file_queue* q);
void print_files(file_queue* q);


/*
 * A shared file type
 * every file has a mutex and a pointer
 * if the mutex is locked, it is busy 
 * (so an open and close needs to complete
 * within the scope of a mutex lock / unlock)
 *
 * NOTE - This is NOT used for files the requesters
 * are requesting. They don't need mutex locks as their
 * files are isolated
 *
 * This is mainly for the shared log files
 */
typedef struct {
        pthread_mutex_t file_lock;
        FILE* fp;
} file;

/*
 * A circular buffer type
 * This is a circular fifo queue with
 * indexes indexing at the current position 
 * and last position
 */
typedef struct {
        size_t size;
        size_t starting_index;
        size_t ending_index;

        char data[BUFFER_SIZE][MAX_NAME_LENGTH];
        pthread_mutex_t buffer_lock;

        // To signal the producer / consumer
        sem_t space_available;
        sem_t items_available;
} buffer;

// Cirular buffer methods
buffer create_buffer(pthread_mutex_t buffer_lock, sem_t sa, sem_t ia);
int push_buffer_element(buffer* buff, const char* str_element);
int pop_buffer_element(buffer* buff, char destination[MAX_NAME_LENGTH]);
void print_buffer(buffer* buff);


/*
 * A producer context is what is passed in order
 * to create a producer thread
 */
typedef struct {
        file* log_f;
        file_queue* f_queue;
        buffer* buff;
} producer_context;

void* producer_thread(void* p_context);

typedef struct {
        file* log_f;
        file* output_f;
        buffer* buff;
        const int* const t_status;
} consumer_context;

void* consumer_thread(void* c_context);

#endif /* end of include guard MULTI_LOOKUP_H */
