#ifndef _THREAD_DATA_
#define _THREAD_DATA_

#include <pthread.h>
#include <stdint.h>
#include "config.h"
#include "list.h"
#include "partee.h"


struct _pthread_data {
	uint32_t            phys_core;
	uint32_t            queue_index;
	pthread_t           pid;
	tpc_task_descriptor *current_task;

#ifdef ENABLE_STATS

	long     thread_start_time;
	long     thread_stop_time;
	long     thread_sync_start;
	long     thread_sync_stop;
	long     start_time;
	long     stop_time;
	long     exec_time;
	long     queue_start;
	long     queue_stop;
	long     alloc_start;
	long     alloc_end;
	long     deps_start;
	long     deps_end;
	long     total_enqueue_time;
	long     total_dequeue_time;
	long     total_alloc_time;
	long     highest_exec_task_time;
	long     runtime_timer;
	long     total_dep_analysis_time;
	long     old_runtime_timer;
	long     total_runtime_time;
	long     total_idle_time;
	long     total_sync_time;
	uint32_t total_enqueues;
	uint32_t total_dequeues;
	uint32_t total_spawned_tasks;
	uint32_t total_scheduled_tasks;
	uint32_t total_immediate_executed_tasks;
	uint32_t total_steals;
	uint32_t total_steal_attempts;
	uint32_t total_tasks;
	uint32_t total_task_descriptors_created;

#endif /* ifdef ENABLE_STATS */
} __attribute__( ( aligned(CACHE_LINE) ) );

typedef struct _pthread_data pthread_data_t;

#ifdef ENABLE_STATS

pthread_data_t thread_data[MAX_THREADS] __attribute__( ( aligned(CACHE_LINE) ) );

#endif /* ifdef ENABLE_STATS */

#endif /* ifndef _THREAD_DATA_ */
