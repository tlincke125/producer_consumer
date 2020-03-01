/**
 * @author      : theo (theo.j.lincke@gmail.com)
 * @file        : multi-lookup
 * @created     : Wednesday Feb 26, 2020 20:57:21 MST
 */

#include <pthread.h>
#include "multi-lookup.h"
#include "file-operations.h"


/*
 * This isn't technically necessary
 * but it helps loads with readability
 *
 * this is just all of the information the
 * producer thread has access to
 */
typedef struct {
        file* f_log;
        file_queue* queue;
        buffer* buff;

        // The file the thread is in charge of
        char file_name[256];
} producer_thread_space;


typedef struct {
        file* log_f;
        file* output_f;

        buffer* buff;

        const int * const t_status;

        // The file the thread is in charge of
        char parse_name[MAX_NAME_LENGTH];
        char ip_address[MAX_IP_LENGTH];
} consumer_thread_space;


/*
 * Creates a new producer thread space from a context
 */
#define NEW_PRODUCER_TS(prod_cont_ptr) \
        (producer_thread_space) \
        {.f_log = ((producer_context*)prod_cont_ptr)->log_f, \
         .queue = ((producer_context*)prod_cont_ptr)->f_queue, \
         .buff = ((producer_context*)prod_cont_ptr)->buff}


void* producer_thread(void* p_context) {

        producer_thread_space p = NEW_PRODUCER_TS(p_context);
        pthread_t tid = pthread_self();
        
        int serviced = 0;
        while(1) {

                // First get data from the file queue
                {
                        pthread_mutex_lock(&p.queue->file_queue_lock);

                        // Critical file queue section
                        if(!p.queue->head){

                                pthread_mutex_unlock(&p.queue->file_queue_lock);
                                break;
                        } else {
                                snprintf(p.file_name, p.queue->head->f_name_size + 1, p.queue->head->filename);
                                pop_file(p.queue);
                                serviced ++;
                        }

                        pthread_mutex_unlock(&p.queue->file_queue_lock);
                }

                // Second push file data onto the buffer 
                {
                        FILE* fp = file_open(p.file_name, "r");
                        if(fp == NULL){
                                printf("ERROR TID: %ld\n", tid);
                                break;
                        }

                        char buffer[MAX_NAME_LENGTH];
                        while(file_gets(buffer, MAX_NAME_LENGTH, fp)) {

                                // Wait until there's space available
                                sem_wait(&p.buff->space_available);

                                pthread_mutex_lock(&p.buff->buffer_lock);

                                // Remove \n and \t
                                strip(buffer);
                                push_buffer_element(p.buff, buffer);

                                pthread_mutex_unlock(&p.buff->buffer_lock);

                                // Let them know there are new items available
                                sem_post(&p.buff->items_available);
                        }

                        file_close(fp);
                }
        } 


        pthread_mutex_lock(&p.f_log->file_lock);

        char resp[256];
        snprintf(resp, 256, "Thread: %ld serviced %d files\n", tid, serviced);
        file_puts(resp, p.f_log->fp);

        pthread_mutex_unlock(&p.f_log->file_lock);

        return NULL;
}


/*
 * Creates a new consumer thread space from a context
 */
#define NEW_CONSUMER_TS(prod_cont_ptr) \
        (consumer_thread_space) \
        {.log_f = ((consumer_context*)prod_cont_ptr)->log_f, \
         .output_f = ((consumer_context*)prod_cont_ptr)->output_f, \
         .buff = ((consumer_context*)prod_cont_ptr)->buff, \
         .t_status = ((consumer_context*)prod_cont_ptr)->t_status}

