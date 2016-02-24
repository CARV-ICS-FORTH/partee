/**
 * @file stack.c
 *
 * @brief A semi-lock-free stack, supporting concurrent pushes at the back,
 * pops should be only performed after locking the task
 *
 * @author Foivos S. Zakkak <zakkak@ics.forth.gr>
 */
#include <string.h>
#include <assert.h>

#include <partee.h>
#include <config.h>

#define STACK_LOCKED ( (void*)-1 )

/**
 * @brief tries to push a new element to the stack
 *
 * @return 0 If the stack is locked
 *         1 If the insertion succeeded
 */
int stack_push(tpc_task_descriptor *task, stack_t *stack)
{
	stack_node_t *tmp;
	stack_node_t *top;
	stack_node_t *cas;

	assert(task);
	assert(stack);

	top = stack->top;

	if (top == STACK_LOCKED)
		return 0;

	tmp = (stack_node_t*)malloc( sizeof(stack_node_t) );
	assert(tmp);
	bzero( tmp, sizeof(stack_node_t) );
	tmp->task = task;
	tmp->next = top;

	while (top != STACK_LOCKED &&
	       ( cas =
	                 __sync_val_compare_and_swap(&stack->top, top,
	                                             tmp) ) != top) {
		top       = cas;
		tmp->next = top;
	}

	if (top == STACK_LOCKED) {
		free(tmp);

		return 0;
	}

	return 1;
}                  /* stack_insert */

/**
 * @brief Locks the given stack
 *
 * @param stack The stack
 * @return The top of the stack
 */
stack_node_t* stack_lock(stack_t *stack)
{
	stack_node_t *top;
	stack_node_t *cas;

	assert(stack);

	top = stack->top;

	assert(top != STACK_LOCKED);

	while ( ( cas =
	                  __sync_val_compare_and_swap(&stack->top, top,
	                                              STACK_LOCKED) ) != top ) {
		top = cas;
		assert(top != STACK_LOCKED);
	}

	return top;
}                  /* stack_lock */

/**
 * stack_pop
 *
 * @param stack TODO
 * @return TODO
 */
tpc_task_descriptor* stack_pop(stack_node_t **stack)
{
	tpc_task_descriptor *ret_val;
	stack_node_t        *tmp;

	if ( (*stack) == NULL )
		return NULL;

	ret_val = (*stack)->task;
	tmp     = (*stack);
	*stack  = (*stack)->next;
	free(tmp);

	return ret_val;
}
