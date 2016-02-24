/**
 * @brief
 * The Dynamic circular work-stealing deque WITHOUT the dynamic part ;)
 * (https://dl.acm.org/citation.cfm?id=1073974)
 *
 * Removes the need for dynamic reallocation and constantly increasing
 * bottom and top
 *
 * @author: Foivos S. Zakkak
 */

#include <string.h>
#include <numa.h>
#include <queue.h>
#include <config.h>

struct _queue_t {
	volatile uint8_t    bottom __attribute__( ( aligned(CACHE_LINE) ) );
	volatile uint8_t    top    __attribute__( ( aligned(CACHE_LINE) ) );
	uint32_t            size;
	tpc_task_descriptor *entries[MAX_QUEUE_ENTRIES];
} __attribute__( ( aligned(CACHE_LINE) ) );

/**
 * @brief Allocates the appropriate space for Double Ended Queue
 *              Uses numa_alloc_local if the machine has NUMA architecture,
 * otherwise
 *              uses GNU malloc.
 *
 * @param Queue
 */
void new_Queue(queue_t **Queue)
{
	assert(MAX_QUEUE_ENTRIES == 128);
	assert( !( MAX_QUEUE_ENTRIES & (MAX_QUEUE_ENTRIES - 1) ) );

	if (numa_available() == -1)
		*Queue = (queue_t*)malloc( sizeof(queue_t) );
	else
		*Queue = (queue_t*)numa_alloc_local( sizeof(queue_t) );

	assert(*Queue);
}

/**
 * @brief Initiallize the Queue entries with zero(NULL) and set the proper
 *              values to the other fields.
 *
 * @param Queue
 * @param index
 */
void init_queue(queue_t *Queue)
{
	assert(Queue);
	bzero( Queue, sizeof(queue_t) );
	Queue->size = MAX_QUEUE_ENTRIES;
}

/**
 * @brief Release the allocated space. If there is NUMA architecture, then use
 *              numa_free, otherwise GNU free.
 *
 * @param Queue
 */
void release_queue(queue_t *Queue)
{
	assert(Queue);

	if (numa_available() == -1)
		free(Queue);
	else
		numa_free( Queue, sizeof(queue_t) );
}

/**
 * @brief Check if the Double Ended Queue is empty.
 *
 * @param Queue
 *
 * @return >0 is empty
 *          0 is not empty
 */
__inline__ uint8_t isEmpty_Queue(queue_t *Queue)
{
	uint8_t b, t;

	assert(Queue);

	b = Queue->bottom;
	t = Queue->top;

	return b == t || (uint8_t)(b + 1) == t;
}

/**
 * @brief Check if the Double Ended Queue is full.
 *
 * @param Queue
 *
 * @return >0 is full
 *          0 is not full
 */
__inline__ uint8_t isFull_Queue(queue_t *Queue)
{
	uint8_t b, t;

	assert(Queue);

	b = Queue->bottom;
	t = Queue->top;

	return (b ^ t) == Queue->size;
}

/**
 * @brief Puts a new element(task) in the Double Ended Queue.
 *        This function is only used by the owner of the Double Ended Queue.
 *
 * @param task
 * @param Queue
 *
 * @return 1 SUCCESS
 *         0 FAIL
 */
int enqueue(tpc_task_descriptor *data, queue_t *Queue)
{
	uint8_t b, t;
	int     i;

	assert(data);
	assert(Queue);

	b = Queue->bottom;
	t = Queue->top;

	/* If there is no more space */
	if ( (b ^ t) == Queue->size )
		return 0;

	i                 = b & (Queue->size - 1);
	Queue->entries[i] = data;
	__sync_synchronize();
	Queue->bottom = b + 1;
	/* printf("b=%u t=%u\n", ++b, t);
	 * assert(((b >> 7) == (t >> 7)) || ((b & 127) <= (t & 127))); */

	return 1;
}

/**
 * @brief Dequeues an element from the bottom(front) of the Double
 *        Ended Queue.  This function is only used by the owner of the
 *        Double Ended Queue.
 *
 * @param Queue
 *
 * @return NULL on failure
 *         The latest pushed element.
 */
tpc_task_descriptor* dequeue_front(queue_t *Queue)
{
	tpc_task_descriptor *ret_val = NULL;
	uint8_t             t, b;
	int                 i;

	assert(Queue);

	/* Move b to reserve the bottom */
	b = --Queue->bottom;
	__sync_synchronize();
	/* Start potential critical section */
	t = Queue->top;
	/* printf("0 b=%u t=%u\n", b, t);
	 * assert((b == 255 &&
	 *         t == 0) || ((b >> 7) == (t >> 7)) || ((b & 127) < (t &
	 *127))); */

	/* If it is empty */
	if ( (uint8_t)(b + 1) == t ) {
		Queue->bottom = t;

		return NULL;
	}

	i = b & (Queue->size - 1);
	/* Get the bottom element */
	ret_val = Queue->entries[i];

	/* If the bottom is larger than top then we are not racing with
	 * anyone for this element.  Note that only one thief can race
	 * with us at any point. In the case of more thieves, all except
	 * from one will fail due to the race for the top. As a result,
	 * in the best case scenario we have more than 2 elements in our
	 * queue so both ourselves and a thief can succeed, without any
	 * contention. */
	if (b != t)
		/* printf("1 b=%u t=%u\n", b, t);
		 * assert(b > t || ((t >> 7) && !(b >> 7) && (b & 127) <= (t &
		 *127)));
		 **/

		return ret_val;

	/* However, in the case that there is only one element in the
	 * queue we must make sure that either us or a thief will succeed.
	 * To achieve this we race on the top, by acting like a thief. */
	/* End critical section */
	if ( !__sync_bool_compare_and_swap(&Queue->top, t, t + 1) )
		ret_val = NULL;

	/* Restore bottom since we either stole this element and did not
	 * pop it or failed at stealing it */
	Queue->bottom = b = t + 1;
	/* printf("2 b=%u t=%u\n", b, t);
	 * assert(((b >> 7) == (t >> 7)) || ((b & 127) < (t & 127))); */

	return ret_val;
}                  /* dequeue_front */

/**
 * @brief Dequeues an element for the Double Ended Queue from the opposite side
 *              of the enqueuer.
 *              This function can be used from all threads.
 *
 * @param Queue
 * @param ret_val The oldest element in the Deque
 *
 * @return 0 if empty
 *         1 if dequeued
 *        -1 if contented
 */
int dequeue_back(queue_t *Queue, tpc_task_descriptor **ret_val)
{
	register uint8_t t, b;
	register int     i;

	assert(Queue);

	/* Only one thief can succeed in the following critical section */
	t = Queue->top;
	b = Queue->bottom;

	/* If it is empty */
	if (b == t || (uint8_t)(b + 1) == t)
		return 0;

	/* Get the top element */
	i        = t & (Queue->size - 1);
	*ret_val = Queue->entries[i];

	if ( __sync_bool_compare_and_swap(&Queue->top, t, t + 1) ) {
		/* printf("3 b=%u t=%u\n", b, t);
		 * assert(((b >> 7) == (t >> 7)) || ((b & 127) <= (t & 127)));
		 **/

		return 1;
	} else {
		*ret_val = NULL;
		return -1;
	}
}