void* consumer_thread(void* c_context) {
        pthread_t tid = pthread_self();
        consumer_thread_space c = NEW_CONSUMER_TS(c_context);

        int serviced = 0;
        while(1) {
                {
                        printf("%ld\n", c.buff->size);
                        if(!c.t_status) {
                                if(c.buff->size <= 0){
                                        printf("HERE\n");
                                        pthread_mutex_unlock(&c.buff->buffer_lock);
                                        break;
                                }
                        }

                        // Wait until items are available
                        sem_wait(&c.buff->items_available);
                        
                        pthread_mutex_lock(&c.buff->buffer_lock);
                        
                        // Operate on the buffer
                        pop_buffer_element(c.buff, c.parse_name);

                        pthread_mutex_unlock(&c.buff->buffer_lock);

                        sem_post(&c.buff->space_available);
                }
                {
                        printf("busy\n");
                        int ret = dnslookup(c.parse_name, c.ip_address, MAX_IP_LENGTH);
                        printf("done\n");
                        
                        if(ret != UTIL_SUCCESS){
                                printf("ERROR Thread"); 
                        }

                        pthread_mutex_lock(&c.output_f->file_lock);

                        file_puts(c.parse_name, c.output_f->fp);
                        file_puts(",", c.output_f->fp);
                        file_puts(c.ip_address, c.output_f->fp);
                        file_puts("\n", c.output_f->fp);

                        pthread_mutex_unlock(&c.output_f->file_lock);

                        serviced ++;
                }

        }

        pthread_mutex_lock(&c.log_f->file_lock);

        char resp[256];
        snprintf(resp, 256, "Thread: %ld serviced %d files\n", tid, serviced);
        file_puts(resp, c.log_f->fp);

        pthread_mutex_unlock(&c.log_f->file_lock);

        return NULL;
}



file_queue create_file_queue(pthread_mutex_t f_queue_lock) {
        file_queue q;
        q.file_queue_lock = f_queue_lock;
        q.size = 0;
        q.head = NULL;
        return q;
}

void push_file(file_queue* f_queue, const char* filename) {
        
        file_node* node = (file_node*)malloc(sizeof(file_node));
        node->f_name_size = strlen(filename);
        strncpy(node->filename, filename, node->f_name_size + 1);

        f_queue->size ++;

        if(f_queue->head == NULL) {
                node->next_file = NULL;
                f_queue->head = node;
                return;
        } else {
                node->next_file = f_queue->head;
                f_queue->head = node;
                return;
        }
}

void pop_file(file_queue* f_queue) {
        if(f_queue && f_queue->head != NULL) {
                f_queue->size --;
                file_node* temp = f_queue->head;
                f_queue->head = f_queue->head->next_file;
                free(temp);
        }
}

void destroy_file_queue(file_queue* f_queue) {
        file_node* temp = f_queue->head;

        if(temp == NULL)
                return;

        while(f_queue->head->next_file != NULL) {
                temp = f_queue->head;
                f_queue->head = f_queue->head->next_file;
                free(temp);
        }
        free(f_queue->head);
}

void print_files(file_queue* f_queue) {
        file_node* temp = f_queue->head;
        while(temp != NULL) {
                printf("%s\n", temp->filename);
                temp = temp->next_file;
        }
}


buffer create_buffer(pthread_mutex_t buffer_lock, sem_t sa, sem_t ia) {
        buffer b = (buffer){0};

        b.buffer_lock = buffer_lock;
        b.space_available = sa;
        b.items_available = ia;

        b.size = 0;
        b.starting_index = 0;
        b.ending_index = 0;
        return b;
}

int push_buffer_element(buffer* buff, const char* str_element) {
        printf("%ld %ld %ld\n", buff->starting_index, buff->ending_index, buff->size);
        if(buff->size >= BUFFER_SIZE) {
                /** printf("overflow\n"); */
                return -1;
        }

        size_t len = strlen(str_element) + 1;

        if(len > MAX_NAME_LENGTH)
                len = MAX_NAME_LENGTH;

        snprintf(buff->data[buff->ending_index], len, str_element);

        buff->ending_index = (buff->ending_index + 1) % BUFFER_SIZE;
        buff->size ++;

        return 1;
}

int pop_buffer_element(buffer* buff, char destination[MAX_NAME_LENGTH]) {
        if(buff->size <= 0) {
                return -1;
        }

        snprintf(destination, strlen(buff->data[buff->starting_index]) + 1, buff->data[buff->starting_index]);

        buff->starting_index = (buff->starting_index + 1) % BUFFER_SIZE;
        buff->size -= 1;

        return 1;
}

void print_buffer(buffer* buff) {
        int i = buff->starting_index;
        int s = buff->size;

        for(int x = 0; x < s; ++x) {
                printf("%s\n", buff->data[i]);
                i = (i + 1) % BUFFER_SIZE;
        }
}
