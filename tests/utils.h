#ifndef __UTILS_H__
#define __UTILS_H__

#include "adt.h"

// simple implementation of producer consumer problem
struct producer_consumer {
	pthread_mutex_t mutex;
	pthread_cond_t condc, condp;
	struct queue q;
};

void producer_consumer_init(struct producer_consumer *pc, int size);
void producer_consumer_destroy(struct producer_consumer *pc);
void produce(struct producer_consumer *pc, void *e);
void *consume(struct producer_consumer *pc);
#endif
