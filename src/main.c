/**
 * @author      : theo (theo.j.lincke@gmail.com)
 * @file        : main
 * @created     : Wednesday Feb 26, 2020 20:30:09 MST
 */

#include "multi-lookup.h"
#include "file-operations.h"
#include <string.h>


int main() {

        // Mutexes:
        pthread_mutex_t buffer_lock;
        pthread_mutex_t producer_log_file_lock;
        pthread_mutex_t consumer_log_file_lock;
        pthread_mutex_t consumer_results_file_lock;
        pthread_mutex_t file_queue_lock;

        pthread_mutex_init(&buffer_lock, NULL);
        pthread_mutex_init(&producer_log_file_lock, NULL);
        pthread_mutex_init(&consumer_log_file_lock, NULL);
        pthread_mutex_init(&consumer_results_file_lock, NULL);
        pthread_mutex_init(&file_queue_lock, NULL);

        sem_t space_available;
        sem_t items_available;

        sem_init(&space_available, 0, BUFFER_SIZE); 
        sem_init(&items_available, 0, 0); 

        // Data structures:
        buffer b = create_buffer(buffer_lock, space_available, items_available);
        file_queue f = create_file_queue(file_queue_lock);
        push_file(&f, "./input/names1.txt");
        push_file(&f, "./input/names2.txt");
        push_file(&f, "./input/names3.txt");
        push_file(&f, "./input/names4.txt");
        push_file(&f, "./input/names5.txt");


        // Open necessary shared files
        FILE* producer_log_file = file_open("./serviced.txt", "w");
        FILE* consumer_log_file = file_open("./consumer_log_file.txt", "w");
        FILE* consumer_results_file = file_open("./results.txt", "w");
        
        file producer_log_file_s = 
                {.file_lock = producer_log_file_lock,
                 .fp = producer_log_file};

        file consumer_log_file_s = 
                {.file_lock = consumer_log_file_lock,
                 .fp = consumer_log_file};

        file consumer_results_file_s = 
                {.file_lock = consumer_results_file_lock,
                 .fp = consumer_results_file};



        pthread_t producers[3];
        pthread_t consumers[3];

        producer_context p = {.log_f = &producer_log_file_s,
                              .f_queue = &f,
                              .buff = &b};

        int status = 1;
        consumer_context c = {.log_f = &consumer_log_file_s,
                              .output_f = &consumer_results_file_s,
                              .buff = &b,
                              .t_status = &status};


        for(int i = 0; i < 3; ++i) {
                pthread_create(&producers[i], NULL, producer_thread, (void*)&p);
        }
        for(int i = 0; i < 3; ++i) {
                pthread_create(&consumers[i], NULL, consumer_thread, (void*)&c);
        }

        for(int i = 0; i < 3; ++i) {
                pthread_join(producers[i], NULL);
        }

        status = 0;

        puts("HEREEEEE\n");

        for(int i = 0; i < 3; ++i) {
                pthread_join(consumers[i], NULL);
        }

        destroy_file_queue(&f);

        file_close(producer_log_file);
        file_close(consumer_log_file);
        file_close(consumer_results_file);

        pthread_mutex_destroy(&buffer_lock);
        pthread_mutex_destroy(&producer_log_file_lock);
        pthread_mutex_destroy(&consumer_log_file_lock);
        pthread_mutex_destroy(&consumer_results_file_lock);
        pthread_mutex_destroy(&file_queue_lock);

        sem_destroy(&space_available);
        sem_destroy(&items_available);

        return 1;
}
