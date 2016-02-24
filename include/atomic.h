#ifndef __ATOMIC_H__
#define __ATOMIC_H__

/* void v_atomic_incr_byte(volatile uint8_t * );
 *
 * void v_atomic_decr_byte(volatile uint8_t * );
 *
 * void v_atomic_incr_word(volatile uint16_t * );
 *
 * void v_atomic_decr_word(volatile uint16_t * ); */

void v_atomic_incr_long(volatile uint32_t*);

/* void v_atomic_decr_long(volatile uint32_t * );
 *
 * #ifdef __x86_64__
 * void v_atomic_incr_quad(volatile uint64_t * );
 *
 * void v_atomic_decr_quad(volatile uint64_t * );
 * #endif */

int v_atomic_test_and_set(volatile int*);

int try_lock(volatile int*);

void acquire_spinlock(volatile int*);
void acquire_spinlock1(volatile int*);
void acquire_spinlock2(volatile int*);

void release_spinlock(volatile int*);

/* int fetch_and_add( volatile int * , int ); */

/* uint32_t compare_and_swap ( volatile uint32_t *, uint32_t , uint32_t );
 *
 * uint32_t atomic_add_unless( volatile uint32_t *, uint32_t , uint32_t );
 *
 * uint32_t atomic_sub_and_fetch ( volatile uint32_t * );
 *
 * long * swap( long * , long ); */

#endif /* ifndef __ATOMIC_H__ */
