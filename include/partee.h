#ifndef __TPC_API_H__
#define __TPC_API_H__

#include <stdint.h>
#include <stdio.h>
#include <config.h>
#include <regions.h>
#include <list.h>
#include <metadata.h>
#include <stack.h>

/* Bit-masks for supported argument flags and helping macros */
#define SCOOP_IN_ARG           0x1
#define SCOOP_OUT_ARG          0x2
#define SCOOP_INOUT_ARG        0x3
#define SCOOP_STRIDE_ARG       0x4
#define SCOOP_SAFE_ARG         0x8
#define SCOOP_BYVALUE_ARG      0x19
#define SCOOP_REGION_ARG       0x16
#define SCOOP_HIGHPRIORITY_ARG 0x64

#define SCOOP_IS_INARG(x)           ( (x->type) & SCOOP_IN_ARG )
#define SCOOP_IS_OUTARG(x)          ( (x->type) & SCOOP_OUT_ARG )
#define SCOOP_IS_INOUTARG(x)        ( (x->type) & SCOOP_INOUT_ARG )
#define SCOOP_IS_STRIDEARG(x)       ( (x->type) & SCOOP_STRIDE_ARG )
#define SCOOP_IS_SAFEARG(x)         ( (x->type) & SCOOP_SAFE_ARG )
#define SCOOP_IS_BYVALUEARG(x)      ( (x->type) & SCOOP_BYVALUE_ARG )
#define SCOOP_IS_HIGHPRIORITYARG(x) ( (x->type) & SCOOP_HIGHPRIORITY_ARG )
#define SCOOP_IS_REGIONARG(x)       ( (x->type) & SCOOP_REGION_ARG )

/* Task status */
#define CREATING_TASK  0x0 /* Single access */
#define CREATED_TASK   0x1 /* Shared access from this point and after */
#define SCHEDULED_TASK 0x2
#define EXECUTED_TASK  0x3
#define RELEASING_TASK 0x4 /* Single access from this point and after */
#define COMPLETE_TASK  0x5

/* #define MAX_DEPENDS_ON 8 */

/* TPC Arguments */
/* #pragma pack(4) */
struct _tpc_task_argument {
	void     *addr_in;
	void     *addr_out;
	uint32_t type;
	uint32_t size;
	uint32_t stride;
	uint32_t element_num;
};                 /* 32-bytes on 64-bit arch */

typedef struct _tpc_task_argument tpc_task_argument;

/* TPC Task Descriptors */
/* #pragma pack(4) */
struct _tpc_task_descriptor {
	void (*task)(tpc_task_argument*); /* Wrapper to the original function
	                                   * (no value returned) */

	tpc_task_argument *args;
	uint32_t          args_num;
	volatile uint32_t number_of_children __attribute__( ( aligned(
	                                                              CACHE_LINE) ) ); /*
	                                                                                * Counter
	                                                                                * of
	                                                                                * completed
	                                                                                * children */
	volatile uint32_t status __attribute__( ( aligned(
	                                                  CACHE_LINE) ) ); /* Task
	                                                                    * status */
	struct _tpc_task_descriptor *parent; /* Pointer to parent Task */
	/* Dependence analysis */
	volatile int            waitfor __attribute__( ( aligned(CACHE_LINE) ) );
	volatile int            fired   __attribute__( ( aligned(CACHE_LINE) ) );
	region_t                task_region;
	stack_t                 to_notify __attribute__( ( aligned(CACHE_LINE) ) );
	struct Metadata_element ***metadata;

	/* Cache with most recent tasks we depend on */
	/* struct _tpc_task_descriptor *deps[MAX_DEPENDS_ON] __attribute__
	 * ((aligned(CACHE_LINE)));
	 * uint8_t deps_index; */
	struct _tpc_task_descriptor *deps_last __attribute__( ( aligned(
	                                                                CACHE_LINE) ) );

	/* Debug */
	/* uint32_t exec_times; */
} __attribute__( ( aligned(CACHE_LINE) ) ); /* 44-bytes on 64-bit arch */

typedef struct _tpc_task_descriptor tpc_task_descriptor;

/* Alloc/Free Task Descriptors */
tpc_task_descriptor* tpc_task_descriptor_alloc(int); /* number of arguments */

void tpc_task_descriptor_free(tpc_task_descriptor*);

/* + Compiler generated list of all possible task argument numbers */
/* const int tpc_task_arguments_list[] */
/* const int tpc_task_arguments_list[] = {2, 3, 5, 9}; */
/* it will help allocate the appropriate pools during initialization */
/* and avoid malloc/free during run-time */

/* The typical TPC API */
void tpc_init(uint32_t);
void tpc_shutdown();
void tpc_call(tpc_task_descriptor*);
void tpc_sync();

/* void tpc_wait_all(); */
/* void tpc_wait_on(tpc_task_descriptor *); */
/* #pragma css wait on(x,y,z) */
/* equivalent to input */

void* tpc_malloc(size_t);

void tpc_free(void*);

static inline void wrapper_SCOOP__(tpc_task_argument *a) {}

extern __thread tpc_task_descriptor *parent;
extern __thread tpc_task_descriptor *curr_task;

#include <queue.h>
#endif /* ifndef __TPC_API_H__ */
