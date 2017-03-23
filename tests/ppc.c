#include "csapp.h"
#include "utils.h"

// the size of the buffer
#define N 100
// the number of producers
#define NP 100
// the number of consumers
#define NC 100

#define ITER 1000000

int producer_shared_counter = ITER;
int consumer_shared_counter = ITER;

void *producer(void *vargp) {
    struct producer_consumer *pc = (struct producer_consumer *)vargp;
    while(1) {
        int iter = __sync_fetch_and_sub(&producer_shared_counter, 1);
        if(iter <= 0)
            break;
        int *p = Malloc(sizeof(int));
        *p = rand();
        produce(pc, p);
        printf("p %d\n", *p);
    }
    return NULL;
}

void *consumer(void *vargp) {
    struct producer_consumer *pc = (struct producer_consumer *)vargp;
    while(1) {
        int iter = __sync_fetch_and_sub(&consumer_shared_counter, 1);
        if(iter <= 0)
            break;
        int *ret = consume(pc);
        printf("c %d\n", *ret);
    }
    return NULL;
}

int main() {
    int i;
    srand(time(NULL));
    struct producer_consumer pc;
    producer_consumer_init(&pc, N);

    pthread_t *producers = (pthread_t *)Malloc(NP * sizeof(pthread_t)); 
    pthread_t *consumers = (pthread_t *)Malloc(NC * sizeof(pthread_t)); 
    // create producers
    for (i = 0; i < NP; i++)
        Pthread_create(&producers[i], NULL, producer, &pc);

    // create consumers
    for (i = 0; i < NC; i++)
        Pthread_create(&consumers[i], NULL, consumer, &pc);

    for (i = 0; i < NP; i++)
        Pthread_join(producers[i], NULL);
    for (i = 0; i < NC; i++)
        Pthread_join(consumers[i], NULL);

    producer_consumer_destroy(&pc);
    return 0;
}
