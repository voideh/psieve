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

#define MIN( a, b) ( a < b ) ? a : b

queue_t* readyq;

// struct for passing args to the producer function
typedef struct {
    int max_number;
    int chunk_size;
    int num_slaves;
} master_t;

// struct for the array of possible primes
typedef struct {
    int value;
    int is_prime; // 0 = false, 1 = true
} prime_t;

prime_t** primes;
pthread_mutex_t mutex; // mutex for prime array


/* 
 * Initiailizes a pointer to a slave_t which is a struct for passing messages
 * between the master & slave.
 */
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

// Cleanup for the slave_t struct
void sfree(slave_t* s)
{
    pthread_cond_destroy(s->ready);
    pthread_mutex_destroy(s->mutex);

    free(s->ready);
    free(s->mutex);
    free(s);
}

/*
 * Producer which is ran from the master thread.
 * Performs the outerloop of the sieve.
 *
 * Then breaks range of k = i*i..N where N = max_number
 * into chunk_size intervals and distributes the work to the 
 * slave threads.
 */
void* producer(void* args)
{
    master_t* m = (master_t*)args;
    int check_till = (int)sqrt(m->max_number);

    for(int i = 2; i <= check_till; i++)
    {
        pthread_mutex_lock(&mutex);
        if(primes[i-1]->is_prime)
        {
            int inc_by;
            pthread_mutex_unlock(&mutex);
            int k = i*i;
            int min_index = k-1;
            if( k < m->max_number)
                inc_by = (m->max_number - k) / m->chunk_size;
            else
                inc_by = 1;

            while(min_index < m->max_number)
            {
                pthread_mutex_lock(readyq->mutex);
                // wait for slave to requeue if empty
                while(readyq->empty)
                {
                    pthread_cond_wait(readyq->not_empty, readyq->mutex);
                }

                slave_t* s = dequeue(readyq);
                pthread_mutex_lock(s->mutex);
                s->work = 1;
                s->k = k;
                s->min_index = min_index;
                s->max_index = MIN(((i - ((min_index + inc_by) % i)) + min_index + inc_by - 1), m->max_number);
                if(s->max_index < m->max_number)
                    min_index = s->max_index;
                else
                    min_index = s->max_index;
                pthread_cond_signal(s->ready);
                pthread_mutex_unlock(s->mutex);
                pthread_mutex_unlock(readyq->mutex);
            }
        }
        else
            pthread_mutex_unlock(&mutex);
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
        while(!s->work)
        {
            pthread_cond_wait(s->ready, s->mutex);
        }
        pthread_mutex_unlock(s->mutex);

        // "kill" signal from producer thread
        if(s->k < 0)
            break;


        for(int i = s->min_index; i < s->max_index; i+=s->k)
        {
            pthread_mutex_lock(&mutex);
            primes[i]->is_prime = 0;
            pthread_mutex_unlock(&mutex);
        }

        pthread_mutex_lock(s->mutex);
        s->work = 0;
        pthread_mutex_unlock(s->mutex);
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

        primes = (prime_t**)malloc(sizeof(prime_t*) * (max_number - 1));
        for(int i = 0; i < max_number; i++)
        {
            primes[i] = (prime_t*)malloc(sizeof(prime_t));
            primes[i]->value = i+1;
            primes[i]->is_prime = 1;
        }
        pthread_mutex_init(&mutex, NULL);

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
        pthread_mutex_destroy(&mutex);
        printf("%d.is_prime = %d\n", primes[max_number-1]->value, primes[max_number-1]->is_prime);
        for(int i = 0; i < num_slaves; i++)
            sfree(slaveargs[i]);

        for(int i = 0; i < max_number; i++)
            free(primes[i]);
        free(primes);
        qfree(readyq);
    }
    return 0;
}
