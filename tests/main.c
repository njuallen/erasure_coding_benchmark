#include "csapp.h"
#include "adt.h"

void print_queue(struct queue *q) {
    int i = 0;
    printf("size: %d head: %d tail:%d\n", 
            q->size, q->head, q->tail);
    for(i = 0; i < q->size; i++) {
        int *p = (int *)q->buffer[i];
        if(p)
            printf("%d ", *p);
        else
            printf("X ");
    }

    printf("\n");
}

int main(void) {
    struct queue q;
    int size = 5;
    int i = 0;
    queue_init(&q, size);
    int *p = Malloc(sizeof(int) * size);
    for(i = 0; i < size + 1; i++)
        p[i] = i;
    for(i = 0; i < size; i++) {
        enqueue(&q, p + i);
        print_queue(&q);
    }
    for(i = 0; i < 3; i++) {
        int *t = dequeue(&q);
        printf("t: %d\n", *t);
        print_queue(&q);
    }
    for(i = 0; i < 3; i++) {
        enqueue(&q, p + i);
        print_queue(&q);
    }
    for(i = 0; i < 6; i++) {
        int *t = dequeue(&q);
        printf("t: %d\n", *t);
        print_queue(&q);
    }
    queue_destroy(&q);
    return 0;
}
