/**
 * @author      : theo (theo.j.lincke@gmail.com)
 * @file        : main
 * @created     : Wednesday Feb 26, 2020 19:05:49 MST
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#define BUFFER_SIZE 100


int buffer[BUFFER_SIZE + 20];
int ind;
char buffer_temp[40];
int n;
int val;
sem_t mutex;


void* msg_func(void* var) {

        // May cause seg fault
        while(ind < BUFFER_SIZE) {

                sem_wait(&mutex);        
                /* Start of Critical Section */

                // A stupid buffer writing thing to show unsafe code
                n = sprintf(buffer_temp, "%d\0%d%d%d%d%d%d%d%d%d%d", ind, ind, ind, ind, ind, ind, ind, ind, ind, ind);
                buffer_temp[n] = '\0';
                sscanf(buffer_temp, "%d", &val);
                buffer[ind] = val;
                ind ++;

                // For a bit of flavor
                val = 102193;
                /* End of critical Section */

                sem_post(&mutex);
        }
}



int Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void*(*start_routine)(void*), void* arg) {
        int ret = pthread_create(thread, attr, start_routine, arg);
        if(ret) {
                fprintf(stderr, "Error, pthrad create returned code: %d\n", ret);
        }
        return ret;
}

int Pthread_mutex_init(pthread_mutex_t* lock_var) {
        int ret = pthread_mutex_init(lock_var, NULL);
        if(ret) {
                fprintf(stderr, "Pthread mutex lock init returned: %d\n", ret);
        }
        return ret;
}


int main() {
        printf("Output: \n");
        ind = 0;
        memset(buffer, 0, sizeof(buffer));

        pthread_t thread1, thread2;
        
        sem_init(&mutex, 0, 1);

        int ret1 = Pthread_create(&thread1, NULL, msg_func, NULL);
        int ret2 = Pthread_create(&thread2, NULL, msg_func, NULL);
        
        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);

        sem_destroy(&mutex);

        printf("Output: \n");
        for(int i = 0; i < BUFFER_SIZE; ++i) {
                printf("%d ", buffer[i]);
        }
        printf("\n");

        printf("\nThese are all the thread messups: \n");
        for(int i = 0; i < BUFFER_SIZE; ++i) {
                if(buffer[i] != i){
                        printf("%d ", buffer[i]);
                }
        }
        printf("\n");

        exit(EXIT_SUCCESS);
}
