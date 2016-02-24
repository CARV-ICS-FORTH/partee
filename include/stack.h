#ifndef __STACK_H__
#define __STACK_H__

/* Forward declaration */
struct _tpc_task_descriptor;

/* A stack node */
struct _stack_node_t {
	struct _tpc_task_descriptor *task;
	struct _stack_node_t        *next;
};

typedef struct _stack_node_t stack_node_t;

/* The stack structure */
struct _stack_t {
	stack_node_t *top; /* Use head as the synchronization point */
};

typedef struct _stack_t stack_t;

int stack_push(struct _tpc_task_descriptor*, stack_t*);

stack_node_t               * stack_lock(stack_t*);
struct _tpc_task_descriptor* stack_pop(stack_node_t**);

#endif /* ifndef __STACK_H__ */
