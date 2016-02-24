/**
 * @file deps.c
 * @brief This file includes the implementation of create_dependencies and
 *                              fire_dependencies functions.
 * @author Nikos Papakonstantinou <nikpapac@ics.forth.gr>
 *         Foivos S. Zakkak <zakkak@ics.forth.gr>
 * @version v1.0
 * @date 2014-05-08
 */
#include <string.h>
#include <partee.h>
#include <list.h>
#include <queue.h>
#include <thread_data.h>
#include <atomic.h>
#include <assert.h>
#include <metadata.h>
#include <config.h>
#include <deps.h>
#include <trie.h>
#include <stats.h>

#define memfence() asm volatile ("mfence" ::: "memory")
#define gccfence() asm volatile ("" ::: "memory")

#ifdef DEBUG_FILES
#define dprintf(format, ...)                                \
	( ( fprintf(debug_file[thread_id], "%d :"           \
	            format, thread_id, ## __VA_ARGS__) ) ); \
	fflush(debug_file[thread_id])
#else /* ifdef DEBUG_FILES */
#define dprintf(format, ...)                                 \
	fprintf(stderr, "%ld | %3d:"                         \
	        format, usecs(), thread_id, ## __VA_ARGS__); \
	fflush(stderr)
#endif /* ifdef DEBUG_FILES */

#define GET_INDEX(address)                                              \
	( ( ( (uint_ptr_t)address - (uint_ptr_t)grid_t ) / BLOCK_SZ ) % \
	  (GRID_SZ / BLOCK_SZ) )

#ifdef ENABLE_STATS

extern __thread pthread_data_t *curr_thread_data;
extern uint32_t                total_immediate_executed_tasks;
extern uint32_t                total_spawned_tasks;

#endif /* ifdef ENABLE_STATS */

extern void *grid_t;

void schedule_task(tpc_task_descriptor*);

#ifdef DEBUG_DEPS
void CheckExistingMetadataElements(tpc_task_descriptor *task)
{
	int               i, j, meta_index_start, meta_index_end;
	tpc_task_argument *tmp_arg;
	Metadata_element  *metadata = NULL;

	assert(task);
	tmp_arg = task->args;

	dprintf("Task: %p\n", task);
	dprintf("Checking parent's: %p lookup table\n", task->parent->metadata);
	for (i = 0; i < task->args_num; i++) {
		dprintf("Argument: %p with type %p.\n", tmp_arg, tmp_arg->type);
		if ( !SCOOP_IS_SAFEARG(tmp_arg) &&
		     !SCOOP_IS_BYVALUEARG(tmp_arg) ) {
			meta_index_start = GET_INDEX(tmp_arg->addr_out);
			meta_index_end   =
			        GET_INDEX(
			                tmp_arg->addr_out + (uint_ptr_t)tmp_arg->size -
			                1);
			dprintf("Checking blocks from %d to %d.\n",
			        meta_index_start, meta_index_end);
			for (j = meta_index_start; j <= meta_index_end; j++) {
				metadata = get_node(task->parent->metadata, j);
				if (metadata) {
					dprintf(
					        "#################################################\n");
					dprintf( "Metadata is:     %p\n",
					         (metadata) );
					dprintf("Type:          \t%u\n",
					        (metadata)->type);
					dprintf("Waiting Tasks: \t%u\n",
					        (metadata)->waiting_tasks);
					dprintf("Total Readers: \t%u\n",
					        (metadata)->total_readers);
					dprintf("Readers:       \t%p\n",
					        (metadata)->readers);
					dprintf("Owner:         \t%p\n",
					        (metadata)->owner);
					dprintf("Owner's status:\t%d\n",
					        (metadata)->owner->status);
					if (metadata->dependent_metadata) {
						dprintf("Dependent Task \t%p\n",
						        (metadata)->dependent_metadata->owner);
						dprintf(
						        "Dep Task's status:\t%d\n",
						        (metadata)->dependent_metadata->owner->status);
					}
					dprintf(
					        "#################################################\n");
				}
			}
		}
		tmp_arg++;
	}
}

void dprint_metadata(Metadata_element *metadata)
{
	dprintf("#################################################\n");
	dprintf( "Metadata is: \t\t%p\n", (metadata) );
	dprintf("Type:          \t%u\n", (metadata)->type);
	dprintf("Waiting Tasks: \t%u\n", (metadata)->waiting_tasks);
	dprintf("Total Readers: \t%u\n", (metadata)->total_readers);
	dprintf("Readers:       \t%p\n", (metadata)->readers);
	dprintf("Owner:         \t%p\n", (metadata)->owner);
	dprintf("Owner's status: \t%d\n", (metadata)->owner->status);
	dprintf("#################################################\n");
}

#else /* ifdef DEBUG_DEPS */
#define dprint_metadata(x)
#define CheckExistingMetadataElements(x)
#endif /* ifdef DEBUG_DEPS */

/* /\**
 *  * Check if task already depends on the metadata's owner task
 *  *
 *  * @return 1 if it does
 *  *         0 otherwise
 *  *\/
 * static inline uint8_t
 * depends_on(tpc_task_descriptor *task, Metadata_element *metadata)
 * {
 *      tpc_task_descriptor *depend_on;
 *      int i;
 *
 *      /\*
 *       * First check with the most recent task we found to
 *       * depend on
 *       *\/
 *      depend_on = task->deps_last;
 *      if (depend_on) {
 *              /\* If we already depend on this task *\/
 *              if (depend_on == metadata->owner) {
 *                      return 1;
 *              }
 *
 *              depend_on = task->deps[0];
 *              for (i=1; depend_on && i<MAX_DEPENDS_ON; ++i) {
 *                      if (depend_on == metadata->owner) {
 *                              return 1;
 *                      }
 *                      depend_on = task->deps[i];
 *              }
 *      }
 *
 *      return 0;
 * } */

/**
 * @brief Parse the blocks in parent's look-up table and checks if other tasks
 *                       use or need this block. Informs other task's to_notify
 *list.
 *
 * @param task The current task
 * @param arg The current argument which is being checked
 * @param new_metadata The Metadata element which will be assinged to parent's
 *                                                                              lookup
 *table
 */
static inline uint32_t create_dependency(tpc_task_descriptor *task,
                                         tpc_task_argument   *arg,
                                         Metadata_element    *new_metadata)
{
	register int      j, meta_index_start, meta_index_end;
	Metadata_element  *metadata;
	register uint32_t waitfor;

	assert(task);
	assert(arg);
	assert(new_metadata);

	waitfor = 0;

	if ( SCOOP_IS_OUTARG(arg) ) {
		/* Check task's metadata table for existing references and if
		 * there are
		 * not references create new metadata element with new task as
		 *owner
		 * which assigned to the proper block. On the other hand if
		 * there is any reference, a new metadata element created with
		 *new task
		 * as owner, the proper pointer updated and the new metadata
		 * element assigned after the existing one. In
		 * addition, increase the dependencies counter in the new task
		 *and put
		 * the new task in the notify task list of the running
		 * task and in the notify list of all waiting tasks.
		 */
#ifdef DEBUG_DEPS
		dprintf("Argument: %p is OUT or INOUT!\n", arg);
#endif /* ifdef DEBUG_DEPS */
		meta_index_start = GET_INDEX(arg->addr_out);
		meta_index_end   = GET_INDEX(arg->addr_out + arg->size - 1);

#ifdef DEBUG_DEPS
		dprintf(
		        "Checking %p to %p (%lu)\n\t\tblock_size=%d grid=%p\n\t\tblocks from %d to %d.\n", arg->addr_out,
		        arg->addr_out + (uint_ptr_t)arg->size,
		        (unsigned long int)arg->size, BLOCK_SZ,
		        (void*)grid_t, meta_index_start, meta_index_end);
#endif /* ifdef DEBUG_DEPS */

		for (j = meta_index_start; j <= meta_index_end; j++) {
#ifdef WITH_REGIONS
			metadata = get_or_create_node(task->parent->metadata, j,
			                              task->parent->task_region);
#else /* ifdef WITH_REGIONS */
			metadata =
			        get_or_create_node(task->parent->metadata, j);
#endif /* ifdef WITH_REGIONS */
#ifdef DEBUG_DEPS
			dprintf(
			        "Getting reference at parent's lookup table:%p[%d].\n", &metadata,
			        j);
#endif /* ifdef DEBUG_DEPS */

			/* Assign the pointer in the new metadata element */
			set_node(task->parent->metadata, j, new_metadata);

			if (metadata == NULL) {
				/* no reference to this block */
#ifdef DEBUG_DEPS
				dprintf("There was no reference at this block\n");
				dprintf( "Metadata is now:%p\n",
				         get_node(task->parent->metadata, j) );
#endif /* ifdef DEBUG_DEPS */
				continue;
			}

			/* there is an existing reference to this block */
			dprint_metadata(metadata);

			/* If the existing metadata element which is assigned to
			 * this block is
			 * INOUT, the new one metadata element created and the
			 *new block now
			 * belongs to the new metadata element. In addition, the
			 *new task
			 * should wait for all previous tasks to complete before
			 *start its
			 * execution. On the other hand, if the existing
			 *metadata element is
			 * IN, then the existing metadata element re-specified
			 *as INOUT and as
			 * owner of the metadata element assigned the new task.
			 *Furthermore,
			 * the new tasks should wait all the previous task
			 *before starting
			 * its execution.
			 */
			if ( SCOOP_IS_OUTARG( (metadata) ) ) { /* WAW */
				/* If we detect a WAW self dependency we ignore
				 * it,
				 * else we try to put the new task in current
				 *owner's
				 * to notify list */
				if ( __builtin_expect(task == metadata->owner ||
				                      task->deps_last ==
				                      metadata->owner ||
				/* depends_on(task, metadata) || */
				                      !stack_push(task,
				                                  &metadata->
				                                   owner->
				                                   to_notify),
                                0) )
					continue;

				/* Increase task's waitfor counter */
				task->deps_last = metadata->owner;
				/* task->deps[task->deps_index++] =
				 * metadata->owner; */
				waitfor++;

#ifdef DEBUG_DEPS
				/* Since the metadata element is locked we can
				 * increase this non atomically */
				metadata->waiting_tasks++;
				dprintf(
				        "Task: %p depends on task %p with status: %u.\n", task, metadata->owner,
				        metadata->owner->status);
				/* dprintf("b:%s: Task:%p waitfor counter = %u
				 * fired counter = %u\n",
				 *         __FUNCTION__, task, task->waitfor,
				 *task->fired); */
#endif /* ifdef DEBUG_DEPS */
			} else { /* WAR */
				/* Put the new task in the to_notify list of all
				 * readers */
				list_entry_t *reader =
				        ( (list_t*)metadata->readers )->head;

				while (reader != NULL) {
					/* Check if it is a self dependency. If
					 * not, then
					 * put the new task in current owner's
					 *to notify
					 * list */
					if ( __builtin_expect(task !=
					                      reader->task &&
					                      task->deps_last !=
					                      reader->task &&
					/* !depends_on(task, metadata) && */
					                      stack_push(task,
					                                 &reader
					                                  ->task
					                                  ->
                                        to_notify), 1) ) {
						/* Increase new task's waitfor
						 * counter */
						task->deps_last = reader->task;
						/*
						 * task->deps[task->deps_index++]
						 * = reader->task; */
						waitfor++;
#ifdef DEBUG_DEPS
						metadata->waiting_tasks++;
						dprintf(
						        "Task: %p depends on task %p with status: %u.\n", task, reader->task,
						        reader->task->status);
						/* dprintf("c:%s: Task:%p
						 * waitfor counter = %u fired
						 * counter = %u\n",
						 *         __FUNCTION__, task,
						 *task->waitfor, task->fired);
						 **/
#endif /* ifdef DEBUG_DEPS */
					}
					/* Get the next reader task */
					reader = reader->next;
				}
			}
		}
	} else {
		/* Check parent's metadata table for existing
		 * references in this block, and if there are not any
		 * references then a new metadata element created without owner
		 *and the
		 * block assigned to this new metadata element.
		 * If there is a reference, and this reference is about an
		 * INOUT or OUT argument, the new task just deposited
		 * in the existing owner's to_notify list. If the
		 * reference is about IN argument, the new task just
		 * deposited in existing metadata's readers list.
		 *
		 * We are the only ones to change the readers list so no locks
		 * needed here
		 */
		insert_front(task, (list_t*)(new_metadata)->readers);

#ifdef DEBUG_DEPS
		new_metadata->total_readers++;
		dprintf("Argument: %p is IN!\n", arg);
#endif /* ifdef DEBUG_DEPS */
		meta_index_start = GET_INDEX(arg->addr_in);
		meta_index_end   = GET_INDEX(arg->addr_in + arg->size - 1);

#ifdef DEBUG_DEPS
		dprintf(
		        "Checking %p to %p (%lu)\n\t\tblock_size=%d grid=%p\n\t\tblocks from %d to %d.\n", arg->addr_in,
		        arg->addr_in + (uint_ptr_t)arg->size, (unsigned long int)arg->size, BLOCK_SZ,
		        (void*)grid_t, meta_index_start, meta_index_end);
#endif /* ifdef DEBUG_DEPS */

		for (j = meta_index_start; j <= meta_index_end; j++) {
#ifdef WITH_REGIONS
			metadata = get_or_create_node(task->parent->metadata, j,
			                              task->parent->task_region);
#else /* ifdef WITH_REGIONS */
			metadata =
			        get_or_create_node(task->parent->metadata, j);
#endif /* ifdef WITH_REGIONS */
#ifdef DEBUG_DEPS
			dprintf(
			        "Getting reference at parent's lookup table:%p[%d].\n", &metadata,
			        j);
#endif /* ifdef DEBUG_DEPS */
			if (metadata == NULL) {
				/* no reference to this block */
#ifdef DEBUG_DEPS
				dprintf(
				        "There is no reference at this block. Let's create one\n");
#endif /* ifdef DEBUG_DEPS */
				/* Assign the pointer in the new metadata
				 * element */
				set_node(task->parent->metadata, j,
				         new_metadata);
#ifdef DEBUG_DEPS
				dprintf( "Metadata is now:%p\n",
				         get_node(task->parent->metadata, j) );
#endif /* ifdef DEBUG_DEPS */
				continue;
			}

			/* there is an existing reference to this block */
			dprint_metadata(metadata);

			if ( SCOOP_IS_OUTARG( (metadata) ) ) { /* RAW */
				/* Assign the pointer in the new metadata
				 * element */
				set_node(task->parent->metadata, j,
				         new_metadata);

				/* If we detect a RAW self dependency we ignore
				 * it,
				 * else we try to put the new task in current
				 *owner's
				 * to notify list */
				if ( __builtin_expect(task == metadata->owner ||
				                      task->deps_last ==
				                      metadata->owner ||
				/* depends_on(task, metadata) || */
				                      !stack_push(task,
				                                  &metadata->
				                                   owner->
				                                   to_notify),
                                0) )
					continue;

				/* Increase task's waitfor counter */
				task->deps_last = metadata->owner;
				/* task->deps[task->deps_index++] =
				 * metadata->owner; */
				waitfor++;
#ifdef DEBUG_DEPS
				dprintf(
				        "Task: %p depends on task %p with status: %u.\n", task, metadata->owner,
				        metadata->owner->status);
				/* dprintf("a:%s: Task:%p waitfor counter = %u
				 * fired counter = %u\n",
				 *         __FUNCTION__, task, task->waitfor,
				 *task->fired); */
				metadata->waiting_tasks++;
#endif /* ifdef DEBUG_DEPS */
				new_metadata->dependent_metadata = metadata;
			} else { /* RAR */
				/* FIXME: Not sure, but i feel this check is
				 * redundant, why does the owner need to be
				 *alive?  */
				if (metadata->owner->status <= EXECUTED_TASK) {
					/* Put task to metadata's readers list */
					insert_front(task,
					             (list_t*)(metadata)->readers);
#ifdef DEBUG_DEPS
					metadata->total_readers++;
#endif /* ifdef DEBUG_DEPS */

					/* Hold a dependent_metadata for RAW so
					 * that
					 * consequent readers will be added to
					 *the
					 * last writer's to_notify list */
					Metadata_element *dep_metadata;

					dep_metadata =
					        metadata->dependent_metadata;

					/* If there is a dependency with some
					 * writer (due
					 * to RRRRRAW) and it is not a self
					 *dependency try
					 * to put the new task in the current
					 *owner's to
					 * notify list */
					if ( __builtin_expect(dep_metadata &&
					                      task !=
					                      metadata->owner &&
					                      task->deps_last !=
					                      metadata->owner &&
					/* !depends_on(task, metadata) && */
					                      stack_push(task,
					                                 &
					                                 dep_metadata
					                                  ->
                                        owner->to_notify), 1) ) {
						task->deps_last =
						        dep_metadata->owner;
						/*
						 * task->deps[task->deps_index++]
						 * = dep_metadata->owner; */
						waitfor++;
					}
				} else {
					set_node(task->parent->metadata, j,
					         new_metadata);
				}
			}
		}
	}

	return waitfor;
} /* create_dependency */

static inline uint32_t handle_argument(tpc_task_argument   *arg,
                                       tpc_task_descriptor *task)
{
	Metadata_element  *new_metadata = NULL;
	register int      j;
	register uint32_t waitfor;

	waitfor = 0;

	/* Skip safe arguments */
	if ( SCOOP_IS_SAFEARG(arg) ) {
#ifdef DEBUG_DEPS
		dprintf("Argument: %p is SAFE or BYVALUE!\n", arg);
#endif /* ifdef DEBUG_DEPS */

		return waitfor;
	}

	/* New metadata element */
#ifdef WITH_REGIONS
	new_metadata = (Metadata_element*)__rc_rstralloc0(
	         task->parent->task_region, sizeof(Metadata_element) );
	new_metadata->readers = (struct list_t*)__rc_rstralloc0(
	         task->parent->task_region, sizeof(list_t) );
#else /* ifdef WITH_REGIONS */
	new_metadata =
	        (Metadata_element*)malloc( sizeof(Metadata_element) );
	new_metadata->readers = (struct list_t*)malloc( sizeof(list_t) );
#endif /* ifdef WITH_REGIONS */
	init_List( (list_t*)new_metadata->readers );
	new_metadata->type  = arg->type;
	new_metadata->owner = task;

#ifdef DEBUG_DEPS
	dprintf("new_metadata = %p\n", new_metadata);
#endif /* ifdef DEBUG_DEPS */

	if ( __builtin_expect(!SCOOP_IS_STRIDEARG(arg), 1) )
		return create_dependency(task, arg, new_metadata);

#ifdef DEBUG_DEPS
	dprintf("Argument: %p is STRIDE argument!\n", arg);
#endif /* ifdef DEBUG_DEPS */

	register void *tmp;

	/* TODO: Further optimize for strides, we do not need to check its
	 * stride element's type of the tile again.  Create a
	 * create_ra_dependency and a create_wa_dependency */
	if ( SCOOP_IS_OUTARG(arg) ) {
		tmp = arg->addr_in;

		for (j = 0; j < arg->element_num; j++) {
			waitfor += create_dependency(task, arg, new_metadata);
			/* Move the start address to the next element */
			arg->addr_out += arg->stride;
		}

		/* Return the pointer to the right place!! */
		arg->addr_out = tmp;
	} else {
		tmp = arg->addr_in;

		for (j = 0; j < arg->element_num; j++) {
			waitfor += create_dependency(task, arg, new_metadata);
			/* Move the start address to the next element */
			arg->addr_in += arg->stride;
		}

		/* Return the pointer to the right place!! */
		arg->addr_in = tmp;
	}

	return waitfor;
}                  /* handle_argument */

/**
 * @brief Parse task's arguments one by one and creates the dependency for each
 *                              one of them, If the task has no dependencies,
 *then schedule the task.
 *
 * @param task The current task.
 */
void create_dependencies(tpc_task_descriptor *task)
{
	int               i;
	tpc_task_argument *tmp_arg;
	register uint32_t waitfor;

	assert(task);

	waitfor = 0;

	tmp_arg = task->args;

#ifdef DEBUG_DEPS
	dprintf("Task: %p\n", task);
	CheckExistingMetadataElements(task);
	dprintf("Checking parent's: %p lookup table\n", task->parent->metadata);
#endif /* ifdef DEBUG_DEPS */

#ifdef ENABLE_STATS
	curr_thread_data->deps_start = usecs();
#endif /* ifdef ENABLE_STATS */
	for (i = 0; i < task->args_num; i++, tmp_arg++) {
#ifdef DEBUG_DEPS
		dprintf("Argument[%d]: %p with type %p.\n", i, tmp_arg,
		        tmp_arg->type);
#endif /* ifdef DEBUG_DEPS */

		waitfor += handle_argument(tmp_arg, task);
	}

#ifdef DEBUG_DEPS
	/* dprintf("d:%s: Task:%p waitfor counter = %u fired counter = %u\n",
	 *         __FUNCTION__, task, task->waitfor, task->fired); */
#endif /* ifdef DEBUG_DEPS */

#ifdef ENABLE_STATS
	curr_thread_data->deps_end                 = usecs();
	curr_thread_data->total_dep_analysis_time +=
	        (double)(curr_thread_data->deps_end -
	                 curr_thread_data->deps_start);

	v_atomic_incr_long(&curr_thread_data->total_spawned_tasks);
#endif /* ifdef ENABLE_STATS */

	/* If there where no dependencies schedule the task */
	if (waitfor == 0) {
		task->status = CREATED_TASK;
		schedule_task(task);
	} else {
		task->waitfor = waitfor;
		__sync_synchronize();
		task->status = CREATED_TASK;
	}
} /* create_dependencies */

/**
 * @brief Parse the waiting list of task and reduce the dependencies to the
 *                              waitng task. In case of zero dependencies, the
 *new task scheduled for
 *                              execution.
 *
 * @param task The current task.
 */
void fire_dependencies(tpc_task_descriptor *task)
{
	tpc_task_descriptor *tpc_task;
	stack_node_t        *to_notify;
	register uint32_t   fired;

	assert(task);
	assert(task->status == RELEASING_TASK);

	/* We do not need to lock this task since at RELEASING_TASK we are
	 * the only core accessing this task descriptor */
	to_notify = stack_lock(&task->to_notify);
#ifdef DEBUG_DEPS
	dprintf("Let's fire the dependencies from task:%p.\n", task);
#endif /* ifdef DEBUG_DEPS */
	while ( ( tpc_task = stack_pop(&to_notify) ) != NULL ) {
#ifdef DEBUG_DEPS
		/* dprintf("%s:b:[%p] Task:%p waitfor counter = %u fired counter
		 * = %u\n",
		 *         __FUNCTION__, task, tpc_task, tpc_task->waitfor,
		 *tpc_task->fired); */
#endif /* ifdef DEBUG_DEPS */
		/* Increasing the fired counter */
		fired = __sync_add_and_fetch(&tpc_task->fired, 1);
#ifdef DEBUG_DEPS
		/* dprintf("%s:b:[%p] Task:%p waitfor counter = %u fired counter
		 * = %u\n",
		 *         __FUNCTION__, task, tpc_task, tpc_task->waitfor,
		 *tpc_task->fired); */
#endif /* ifdef DEBUG_DEPS */
		/* Wait until the task becomes created FIXME: slower than
		 * Nikos... */
		while (tpc_task->status < CREATED_TASK) {
			/* printf("Wait for me\n"); */
		}
		/* Be sure the waitfor counter is equal or greater to the
		 * fired counter. */
		assert(tpc_task->waitfor >= fired);
		assert(fired > 0);
		assert(tpc_task->status >= CREATED_TASK);
		/* If the task has no dependencies and its is marked as
		 * CREATED_TASK, */
		/* then task scheduled for execution */
		if (tpc_task->status == CREATED_TASK &&
		    tpc_task->waitfor == fired)
			schedule_task(tpc_task);
	}
	task->status = COMPLETE_TASK;
#ifdef WITH_REGIONS
	/* Delete task region */
	deleteregion(task->task_region);
#endif /* ifdef WITH_REGIONS */

#ifdef DEBUG_DEPS
#ifdef WITH_REGIONS
	dprintf("Task %p delete region: %p\n", task, task->task_region);
#endif /* ifdef WITH_REGIONS */
	dprintf("Dependencies fired!\n");
#endif /* ifdef DEBUG_DEPS */
} /* fire_dependencies */
