#include <pthread.h>
#include <stdlib.h>
#include "queue.h"

queue_t* qinit(int max_size)
{
    queue_t* q = (queue_t*)malloc(sizeof(queue_t));

    q->max_size = max_size;
    q->head = 0;
    q->tail = 0;
    q->empty = 1;
    q->not_empty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->not_empty, NULL);

    q->mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mutex, NULL);

    return q;
}

void enqueue(queue_t* q, slave_t* s)
{
    q->slaves[q->tail++] = s;
    if(q->tail == q->max_size)
        q->tail = 0;
    q->empty = 0;
}

slave_t* dequeue(queue_t* q)
{
    slave_t* s = q->slaves[q->head++];

    if(q->head == q->max_size)
        q->head = 0;
    if(q->head == q->tail)
        q->empty = 1;

    return s;
}

void qfree(queue_t* q)
{
    pthread_cond_destroy(q->not_empty);
    pthread_mutex_destroy(q->mutex);
    free(q->not_empty);
    free(q->mutex);
    free(q);
}
