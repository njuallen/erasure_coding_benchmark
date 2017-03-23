#include "csapp.h"
#include "adt.h"
#include "utils.h"

void producer_consumer_init(struct producer_consumer *pc, int size) {
    queue_init(&pc->q, size);
    Pthread_mutex_init(&pc->mutex, NULL);
    Pthread_cond_init(&pc->condc, NULL);
    Pthread_cond_init(&pc->condp, NULL);
}

void producer_consumer_destroy(struct producer_consumer *pc) {
    queue_destroy(&pc->q);
    Pthread_cond_destroy(&pc->condc);
    Pthread_cond_destroy(&pc->condp);
    Pthread_mutex_destroy(&pc->mutex);
}

void produce(struct producer_consumer *pc, void *e) {

    pthread_mutex_t *m = &pc->mutex;
    struct queue *q = &pc->q;

    // get exclusive access to buffer
    Pthread_mutex_lock(m); 

    while (queue_is_full(q))
        Pthread_cond_wait(&pc->condp, m);
    enqueue(q, e);

    // wake up consumer
    Pthread_cond_signal(&pc->condc); 

    // release access to buffer
    Pthread_mutex_unlock(m); 
}

void *consume(struct producer_consumer *pc) {
    pthread_mutex_t *m = &pc->mutex;
    struct queue *q = &pc->q;

    // get exclusive access to buffer
    Pthread_mutex_lock(m); 

    while (queue_is_empty(q))
        Pthread_cond_wait(&pc->condc, m);

    // take item out of buffer
    void *ret = dequeue(q);

    // wake up producer
    Pthread_cond_signal(&pc->condp); 
    Pthread_mutex_unlock(m);
    return ret;
}
