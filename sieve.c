/* 
 * COP4520 Homework 3
 * Pthread Sieve of Eratosthenese
 *
 * By Jake Lopez
 * PID 5820326
 *
 * usage: sieve [num_slaves] [max_number] [chunk_size]
 */
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

queue_t* readyq;

// struct for passing args to the producer function
typedef struct {
    int max_number;
    int chunk_size;
    int num_slaves;
} master_t;


slave_t* sinit(int id)
{
    slave_t* s = (slave_t*) malloc(sizeof(slave_t));
    s->id = id;
    s->k = 0;
    s->min_index = -1;
    s->max_index = -1;
    s->work = 0;

    s->ready = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(s->ready, NULL);

    s->mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(s->mutex, NULL);

    return s;
}

void sfree(slave_t* s)
{
    pthread_cond_destroy(s->ready);
    pthread_mutex_destroy(s->mutex);

    free(s->ready);
    free(s->mutex);
    free(s);
}

void* producer(void* args)
{
    master_t* m = (master_t*)args;
    int check_till = (int)sqrt(m->max_number);

    for(int i = 2; i <= check_till; i++)
    {
        int k = i*i;
        while(k <= m->max_number)
        {
            pthread_mutex_lock(readyq->mutex);
            while(readyq->empty)
                pthread_cond_wait(readyq->not_empty, readyq->mutex);
            pthread_mutex_unlock(readyq->mutex);

            pthread_mutex_lock(readyq->mutex);
            slave_t* s = dequeue(readyq);
            pthread_mutex_lock(s->mutex);
            printf("[MASTER] sending %d %d\n", s->id, k);
            s->work = 1;
            s->k = k;
            pthread_cond_signal(s->ready);
            k += k;
            pthread_mutex_unlock(s->mutex);
            pthread_mutex_unlock(readyq->mutex);
        }
    }

    for(int i = 0; i < m->num_slaves; i++)
    {

        pthread_mutex_lock(readyq->mutex);
        while(readyq->empty)
            pthread_cond_wait(readyq->not_empty, readyq->mutex);
        slave_t* s = dequeue(readyq);
        pthread_mutex_unlock(readyq->mutex);
        pthread_mutex_lock(s->mutex);
        s->work = 1;
        s->k = -1;
        pthread_cond_signal(s->ready);
        printf("[MASTER] Sending %d kill signal.\n", s->id);
        pthread_mutex_unlock(s->mutex);
    }

    return (void*)NULL;
}

void* consumer(void* args)
{
    slave_t* s = (slave_t*)args;
    while(1)
    { 
        pthread_mutex_lock(s->mutex);
        printf("[%d] waiting for work...\n", s->id);
        while(!s->work)
            pthread_cond_wait(s->ready, s->mutex);
        pthread_mutex_unlock(s->mutex);

        // "kill" signal from producer thread
        if(s->k < 0)
        {
            printf("[%d] Got kill signal.\n", s->id);
            break;
        }
        else
            printf("[%d] got %d\n", s->id, s->k);

        pthread_mutex_lock(s->mutex);
        s->work = 0;
        pthread_mutex_unlock(s->mutex);

        // requeue and signal producer that we're good for more work
        pthread_mutex_lock(readyq->mutex);
        enqueue(readyq, s);
        pthread_cond_signal(readyq->not_empty);
        pthread_mutex_unlock(readyq->mutex);
    }
    return (void*)NULL;
}

int main(int argc, char** argv)
{
    if(argc != 4)
    {
        fprintf(stderr, "usage: sieve [num_slaves] [max_number] [chunk_size]\n");
        exit(1);
    }
    else
    {
        int num_slaves = atoi(argv[1]);
        int max_number = atoi(argv[2]);
        int chunk_size = atoi(argv[3]);

        pthread_t master;
        pthread_t slaves[num_slaves];

        slave_t** slaveargs;
        master_t* masterargs;

        readyq = qinit(num_slaves);

        masterargs = (master_t*)malloc(sizeof(master_t));

        masterargs->max_number = max_number;
        masterargs->chunk_size = chunk_size;
        masterargs->num_slaves = num_slaves;

        slaveargs = (slave_t**)malloc(sizeof(slave_t*) * num_slaves);
        for(int i = 0; i < num_slaves; i++)
        {
            slaveargs[i] = sinit(i);
            pthread_create(&slaves[i], NULL, (void*) consumer, (void*) slaveargs[i]);
        }
        readyq->slaves = slaveargs;

        for(int i = 0; i < num_slaves; i++)
            enqueue(readyq, slaveargs[i]);

        pthread_create(&master, NULL, (void*) producer, (void*) masterargs);
        // cleaning up
        for(int i = 0; i < num_slaves; i++)
            pthread_join(slaves[i], NULL);
        pthread_join(master, NULL);

        for(int i = 0; i < num_slaves; i++)
            sfree(slaveargs[i]);

        qfree(readyq);
    }
    return 0;
}
