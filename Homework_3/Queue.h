#ifndef QUEUE
#define QUEUE
#include "stdbool.h"
#include "request.h"

typedef struct Queue_t Queue;
typedef struct request_t QueueItemType;

Queue* initQ(int max_size);
void enqueueQ(Queue* queue, QueueItemType item);
QueueItemType dequeueQ(Queue* queue);
bool emptyQ(Queue* queue);
bool fullQ(Queue* queue);
bool canInsertTo(Queue* queue);
bool canTakeFrom(Queue* queue);
int numItemsQ(Queue* queue);
void destroyQ(Queue* queue);
void dropRandQuarter(Queue* queue, void(*do_to_each_dropped)(void*));
void doEachQ(Queue* queue, void(*do_to_each)(void*));
void decMaxSize(Queue* q);
void incMaxSize(Queue* q);


#endif
