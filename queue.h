typedef struct { 
    int id;
    int k;
    int min_index;
    int max_index;
    int work;
    pthread_cond_t* ready;
    pthread_mutex_t* mutex;
} slave_t;

typedef struct {
    int max_size;
    int head;
    int tail;
    slave_t** slaves;
    int empty;
    pthread_cond_t* not_empty;
    pthread_mutex_t* mutex;
} queue_t;

slave_t* sinit(int id);
queue_t* qinit(int max_size);

void enqueue(queue_t* q, slave_t* s);
slave_t* dequeue(queue_t* q);

void qfree(queue_t* q);
void sfree(slave_t* s);
