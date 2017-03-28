#include "csapp.h"
#include "adt.h"

void queue_init(struct queue *q, int size) {
    if(size <= 0)
        app_error("queue_init: size should be positive");
    q->size = size + 1;
    // initially the circular buffer is empty
    q->head = q->tail = 0;
    q->cnt = 0;
    q->buffer = (void *)Calloc(size + 1, sizeof(void *));
}

void enqueue(struct queue *q, void *e) {
	if(queue_is_full(q))
		app_error("enqueue: queue overflow");
	q->buffer[q->tail] = e;
	q->tail++;
	q->tail %= q->size;
    q->cnt++;
}

int queue_is_full(struct queue *q) {
	return (q->tail + 1) % q->size == q->head;
}

int queue_is_empty(struct queue *q) {
	return q->head == q->tail;
}

void *dequeue(struct queue *q) {
	if(queue_is_empty(q))
		app_error("dequeue: queue underflow");
	void *ret = q->buffer[q->head];
	q->head++;
	q->head %= q->size;
    q->cnt--;
	return ret;
}

void queue_destroy(struct queue *q) {
	q->size = 0;
	q->head = q->tail = 0;
    q->cnt = 0;
	Free(q->buffer);
}
