#include "queue.h"

void queue_init(queue_t *q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

int queue_is_empty(queue_t *q) {
    return q->count == 0;
}

int queue_is_full(queue_t *q) {
    return q->count == MAX_QUEUE_LEN;
}

void queue_enqueue(queue_t *q, PCB_t *pcb) {
    if (queue_is_full(q)) return;
    q->rear = (q->rear + 1) % MAX_QUEUE_LEN;
    q->items[q->rear] = pcb;
    q->count++;
}

PCB_t* queue_dequeue(queue_t *q) {
    if (queue_is_empty(q)) return 0;
    PCB_t *pcb = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_LEN;
    q->count--;
    return pcb;
}
