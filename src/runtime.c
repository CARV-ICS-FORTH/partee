/**
 * @file runtime.c
 * @brief This file contains the core of scheduler of ParTEE.
 * @author Nikos Papakonstantinou <nikpapac@ics.forth.gr>
 *         Foivos S. Zakkak <zakkak@ics.forth.gr>
 * @version v1.0
 * @date 2014-05-08
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <numa.h>
#include <thread_data.h>
#include <config.h>
#include <partee.h>
#include <queue.h>
#include <list.h>
#include <atomic.h>
#include <stats.h>
#include <balloc.h>
#include <deps.h>
#include <trie.h>
#ifdef WITH_REGIONS
#include <regions.h>
#endif /* ifdef WITH_REGIONS */

#define memfence() asm volatile ("mfence" ::: "memory")
#define gccfence() asm volatile ("" ::: "memory")

#define handle_error_en(en, msg)      \
	do { errno = en; perror(msg); \
	     exit(EXIT_FAILURE); } while (0)

#ifdef DEBUG_FILES
#define dprintf(format, ...)                                \
	( ( fprintf(debug_file[thread_id], "%d :"           \
	            format, thread_id, ## __VA_ARGS__) ) ); \
	fflush(debug_file[thread_id])
#else /* ifdef DEBUG_FILES */
#define dprintf(format, ...)                                 \
	fprintf(stderr, "%ld | %d:"                          \
	        format, usecs(), thread_id, ## __VA_ARGS__); \
	fflush(stderr)
#endif /* ifdef DEBUG_FILES */

#define get_next_random_Queue(total_queues) (rand_r(&seed) % total_queues)

uint32_t          MaxActiveThreads = 1;
volatile uint32_t active_threads   = 1;
pthread_barrier_t barrier;

#ifdef DEBUG_CNT_TASKS

volatile uint32_t total_tasks                    = 1;
volatile uint32_t total_tasks_inc                = 1;
volatile uint32_t total_complete_tasks           = 0;
volatile uint32_t total_spawned_tasks            = 0;
volatile uint32_t total_immediate_executed_tasks = 0;

#endif /* ifdef DEBUG_CNT_TASKS */
#ifdef ENABLE_STATS

volatile uint_ptr_t total_allocated_space            = GRID_SZ;
volatile uint32_t   total_allocated_task_descriptors = 0;
__thread int        runtime_flag                     = -1;

#endif /* ifdef ENABLE_STATS */

__thread tpc_task_descriptor *parent;
__thread tpc_task_descriptor *curr_task;
__thread uint32_t            thread_id;
__thread pthread_data_t      *curr_thread_data;
__thread unsigned int        seed;
tpc_task_descriptor          main_task_desc;
pthread_t                    thread[MAX_THREADS];
int                          total_nodes = 0;
/* Allocator's variables */
extern void          *grid_t;
extern unsigned long grid_size_g; /* default grid size */
extern unsigned long block_size_g; /* default block size */

#ifdef DEBUG_FILES

FILE *debug_file[MAX_THREADS];

#endif /* ifdef DEBUG_FILES */

static __thread int steal_index = 0;

/**
 * @brief Allocates and initializes a new tpc_task_descriptor which used to
 *                              describe a task and its arguments.
 *        If statistics are enabled, then the time for allocation measured and
 *        added to the total allocation time, the size of allocation added to
 *        the total_allocated_space and the counters for total allocations
 *        and per thread allocations Increased.
 *
 * @param args_num The number of task's arguments.
 *
 * @return Pointer to new task_descriptor
 */
tpc_task_descriptor* tpc_task_descriptor_alloc(int args_num)
{
	assert(args_num >= 0);

	tpc_task_descriptor *new;

#ifdef ENABLE_STATS
	total_start_time_alloc        = usecs();
	curr_thread_data->alloc_start = usecs();
#endif /* ifdef ENABLE_STATS */

#ifdef WITH_REGIONS
	new = (tpc_task_descriptor*)__rc_rstralloc0( parent->task_region,
	                                             sizeof(tpc_task_descriptor) + args_num *
	                                             ( sizeof(tpc_task_argument) ) );
#else /* ifdef WITH_REGIONS */
	new = (tpc_task_descriptor*)malloc(
	         sizeof(tpc_task_descriptor) + args_num *
	        ( sizeof(tpc_task_argument) ) );
#endif /* ifdef WITH_REGIONS */

#ifdef ENABLE_STATS
	curr_thread_data->alloc_end         = usecs();
	total_stop_time_alloc               = usecs();
	curr_thread_data->total_alloc_time +=
	        (double)(curr_thread_data->alloc_end -
	                 curr_thread_data->alloc_start);
	total_alloc_time +=
	        (double)(total_stop_time_alloc - total_start_time_alloc);
#endif /* ifdef ENABLE_STATS */

	assert(new);

#ifdef ENABLE_STATS
	total_allocated_space += sizeof(tpc_task_descriptor) + args_num *
	                         ( sizeof(tpc_task_argument) );
	v_atomic_incr_long(&total_allocated_task_descriptors);
	v_atomic_incr_long(&curr_thread_data->total_task_descriptors_created);
#endif /* ifdef ENABLE_STATS */
	bzero( new,
	       sizeof(tpc_task_descriptor) + args_num *
	       ( sizeof(tpc_task_argument) ) );
#ifdef WITH_REGIONS
	new->task_region = newregion();
	assert(new->task_region);
#endif /* ifdef WITH_REGIONS */
#ifdef ENABLE_STATS
	total_allocated_space += sizeof(list_t);
#endif /* ifdef ENABLE_STATS */

#ifdef DEBUG_TASK_ALLOC
	dprintf("New tpc_task_descriptor allocated and initialized.\n");
	dprintf("Start Address:   %p\n", new);
	dprintf( "End Address:     %p\n",
	         (uint_ptr_t)new +
	         ( sizeof(tpc_task_descriptor) + args_num *
	           sizeof(tpc_task_argument) ) );
	dprintf( "Allocation size: %p\n",
	         sizeof(tpc_task_descriptor) + args_num *
	         sizeof(tpc_task_argument) );
#ifdef WITH_REGIONS
	dprintf("Task %p create region: %p\n", new, new->task_region);
#endif /* ifdef WITH_REGIONS */
#endif /* ifdef DEBUG_TASK_ALLOC */
	return new;
} /* tpc_task_descriptor_alloc */

/*********NEVER USED*******/
/**
 * @brief Release the memory of tpc_task
 *
 * @param tpc_task
 */
void tpc_task_descriptor_free(tpc_task_descriptor *tpc_task)
{
	assert(tpc_task);
#ifdef DEBUG_TASK_ALLOC
	dprintf("task_descriptor %p with metadata %p will be released\n",
	        tpc_task, tpc_task->metadata);
#endif /* ifdef DEBUG_TASK_ALLOC */
#ifdef WITH_REGIONS
	assert(tpc_task->task_region);
	deleteregion(tpc_task->task_region);
#endif /* ifdef WITH_REGIONS */
	free(tpc_task);
	tpc_task = NULL;
#ifdef debug_task_alloc
	dprintf("a tpc_task_descriptor released.\n");
#endif /* ifdef debug_task_alloc */
}

/**
 * @brief This function, detects the thread_id of the calling thread and place
 *                              it in the core_id physical core.
 *
 * @param core_id The physical core in which the thread will be pinned.
 */
void thread_set_affinity(unsigned int core_id)
{
	cpu_set_t cpuset;
	pthread_t thread;
	int       s;

	thread = pthread_self();

	CPU_ZERO(&cpuset);
	CPU_SET(core_id, &cpuset);

	if ( ( s =
	               pthread_setaffinity_np(thread, sizeof(cpu_set_t),
	                                      &cpuset) ) != 0 )
		handle_error_en(s, "pthread_setaffinity_np");

	/* Check the actual affinity mask assigned to the thread */
#ifdef DEBUG_THREADS
	if ( ( s =
	               pthread_getaffinity_np(thread, sizeof(cpu_set_t),
	                                      &cpuset) ) != 0 )
		handle_error_en(s, "pthread_getaffinity_np");

	dprintf("returned by pthread_getaffinity_np() contained:\n");
	if ( CPU_ISSET(core_id, &cpuset) )
		fprintf(stderr, "CPU %d\n", core_id);

#endif /* ifdef DEBUG_THREADS */
}

/**
 * @brief Change the status of task to COMPLETE_TASK and releases the
 *                              allocated memory of the task descriptor. Also,
 *reduces the number of
 *                              children of parent tasks and this number become
 *0, then recursively
 *                              became the parent task, change the status and
 *release the memory.
 * @brief release the task and its ancestors if needed
 *
 * @param task The task to release
 */
static inline void release_task(tpc_task_descriptor *task)
{
	uint32_t children;

	assert(task);

	/* Race with the task itself, releasing itself. */
	if ( !__sync_bool_compare_and_swap(&task->status, EXECUTED_TASK,
	                                   RELEASING_TASK) )
		return;

	/* release the pending tasks */
	fire_dependencies(task);

#ifdef DEBUG_CNT_TASKS
	/* Decreasing total_tasks counter */
	v_atomic_decr_long(&total_tasks);
	assert( (int)total_tasks > 0 );
	/* dprintf("Decreased TOTAL_TASKS = %d\n", total_tasks); */
#endif /* ifdef DEBUG_CNT_TASKS */

	/* Since I have no children and I just changed my status to completed, I */
	/* have to let my parent know (if any) */
	if ( __builtin_expect(task->parent != NULL, 1) ) {
		children = __sync_sub_and_fetch(
		        &task->parent->number_of_children, 1);
		assert( (int)children >= 0 );
		/* If my parent is executed and it has no children, I have to
		 * release it. */
		if ( __builtin_expect(children == 0 &&
		                      task->parent->status == EXECUTED_TASK,
		                      0) ) {
#ifdef DEBUG_WORKER
			dprintf("c:%p->status = %u\n", task->parent,
			        task->parent->status);
#endif /* ifdef DEBUG_WORKER */
			/* Here only one core will succeed to see zero children
			 * and will only race with its parent completion */
			release_task(task->parent);
		}
	}
}

#ifdef DEBUG_WORKER
void dprint_task_descriptor(tpc_task_descriptor *task)
{
	int               i;
	tpc_task_argument *tmp_arg = task->args;

	dprintf("Next Task for execution...\n");
	dprintf("************************TASK_DESCRIPTOR******************\n");
	dprintf("task->task               = %p\n", task->task);
	dprintf("task->args_num           = %d\n", task->args_num);
	dprintf("task->number_of_children = %d\n", task->number_of_children);
	dprintf("task->status             = %d\n", task->status);
	dprintf("task->parent             = %p\n", task->parent);

	for (i = 0; i < task->args_num; i++) {
		dprintf("\t-------------------------------------\n");
		dprintf("\ttask_arg[%02d]->addr_in     = %p\n", i,
		        tmp_arg->addr_in);
		dprintf("\ttask_arg[%02d]->addr_out    = %p\n", i,
		        tmp_arg->addr_out);
		dprintf("\ttask_arg[%02d]->type        = %d\n", i,
		        (int)tmp_arg->type);
		dprintf("\ttask_arg[%02d]->size        = %d\n", i,
		        (int)tmp_arg->size);
		dprintf("\ttask_arg[%02d]->stride      = %d\n", i,
		        (int)tmp_arg->stride);
		dprintf("\ttask_arg[%02d]->element_num = %d\n", i,
		        (int)tmp_arg->element_num);
		dprintf("\t-------------------------------------\n");
		tmp_arg++;
	}

	dprintf("task->waitfor     = %u\n", task->waitfor);
	/* dprintf("task->fired       = %u\n", task->fired); */
	dprintf("task->lock        = %u\n", task->lock);
#ifdef WITH_REGIONS
	dprintf("task->task_region = %p\n", task->task_region);
#endif /* ifdef WITH_REGIONS */
	dprintf("task->to_notify   = %p\n", task->to_notify);
	dprintf("task->metadata    = %p\n", task->metadata);
	dprintf("*********************************************************\n");
	dprintf("Calling the task...\n");
}

#else /* ifdef DEBUG_WORKER */
#define dprint_task_descriptor(x)
#endif /* ifdef DEBUG_WORKER */

static inline void execute_task(tpc_task_descriptor *task)
{
	tpc_task_descriptor *tmp = curr_task;

	parent    = task;
	curr_task = task;

	dprint_task_descriptor(task);

#ifdef WITH_REGIONS
	task->metadata = new_trie(task->task_region);
#else /* ifdef WITH_REGIONS */
	task->metadata = new_trie();
#endif /* ifdef WITH_REGIONS */
#ifdef ENABLE_STATS
	curr_thread_data->start_time          = usecs();
	curr_thread_data->total_runtime_time +=
	        (double)(curr_thread_data->start_time -
	                 curr_thread_data->runtime_timer);
#endif /* ifdef ENABLE_STATS */

	/* call the wrapper function */
	task->task(task->args);

#ifdef ENABLE_STATS
	curr_thread_data->stop_time  = usecs();
	curr_thread_data->exec_time +=
	        (double)(curr_thread_data->stop_time -
	                 curr_thread_data->start_time);
	if ( curr_thread_data->highest_exec_task_time <
	     (curr_thread_data->stop_time - curr_thread_data->start_time) )
		curr_thread_data->highest_exec_task_time =
		        (curr_thread_data->stop_time -
		         curr_thread_data->start_time);
	v_atomic_incr_long(&curr_thread_data->total_tasks);
	runtime_flag                    = 1;
	curr_thread_data->runtime_timer = usecs();
#endif /* ifdef ENABLE_STATS */

#ifdef DEBUG_CNT_TASKS
	v_atomic_incr_long(&total_complete_tasks);
	assert( (total_tasks_inc > total_complete_tasks) );
#endif /* ifdef DEBUG_CNT_TASKS */

#ifdef DEBUG_WORKER
	dprintf("Task cleared...\n");
#endif /* ifdef DEBUG_WORKER */

	/* The task now is completed and we have to release the depended tasks. */
	assert(task && task->status == SCHEDULED_TASK);

	/* Only the one running the task can complete it, so we can skip
	 * locking here */
	task->status = EXECUTED_TASK;

#ifdef DEBUG_WORKER
	dprintf("a:%p->status = %u\n", task, task->status);
#endif /* ifdef DEBUG_WORKER */

	if ( (int)task->number_of_children > 0 ) {
		curr_task = tmp;
		parent    = tmp;

		return;
	}

	release_task(task);

	curr_task = tmp;
	parent    = tmp;

#ifdef DEBUG_WORKER
	dprintf(
	        "###################################################################\n");
#endif /* ifdef DEBUG_WORKER */
} /* execute_task */

static inline tpc_task_descriptor* pop_task()
{
	tpc_task_descriptor *task;

#ifdef DEBUG_WORKER
	dprintf(
	        "######################### WORKER_%02d_DEBUG #########################\n",
	        curr_thread_data->phys_core);
	dprintf("Dequeuing new task from its PUBLIC Queue...\n");
#endif /* ifdef DEBUG_WORKER */

#ifdef ENABLE_STATS
	curr_thread_data->old_runtime_timer = curr_thread_data->runtime_timer;
	curr_thread_data->runtime_timer     = usecs();
	if (runtime_flag == 1) {
		curr_thread_data->total_runtime_time +=
		        (double)(curr_thread_data->runtime_timer -
		                 curr_thread_data->stop_time);
		runtime_flag = 0;
	} else if (runtime_flag == 0) {
		curr_thread_data->total_idle_time +=
		        (double)(curr_thread_data->runtime_timer -
		                 curr_thread_data->old_runtime_timer);
	}
	curr_thread_data->queue_start = usecs();
#endif /* ifdef ENABLE_STATS */
	task = dequeue_front(task_Queues[curr_thread_data->queue_index]);

#ifdef ENABLE_STATS
	if (task) {
		curr_thread_data->queue_stop          = usecs();
		curr_thread_data->total_dequeue_time +=
		        (double)(curr_thread_data->queue_stop -
		                 curr_thread_data->queue_start);
		v_atomic_incr_long(&curr_thread_data->total_dequeues);
	}
#endif /* ifdef ENABLE_STATS */

	assert( task || isEmpty_Queue(
	                task_Queues[curr_thread_data->queue_index]) );

	return task;
}

static inline tpc_task_descriptor* steal_task()
{
	tpc_task_descriptor *task;
	register int        ret;

#ifdef BACKOFF

	volatile static __thread unsigned int bk = BACKOFF_MIN;

#endif /* ifdef BACKOFF */

#ifdef ENABLE_STATS
	curr_thread_data->old_runtime_timer = curr_thread_data->runtime_timer;
	curr_thread_data->runtime_timer     = usecs();
	if (runtime_flag == 1) {
		curr_thread_data->total_runtime_time +=
		        (double)(curr_thread_data->runtime_timer -
		                 curr_thread_data->stop_time);
		runtime_flag = 0;
	} else if (runtime_flag == 0) {
		curr_thread_data->total_idle_time +=
		        (double)(curr_thread_data->runtime_timer -
		                 curr_thread_data->old_runtime_timer);
	}
#endif /* ifdef ENABLE_STATS */

#ifdef DEBUG_WORKER
	dprintf("Work Steal Start!!!!\n");
#endif /* ifdef DEBUG_WORKER */

	task = NULL;
	ret  = dequeue_back(task_Queues[steal_index], &task);
	if (ret == 0) {
		/* If empty next time try another queue */
		steal_index = get_next_random_Queue(MaxActiveThreads);
#ifdef BACKOFF
		bk = BACKOFF_MIN;
#endif /* ifdef BACKOFF */
	}
#ifdef BACKOFF
	else if (ret == -1) {
		/* If there is contention back-off */
		/* printf("BACKOFF %d\n", bk); */
		usleep(bk);
		bk <<= 2;
		bk   = (bk > BACKOFF_MAX) ? BACKOFF_MAX : bk;
	} else {
		bk = BACKOFF_MIN;
	}
#endif /* ifdef BACKOFF */

#ifdef ENABLE_STATS
	if (task) {
		curr_thread_data->queue_stop          = usecs();
		curr_thread_data->total_dequeue_time +=
		        (double)(curr_thread_data->queue_stop -
		                 curr_thread_data->queue_start);
		v_atomic_incr_long(&curr_thread_data->total_steals);
	}
	v_atomic_incr_long(&curr_thread_data->total_steal_attempts);
#endif /* ifdef ENABLE_STATS */

#ifdef DEBUG_WORKER
	if (!task)
		dprintf("Failed to steal from task_Queues[%02d]\n",
		        curr_thread_data->queue_index);
#endif /* ifdef DEBUG_WORKER */
	return task;
} /* steal_task */

/**
 * @brief Try to enqueue the task in current thread's task Double Ended Queue
 *                              and if the Queue is full, execute the task
 *inline.
 *
 * @param task Task to be scheduled.
 */
void schedule_task(tpc_task_descriptor *task)
{
	assert(task && task->waitfor == task->fired);

#ifdef DEBUG_WORKER
	dprintf("Task to be scheduled: %p.\n", task);
#endif /* ifdef DEBUG_WORKER */

	/* Race with other notifiers */
	if ( __sync_bool_compare_and_swap(&task->status, CREATED_TASK,
	                                  SCHEDULED_TASK) ) {
#ifdef ENABLE_STATS
		curr_thread_data->queue_start = usecs();
#endif /* ifdef ENABLE_STATS */
		if (enqueue(task,
		            task_Queues[curr_thread_data->queue_index]) == 0) {
			/* Enqueue failed (it is full), so we have to execute
			 * the
			 * task inline! */
			execute_task(task);
#ifdef DEBUG_WORKER
			dprintf("Task %p executed inline.\n", task);
#endif /* ifdef DEBUG_WORKER */
#ifdef ENABLE_STATS
			v_atomic_incr_long(
			        &curr_thread_data->total_immediate_executed_tasks);
#endif /* ifdef ENABLE_STATS */
#ifdef DEBUG_CNT_TASKS
			v_atomic_incr_long(&total_immediate_executed_tasks);
#endif /* ifdef DEBUG_CNT_TASKS */
			return;
		}

#ifdef ENABLE_STATS
		curr_thread_data->queue_stop          = usecs();
		curr_thread_data->total_enqueue_time +=
		        (double)(curr_thread_data->queue_stop -
		                 curr_thread_data->queue_start);
		v_atomic_incr_long(&curr_thread_data->total_enqueues);
		v_atomic_incr_long(&curr_thread_data->total_scheduled_tasks);
		curr_thread_data->runtime_timer = usecs();
#endif /* ifdef ENABLE_STATS */
#ifdef DEBUG_CNT_TASKS
		v_atomic_incr_long(&total_spawned_tasks);
#endif /* ifdef DEBUG_CNT_TASKS */
#ifdef DEBUG_WORKER
		dprintf("Task %p enqueued.\n", task);
#endif /* ifdef DEBUG_WORKER */
	}
} /* schedule_task */

/**
 * @brief The argument arg is the number of the thread index in an array of
 *                              thread_data and task_Queues. The physical core
 *in which the thread
 *                              will be placed is calculated from the arg too.
 *If the machine is
 *                              NUMA, the placement of the threads is being with
 *a round robin
 *                              algorithm in numa nodes and there is a special
 *configuration for a
 *                              machine (hydra0) in which the threads except
 *from the previous
 *                              placement policy, the are placed core by core in
 *the every numa node.
 *                              Also, there is different memory placement in
 *this case. The
 *                              allocations are made in the nearest memory
 *controller. If the
 *                              machine is not NUMA, threads are placed in every
 *node with a round
 *                              robin policy. The task_Queues are initialized in
 *this function and
 *                              all the statistics are initialized if they are
 *enabled. There is a
 *                              synchronization barrier before the thread start
 *executing tasks, in
 *                              which all the threads are wait for the
 *initialization of the other
 *                              thread and then, the start execute tasks. After
 *they are finishing
 *                              with the tasks, the copy their statistics in a
 *global array (if the
 *                              are enabled), release its task queue and then,
 *each thread stop its
 *                              execution.
 *
 * @param arg
 *
 * @return void *
 */
void* worker_thread(void *arg)
{
	assert(arg);
	parent           = (tpc_task_descriptor*)&main_task_desc;
	curr_thread_data = (pthread_data_t*)malloc( sizeof(pthread_data_t) );
	bzero( curr_thread_data, sizeof(pthread_data_t) );

	uint32_t            system_proc = sysconf(_SC_NPROCESSORS_CONF);
	uint32_t            phys_core;
	tpc_task_descriptor *task;

	if (numa_available() == -1) {
		/* round-robin pinning threads */
		phys_core =
		        (uint32_t)( ( (uint_ptr_t)arg & 0xFFFFFFFF ) %
		                    system_proc );
	} else {
		total_nodes = numa_num_configured_nodes();
		/*phys_core = (uint32_t)*/
		/*((((uint_ptr_t)arg & 0xFFFFFFFF)*total_nodes)%system_proc) +*/
		/*((((uint_ptr_t)arg & 0xFFFFFFFF)*total_nodes)/system_proc);*/
		phys_core =
		        (uint32_t)( ( ( (uint_ptr_t)arg & 0xFFFFFFFF ) *
		                      (system_proc / total_nodes) ) %
		                    system_proc );

		uint32_t pad =
		        ( ( ( ( (uint_ptr_t)arg & 0xFFFFFFFF ) ) *
		            (system_proc / total_nodes) ) / system_proc );

		/* Special configuration for hydra0 */
		/*if( pad == 1 ) pad = 2; */
		/*else if( pad == 2) pad = 4; */
		/*else if( pad == 3) pad = 6; */
		/*else if( pad == 4) pad = 1; */
		/*else if( pad == 5) pad = 3; */
		/*else if( pad == 6) pad = 5; */
		if (pad != 0 && pad != system_proc / total_nodes - 1) {
			if ( pad < ( (system_proc / total_nodes) / 2 ) )
				pad = 0 + pad * 2;
			else
				pad = 1 +
				      ( pad -
				        ( (system_proc / total_nodes) / 2 ) ) *
				      2;
		}
		phys_core += pad;
		phys_core %= system_proc;

		struct bitmask *bitmask = numa_get_membind();

		numa_bind(bitmask);
	}
	curr_thread_data->phys_core = phys_core;
	thread_id                   =
	        (uint32_t)( (uint_ptr_t)arg & 0xFFFFFFFF );
	thread_set_affinity(curr_thread_data->phys_core);
	/*if(numa_available() != -1) */
	/*numa_set_localalloc(); */
	curr_thread_data->queue_index =
	        (uint32_t)( (uint_ptr_t)arg & 0xFFFFFFFF );
	curr_thread_data->pid = pthread_self();
	seed                  = pthread_self();
	/* Each Worker allocates and initialize its queue. */
	new_Queue(&task_Queues[curr_thread_data->queue_index]);
	assert(task_Queues[curr_thread_data->queue_index]);
	init_queue(task_Queues[curr_thread_data->queue_index]);
	/* Synchronization Barrier */
	pthread_barrier_wait(&barrier);
	/* Barrier released, rest Workers starting execute tasks */
#ifdef ENABLE_STATS
	curr_thread_data->thread_start_time = usecs();
#endif /* ifdef ENABLE_STATS */
	while (active_threads) {
		if ( ( task = pop_task() ) )
			execute_task(task);
		else if ( ( task = steal_task() ) )
			execute_task(task);
	}
#ifdef ENABLE_STATS
	curr_thread_data->thread_stop_time = usecs();
	thread_data[thread_id]             = *curr_thread_data;
#endif /* ifdef ENABLE_STATS */
	/* Synchronization Barrier */
	pthread_barrier_wait(&barrier);
	/* Barrier released, rest Workers release their queues */
	release_queue(task_Queues[curr_thread_data->queue_index]);
	free(curr_thread_data);
	pthread_exit(NULL);
} /* worker_thread */

/**
 * @brief Initialize the whole runtime. Spawns the proper number of thread,
 *                              creates the main task descriptor.
 *                              If the statistics are enabled, the start the
 *proper timers and
 *                              initialize the values of counters.
 *
 * @param threads
 */
void tpc_init(uint32_t threads)
{
	uint32_t system_proc;
	int      i;

	system_proc = sysconf(_SC_NPROCESSORS_CONF);

	if (threads > system_proc)
		fprintf(stderr,
		        "WARNING: Number of threads is bigger than CPU cores!\n");

	MaxActiveThreads =
	        (threads <= 0 || threads > MAX_THREADS) ? system_proc : threads;

#ifdef DEBUG_THREADS
	dprintf("MaxActiveThreads = %d\n", MaxActiveThreads);
#endif /* ifdef DEBUG_THREADS */
	balloc_init(GRID_SZ);
#ifdef DEBUG_THREADS
	dprintf("Allocator initialized\n");
#endif /* ifdef DEBUG_THREADS */
#ifdef WITH_REGIONS
	region_init();
#endif /* ifdef WITH_REGIONS */

#ifdef DEBUG_FILES
	for (i = 0; i < MaxActiveThreads; i++) {
		char file[20];

		sprintf(file, "debug_file_%d.log", i);
		debug_file[i] = fopen(file, "w");
	}
#endif /* ifdef DEBUG_FILES */

	int rc = pthread_barrier_init(&barrier, NULL, threads);

	if (rc) {
		fprintf( stderr, "pthread_barrier_init: %s\n", strerror(rc) );
		exit(1);
	}

	thread_id        = 0;
	curr_thread_data = (pthread_data_t*)malloc( sizeof(pthread_data_t) );
	bzero( curr_thread_data, sizeof(pthread_data_t) );
	curr_thread_data->phys_core = 0;
	thread_set_affinity(curr_thread_data->phys_core);
	thread_id                     = curr_thread_data->phys_core;
	curr_thread_data->queue_index = curr_thread_data->phys_core;
	curr_thread_data->pid         = pthread_self();
	seed                          = pthread_self();
	thread_set_affinity(0);
	/*if(numa_available() != -1) */
	/*numa_set_localalloc(); */
	bzero( &main_task_desc, sizeof(tpc_task_descriptor) );
	main_task_desc.status             = SCHEDULED_TASK;
	main_task_desc.number_of_children = 0;
	main_task_desc.parent             = NULL;
#ifdef WITH_REGIONS
	main_task_desc.task_region = newregion();
	main_task_desc.metadata    = new_trie(main_task_desc.task_region);
#else /* ifdef WITH_REGIONS */
	main_task_desc.metadata = new_trie();
#endif /* ifdef WITH_REGIONS */
#ifdef ENABLE_STATS
	total_allocated_space += sizeof(list_t);
	total_alloc_time       = 0;
#endif /* ifdef ENABLE_STATS */
	parent    = &main_task_desc;
	curr_task = &main_task_desc;

	/* Worker 0 allocates and initialize its queue. */
	new_Queue(&task_Queues[curr_thread_data->queue_index]);
	assert(task_Queues[curr_thread_data->queue_index]);
	init_queue(task_Queues[curr_thread_data->queue_index]);
	for (i = 1; i < MaxActiveThreads; i++) {
		/* create worker threads */
		pthread_create( &thread[i], NULL, worker_thread,
		                (void*)( (uint_ptr_t)i ) );
#ifdef DEBUG_THREADS
		dprintf("New Worker Created...\n");
#endif /* ifdef DEBUG_THREADS */
	}
	/* Synchronization Barrier */
	pthread_barrier_wait(&barrier);
#ifdef ENABLE_STATS
	total_start_time                    = usecs();
	curr_thread_data->thread_start_time = usecs();
#endif /* ifdef ENABLE_STATS */
} /* tpc_init */

/**
 * @brief The caller thread continues execute tasks by calling the tpc_sync
 *                              until the number of spawned tasks become zero.
 *Then inform the other
 *                              threads by setting the value 0 to active_threads
 *variable. The other
 *                              threads then stop executing. Releases the used
 *memory.
 *                              If the statistics are enabled stops the timers.
 */
void tpc_shutdown()
{
	int i;

#ifdef DEBUG_CNT_TASKS
	dprintf("TOTAL_TASKS = %d\n", total_tasks);
	dprintf("Last Worker Created...\n");
#endif /* ifdef DEBUG_CNT_TASKS */
	/* calling tpc_sync to continue executing tasks */
	tpc_sync();
#ifdef DEBUG_CNT_TASKS
	/* Since all our children finished execution we should be the only
	 * one active */
	assert(total_tasks == 1);
#endif /* ifdef DEBUG_CNT_TASKS */

	active_threads = 0;

#ifdef ENABLE_STATS
	curr_thread_data->thread_stop_time = usecs();
	total_stop_time                    = usecs();
#endif /* ifdef ENABLE_STATS */
	/* Synchronization Barrier */
	pthread_barrier_wait(&barrier);
	for (i = 1; i < MaxActiveThreads; i++) {
		pthread_join(thread[i], NULL);
	}
#ifdef ENABLE_STATS
	thread_data[thread_id] = *curr_thread_data;
	print_stats(stdout);
#endif /* ifdef ENABLE_STATS */

#ifdef WITH_REGIONS
	deleteregion(main_task_desc.task_region);
#endif /* ifdef WITH_REGIONS */
	balloc_finalize();

	/* Worker 0 release its queue. */
	release_queue(task_Queues[curr_thread_data->queue_index]);
	free(curr_thread_data);

#ifdef DEBUG_FILES
	for (i = 0; i < MaxActiveThreads; i++) {
		fclose(debug_file[i]);
	}
#endif /* ifdef DEBUG_FILES */
} /* tpc_shutdown */

/**
 * @brief Creates the Dependencies of the task in Dependency Graph. Also,
 *                              increasing the proper counters.
 *
 * @param tpc_task
 */
void tpc_call(tpc_task_descriptor *tpc_task)
{
	assert(tpc_task);
#ifdef DEBUG_CNT_TASKS
	v_atomic_incr_long(&total_tasks);
	v_atomic_incr_long(&total_tasks_inc);
	assert( (int)total_tasks > 0 );
	/* dprintf("Increased TOTAL_TASKS = %d\n", total_tasks); */
#endif /* ifdef DEBUG_CNT_TASKS */

	if (tpc_task->parent)
		v_atomic_incr_long(&tpc_task->parent->number_of_children);

	tpc_task->status = CREATING_TASK;

	/* Dependence Analysis! */
	create_dependencies(tpc_task);
}

/**
 * @brief Checks the status of the task which calls this function and if the
 *                              task is INCOMPLETE_TASK, then starts executes
 *ready tasks, until the
 *                              current task become COMPLETE_TASK.
 */
void tpc_sync()
{
	tpc_task_descriptor *tmp, *task;

#ifdef DEBUG_SYNC
	dprintf("TPC_SYNC CALLED!!!\n");
#endif /* ifdef DEBUG_SYNC */

#ifdef ENABLE_STATS
	curr_thread_data->stop_time = usecs();
	if (curr_thread_data->start_time > 0 &&
	    curr_thread_data->start_time < curr_thread_data->stop_time)
		curr_thread_data->exec_time +=
		        (double)(curr_thread_data->stop_time -
		                 curr_thread_data->start_time);
	curr_thread_data->thread_sync_start = usecs();
#endif /* ifdef ENABLE_STATS */
	tmp = curr_task;
	while (tmp->number_of_children) {
		/* First execute our local tasks */
		if ( ( task = pop_task() ) )
			execute_task(task);
		/* If there is no local work become a thief */
		else if ( ( task = steal_task() ) )
			execute_task(task);
	}
	curr_task = tmp;
	parent    = tmp;

#ifdef ENABLE_STATS
	curr_thread_data->thread_sync_stop = usecs();
	curr_thread_data->total_sync_time +=
	        (double)(curr_thread_data->thread_sync_stop -
	                 curr_thread_data->thread_sync_start);
	curr_thread_data->start_time = usecs();
#endif /* ifdef ENABLE_STATS */

#ifdef DEBUG_SYNC
	dprintf("TPC_SYNC CLEARED!!!\n");
#endif /* ifdef DEBUG_SYNC */
}

/**
 * @brief Allocates enough number of blocks to cover the requested size.
 *
 * @param size
 *
 * @return Pointed to the start of the first block.
 */
void* tpc_malloc(size_t size)
{
	void *new;

	assert(size > 0);

#ifdef ENABLE_STATS
	curr_thread_data->alloc_start = usecs();
	total_start_time_alloc        = usecs();
#endif /* ifdef ENABLE_STATS */

	new = (void*)balloc_alloc(size);
	assert(new);

#ifdef ENABLE_STATS
	curr_thread_data->alloc_end         = usecs();
	total_stop_time_alloc               = usecs();
	curr_thread_data->total_alloc_time +=
	        (double)(curr_thread_data->alloc_end -
	                 curr_thread_data->alloc_start);
	total_alloc_time +=
	        (double)(total_stop_time_alloc - total_start_time_alloc);
#endif /* ifdef ENABLE_STATS */


#ifdef DEBUG_ALLOC
	dprintf("A new segment allocated...\n");
#endif /* ifdef DEBUG_ALLOC */

	return new;
}

/**
 * @brief Release the allocated blocks using bfree
 *
 * @param ptr
 */
void tpc_free(void *ptr)
{
	assert(ptr);
	bfree( ptr, sizeof(ptr) );

#ifdef DEBUG_ALLOC
	dprintf("A segment released...\n");
#endif /* ifdef DEBUG_ALLOC */
}
