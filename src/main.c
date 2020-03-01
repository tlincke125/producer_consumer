/**
 * @author      : theo (theo.j.lincke@gmail.com)
 * @file        : main
 * @created     : Wednesday Feb 26, 2020 20:30:09 MST
 */

#include <sys/time.h>
#include "file-operations.h"
#include "multi-lookup.h"


#define DEFAULT_REQUESTER_LOG "./serviced.txt"
#define DEFAULT_RESOLVER_LOG "./results.txt"


const char * help = "\nUSAGE:         \n\n"
"       multi-lookup <# requester> <# resolver> <requester log> <resolver log> [<data file i> ... ]\n\n";

double what_time_is_it()
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec + now.tv_nsec*1e-9;
}

int main(int argc, const char** argv) {

        if(argc < 6) {
                printf("%s\n", help);
                return 1;
        }
        
        
        const size_t num_requester_threads = atoi(argv[1]);
        const size_t num_resolver_threads = atoi(argv[2]);
        
        if(num_requester_threads == 0 || num_resolver_threads == 0) {
                printf("%s\n", help);
                return 1;
        }

        const char* req_log = argv[3];
        const char* res_log = argv[4];

        if(!file_exists(req_log) || !file_exists(res_log)) {
                fprintf(stderr, "Please enter a valid log file\n");
                return 1;
        }


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

        for(int i = 5; i < argc; ++i) {
                push_file(&f, argv[i]);
        }



        // Open necessary shared files
        FILE* producer_log_file = file_open(req_log, "w");
        FILE* consumer_log_file = file_open("./consumer_log_file.txt", "w");
        FILE* consumer_results_file = file_open(res_log, "w");
        
        file producer_log_file_s = 
                {.file_lock = producer_log_file_lock,
                 .fp = producer_log_file};

        file consumer_log_file_s = 
                {.file_lock = consumer_log_file_lock,
                 .fp = consumer_log_file};

        file consumer_results_file_s = 
                {.file_lock = consumer_results_file_lock,
                 .fp = consumer_results_file};



        pthread_t producers[num_requester_threads];
        pthread_t consumers[num_resolver_threads];

        producer_context p = {.log_f = &producer_log_file_s,
                              .f_queue = &f,
                              .buff = &b};

        int status = 1;
        consumer_context c = {.log_f = &consumer_log_file_s,
                              .output_f = &consumer_results_file_s,
                              .buff = &b,
                              .t_status = &status};



        double start = what_time_is_it();

        for(int i = 0; i < num_requester_threads; ++i) {
                pthread_create(&producers[i], NULL, producer_thread, (void*)&p);
        }
        for(int i = 0; i < num_resolver_threads; ++i) {
                pthread_create(&consumers[i], NULL, consumer_thread, (void*)&c);
        }

        for(int i = 0; i < num_requester_threads; ++i) {
                pthread_join(producers[i], NULL);
        }

        status = 0;

        for(int i = 0; i < num_resolver_threads; ++i) {
                pthread_join(consumers[i], NULL);
        }

        printf("Time spent: %.9f\n", what_time_is_it() - start);

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
