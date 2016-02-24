#include <stdint.h>
#include "atomic.h"

/* void v_atomic_incr_byte(volatile uint8_t * address)
 * {
 *   asm volatile ("lock incb %0":"=m" (*address)
 *                 :"m"(*address)
 *       );
 * }
 *
 * void v_atomic_decr_byte(volatile uint8_t * address)
 * {
 *   asm volatile ("lock decb %0":"=m" (*address)
 *                 :"m"(*address)
 *       );
 * }
 *
 * void v_atomic_incr_word(volatile uint16_t * address)
 * {
 *   asm volatile ("lock incw %0":"=m" (*address)
 *                 :"m"(*address)
 *       );
 * }
 *
 * void v_atomic_decr_word(volatile uint16_t * address)
 * {
 *   asm volatile ("lock decw %0":"=m" (*address)
 *                 :"m"(*address)
 *       );
 * }
 */

void v_atomic_incr_long(volatile uint32_t *address)
{
	asm volatile ( "lock incl %0" : "=m" (*address) : "m" (*address) );
}

/* void v_atomic_decr_long(volatile uint32_t * address)
 * {
 *   asm volatile ("lock decl %0":"=m" (*address)
 *                 :"m"(*address)
 *       );
 * }
 *
 * #ifdef __i386__
 * #endif
 *
 * #ifdef __x84_64__
 * void v_atomic_incr_quad(volatile uint64_t * address)
 * {
 *   asm volatile ("lock incq %0":"=m" (*address)
 *                 :"m"(*address)
 *       );
 * }
 *
 * void v_atomic_decr_quad(volatile uint64_t * address)
 * {
 *   asm volatile ("lock decq %0":"=m" (*address)
 *                 :"m"(*address)
 *       );
 * }
 * #endif */

/* Returns the old value of of *lock and sets *lock to 1 (true) atomicaly */
int v_atomic_test_and_set(volatile int *lock)
{
	int ret;
	__asm__ __volatile__ ("lock; xchg %0, %1" : "=r" (ret),
	                      "=m" (*lock) : "0" (1), "m" (*lock) : "memory");

	return ret;
}

#define LOCKED   1
#define UNLOCKED 0

int try_lock(volatile int *l)
{
	return v_atomic_test_and_set(l);
}

/*NOTE: BACKOFF_MIN/MAX are lower and upper bound backoff values.*/
/*Notice that backoff bounds are highly dependent from the system and */
/*from the concurrency levels. This values should be carefully tuned*/
/*in order to achieve the maximum performance.*/

#if !defined (BACKOFF_MIN)
#define BACKOFF_MIN 128
#endif /* if !defined (BACKOFF_MIN) */
#if !defined (BACKOFF_MAX)
#define BACKOFF_MAX 1024
#endif /* if !defined (BACKOFF_MAX) */

void acquire_spinlock(volatile int *l)
{
	long i, delay = BACKOFF_MIN;

	while ( LOCKED == v_atomic_test_and_set(l) ) {
		for (i = 0; i < delay; i++)
			;
		delay <<= 1;
		delay  &= BACKOFF_MAX;
	}
}

void acquire_spinlock1(volatile int *l)
{
	long i, delay = BACKOFF_MIN;

	while ( LOCKED == v_atomic_test_and_set(l) ) {
		for (i = 0; i < delay; i++)
			;
		delay <<= 1;
		delay  &= BACKOFF_MAX;
	}
}

void acquire_spinlock2(volatile int *l)
{
	long i, delay = BACKOFF_MIN;

	while ( LOCKED == v_atomic_test_and_set(l) ) {
		for (i = 0; i < delay; i++)
			;
		delay <<= 1;
		delay  &= BACKOFF_MAX;
	}
}

void release_spinlock(volatile int *l)
{
	*l = UNLOCKED;
}

/* int fetch_and_add( volatile int * variable, int value ){
 *         /\*__asm__ __volatile__("lock; xaddl %%eax, %2;"*\/
 *                         /\*:"=a" (value)                   //Output*\/
 *                         /\*: "a" (value), "m" (*variable)  //Input*\/
 *                         /\*:"memory" );*\/
 *         __sync_add_and_fetch(variable, value);
 *         return value;
 * }
 *
 * uint32_t compare_and_swap ( volatile uint32_t *destination, uint32_t compare,
 *uint32_t exchange ){
 *         return ( __sync_val_compare_and_swap(destination, compare, exchange)
 *);
 * } */

/* uint32_t atomic_add_unless( volatile uint32_t *v, uint32_t a, uint32_t u){
 *         uint32_t c, old;
 *         c = *v;
 *         while (c != u && (old = __sync_val_compare_and_swap(v, c, c + a)) !=
 *c)
 *                 c = old;
 *         return c;
 * } */

/* uint32_t atomic_sub_and_fetch ( volatile uint32_t * variable ){
 *      return ( __sync_sub_and_fetch( variable, 1 ) );
 * } */

/* long * swap( long * ptr , long value ){
 *      return ( (long *)__sync_lock_test_and_set( ptr, value ) );
 * } */
