#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "partee.h"
#include "config.h"

typedef struct _queue_node_t queue_node_t;
typedef struct _queue_t      queue_t;

void    new_Queue(queue_t**);
void    init_queue(queue_t*);
void    release_queue(queue_t*);
uint8_t isEmpty_Queue(queue_t*);
uint8_t isFull_Queue(queue_t*);
int     enqueue(tpc_task_descriptor*, queue_t*);

tpc_task_descriptor* dequeue_front(queue_t*);

int dequeue_back(queue_t*, tpc_task_descriptor**);

queue_t *task_Queues[MAX_THREADS] __attribute__( ( aligned(CACHE_LINE) ) );

#endif /* ifndef __QUEUE_H__ */
