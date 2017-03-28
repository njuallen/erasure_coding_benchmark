#ifndef __ADT_H__
#define __ADT_H__

struct queue {
    /* size: the size of the queue
     * cnt: the number of elements in the queue
     */
    int head, tail, size, cnt;
    void **buffer;
};

int queue_is_full(struct queue *q);
int queue_is_empty(struct queue *q);
void queue_init(struct queue *q, int size);
void enqueue(struct queue *q, void *e);
void *dequeue(struct queue *q);
void queue_destroy(struct queue *q);


#endif
