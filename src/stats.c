/**
 * @file stats.c
 * @brief Statistics implementations.
 * @author Nikos Papakonstantinou <nikpapac@ics.forth.gr>
 * @version v1.0
 * @date 2014-05-08
 */
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <config.h>
#include <thread_data.h>
#include <stats.h>

extern volatile uint32_t MaxActiveThreads;
extern volatile uint32_t max_inclomplete_tasks;

#ifdef DEBUG_CNT_TASKS

extern volatile uint32_t total_complete_tasks;
extern volatile uint32_t total_spawned_tasks;
extern volatile uint32_t total_immediate_executed_tasks;

#endif /* ifdef DEBUG_CNT_TASKS */

extern volatile uint_ptr_t total_allocated_space;
extern volatile uint32_t   total_allocated_task_descriptors;
uint32_t                   total_enqueue_time = 0;
uint32_t                   total_dequeue_time = 0;

/**
 * @brief Give the current timestamp.
 *
 * @return The current timestamp
 */
long usecs(void)
{
	struct timeval t;

	gettimeofday(&t, NULL);
	return t.tv_sec * 1000000 + t.tv_usec;
}

#ifdef ENABLE_STATS
/**
 * @brief Print the statistics in th fp file pointer. Also print the results to
 *                              a csv file.
 *
 * @param fp
 */
void print_stats(FILE *fp)
{
	int i;

	total_exec_time = (double)(total_stop_time - total_start_time);
	for (i = 0; i < MaxActiveThreads; i++) {
		total_enqueue_time += thread_data[i].total_enqueue_time;
		total_dequeue_time += thread_data[i].total_dequeue_time;
	}
	fprintf(fp,
	        "\n\n############################# STATISTICS ##########################\n");
	fprintf(fp,
	        "#                                                                 #\n");
	fprintf(fp,
	        "#                          Global Statistics                      #\n");
	fprintf(fp,
	        "#-----------------------------------------------------------------#\n");
	fprintf(fp,
	        "# Max Threads                        :%12d                #\n",
	        MAX_THREADS);
	fprintf(fp,
	        "# Total Threads                      :%12d                #\n",
	        MaxActiveThreads);
	fprintf(fp,
	        "# Total Allocated Memory             :%12lu bytes          #\n",
	        total_allocated_space);
	fprintf(fp,
	        "# Total Allocated Task Descriptors   :%12u                #\n",
	        total_allocated_task_descriptors);
#ifdef DEBUG_CNT_TASKS
	fprintf(fp,
	        "# Total Spawned Tasks                :%12u                #\n",
	        total_spawned_tasks);
	fprintf(fp,
	        "# Total Completed Tasks              :%12u                #\n",
	        total_complete_tasks);
	fprintf(fp,
	        "# Total Immediate Completed Tasks    :%12u                #\n",
	        total_immediate_executed_tasks);
#endif /* ifdef DEBUG_CNT_TASKS */
	fprintf(fp,
	        "#-----------------------------------------------------------------#\n");
	fprintf(fp,
	        "# Total Execution time               :%12.3lf ms             #\n",
	        total_exec_time / 1000.0);
	fprintf(fp,
	        "# Total Allocation time              :%12.3lf ms             #\n",
	        total_alloc_time / 1000.0);
	fprintf(fp,
	        "# Total Enqueue time                 :%12.3lf ms             #\n",
	        total_enqueue_time / 1000.0);
	fprintf(fp,
	        "# Total Dequeue time                 :%12.3lf ms             #\n",
	        total_dequeue_time / 1000.0);
	fprintf(fp,
	        "#-----------------------------------------------------------------#\n");
	fprintf(fp,
	        "#                                                                 #\n");
	fprintf(fp,
	        "#                          Workers Statistics                     #\n");
	fprintf(fp,
	        "#-----------------------------------------------------------------#\n");
	for (i = 0; i < MaxActiveThreads; i++) {
		fprintf(fp,
		        "#                             Worker #%2d                          #\n",
		        i);
		fprintf(fp,
		        "#-----------------------------------------------------------------#\n");
		fprintf(fp,
		        "# Physical_Core                     :%12d                 #\n",
		        thread_data[i].phys_core);
		fprintf(fp,
		        "# PID                               :%12d                 #\n",
		        (int)thread_data[i].pid);
		fprintf(fp,
		        "# Queue Size                        :%12d                 #\n",
		        MAX_QUEUE_ENTRIES);
		fprintf(fp,
		        "# Thread Enqueues                   :%12u                 #\n",
		        thread_data[i].total_enqueues);
		fprintf(fp,
		        "# Thread Dequeues                   :%12u                 #\n",
		        thread_data[i].total_dequeues);
		fprintf(fp,
		        "# Thread Spawned Tasks              :%12u                 #\n",
		        thread_data[i].total_spawned_tasks);
		fprintf(fp,
		        "# Thread Scheduled Tasks            :%12u                 #\n",
		        thread_data[i].total_scheduled_tasks);
		fprintf(fp,
		        "# Thread Task Descriptors Created   :%12u                 #\n",
		        thread_data[i].total_task_descriptors_created);
		fprintf(fp,
		        "# Thread Immediate Completed Tasks  :%12u                 #\n",
		        thread_data[i].total_immediate_executed_tasks);
		fprintf(fp,
		        "# Thread Steals                     :%12u                 #\n",
		        thread_data[i].total_steals);
		fprintf(fp,
		        "# Thread Steal Attempts             :%12u                 #\n",
		        thread_data[i].total_steal_attempts);
		fprintf(fp,
		        "# Thread Completed Tasks            :%12u                 #\n",
		        thread_data[i].total_tasks);
		fprintf(fp,
		        "#-----------------------------------------------------------------#\n");
		fprintf(fp,
		        "# Thread up time                    :%12.3lf ms              #\n",
		        (thread_data[i].thread_stop_time -
		         thread_data[i].thread_start_time) / 1000.0);
		fprintf(fp,
		        "# Thread execution time             :%12.3lf ms              #\n",
		        thread_data[i].exec_time / 1000.0);
		fprintf(fp,
		        "# Average execution time per task   :%12.3lf ms              #\n",
		        (thread_data[i].exec_time / 1000.0) /
		        thread_data[i].total_tasks);
		fprintf(fp,
		        "# Thread highest execution time     :%12.3lf ms              #\n",
		        (thread_data[i].highest_exec_task_time) / 1000.0);
		fprintf(fp,
		        "# Thread runtime time               :%12.3lf ms              #\n",
		        thread_data[i].total_runtime_time / 1000.0);
		fprintf(fp,
		        "# Thread idle time                  :%12.3lf ms              #\n",
		        thread_data[i].total_idle_time / 1000.0);
		fprintf(fp,
		        "# Thread dependence analysis time   :%12.3lf ms              #\n",
		        thread_data[i].total_dep_analysis_time / 1000.0);
		fprintf(fp,
		        "# Thread synchronization time       :%12.3lf ms              #\n",
		        thread_data[i].total_sync_time / 1000.0);
		fprintf(fp,
		        "# Thread enqueue time               :%12.3lf ms              #\n",
		        thread_data[i].total_enqueue_time / 1000.0);
		fprintf(fp,
		        "# Average enqueue time              :%12.3lf ms              #\n",
		        (thread_data[i].total_enqueue_time / 1000.0) /
		        thread_data[i].total_enqueues);
		fprintf(fp,
		        "# Thread dequeue time               :%12.3lf ms              #\n",
		        thread_data[i].total_dequeue_time / 1000.0);
		fprintf(fp,
		        "# Average dequeue time              :%12.3lf ms              #\n",
		        (thread_data[i].total_dequeue_time / 1000.0) /
		        thread_data[i].total_dequeues);
		fprintf(fp,
		        "# Thread allocate time              :%12.3lf ms              #\n",
		        thread_data[i].total_alloc_time / 1000.0);
		fprintf(fp,
		        "#-----------------------------------------------------------------#\n");
	}
	fprintf(fp,
	        "#                                                                 #\n");
	fprintf(fp,
	        "###################################################################\n\n");

	FILE *csvfp = fopen("stats.csv", "w");

	fprintf(csvfp, "Global Statistics\n");
	fprintf(csvfp, "Max Threads,%d\n", MAX_THREADS);
	fprintf(csvfp, "Total Threads,%d\n", MaxActiveThreads);
	fprintf(csvfp, "Total Allocated Memory,%lu bytes\n",
	        total_allocated_space);
	fprintf(csvfp, "Total Allocated Task Descriptors,%u\n",
	        total_allocated_task_descriptors);
#ifdef DEBUG_CNT_TASKS
	fprintf(csvfp, "Total Spawned Tasks,%u\n", total_spawned_tasks);
	fprintf(csvfp, "Total Completed Tasks,%u\n", total_complete_tasks);
	fprintf(csvfp, "Total Immediate Completed Tasks,%u\n",
	        total_immediate_executed_tasks);
#endif /* ifdef DEBUG_CNT_TASKS */
	fprintf(csvfp, "Total execution time,%.3lf ms\n",
	        total_exec_time / 1000.0);
	fprintf(csvfp, "Total Allocation time,%.3lf ms\n",
	        total_alloc_time / 1000.0);
	fprintf(csvfp, "Total Enqueue time,%.3lf ms\n",
	        total_enqueue_time / 1000.0);
	fprintf(csvfp, "Total Dequeue time,%.3lf ms\n",
	        total_dequeue_time / 1000.0);
	fprintf(csvfp, "Workers Statistics\n");
	for (i = 0; i < MaxActiveThreads; i++) {
		fprintf(csvfp, "Worker #%d\n", i);
		fprintf(csvfp, "Physical_Core,%d\n", thread_data[i].phys_core);
		fprintf(csvfp, "PID,%d\n", (int)thread_data[i].pid);
		fprintf(csvfp, "Queue Size,%d\n", MAX_QUEUE_ENTRIES);
		fprintf(csvfp, "Thread Enqueues,%u\n",
		        thread_data[i].total_enqueues);
		fprintf(csvfp, "Thread Dequeues,%u\n",
		        thread_data[i].total_dequeues);
		fprintf(csvfp, "Thread Spawned Tasks,%u\n",
		        thread_data[i].total_spawned_tasks);
		fprintf(csvfp, "Thread Scheduled Tasks,%u\n",
		        thread_data[i].total_scheduled_tasks);
		fprintf(csvfp, "Thread Immediate Completed Tasks,%u\n",
		        thread_data[i].total_immediate_executed_tasks);
		fprintf(csvfp, "Thread Steals,%u\n",
		        thread_data[i].total_steals);
		fprintf(csvfp, "Thread Steal Attempts,%u\n",
		        thread_data[i].total_steal_attempts);
		fprintf(csvfp, "Thread Complete Tasks,%u\n",
		        thread_data[i].total_tasks);
		fprintf(csvfp, "Thread up time,%.3lf ms\n",
		        (thread_data[i].thread_stop_time -
		         thread_data[i].thread_start_time) / 1000.0);
		fprintf(csvfp, "Thread execution time,%.3lf ms\n",
		        thread_data[i].exec_time / 1000.0);
		fprintf(csvfp, "Average execution time per task,%.3lf ms\n",
		        (thread_data[i].exec_time / 1000.0) /
		        thread_data[i].total_tasks);
		fprintf(csvfp, "Thread highest execution time,%.3lf ms\n",
		        (thread_data[i].highest_exec_task_time) / 1000.0);
		fprintf(csvfp, "Thread runtime time,%.3lf ms\n",
		        thread_data[i].total_runtime_time / 1000.0);
		fprintf(csvfp, "Thread idle time,%.3lf ms\n",
		        thread_data[i].total_idle_time / 1000.0);
		fprintf(csvfp, "Thread dependence analysis time,%.3lf ms\n",
		        thread_data[i].total_dep_analysis_time / 1000.0);
		fprintf(csvfp, "Thread synchronization time,%.3lf ms\n",
		        thread_data[i].total_sync_time / 1000.0);
		fprintf(csvfp, "Thread enqueue time,%.3lf ms\n",
		        thread_data[i].total_enqueue_time / 1000.0);
		fprintf(csvfp, "Average enqueue time,%.3lf ms\n",
		        (thread_data[i].total_enqueue_time / 1000.0) /
		        thread_data[i].total_enqueues);
		fprintf(csvfp, "Thread dequeue time,%.3lf ms\n",
		        thread_data[i].total_dequeue_time / 1000.0);
		fprintf(csvfp, "Average dequeue time,%.3lf ms\n",
		        (thread_data[i].total_dequeue_time / 1000.0) /
		        thread_data[i].total_dequeues);
		fprintf(csvfp, "Thread allocate time,%.3lf ms\n",
		        thread_data[i].total_alloc_time / 1000.0);
	}
	fclose(csvfp);
} /* print_stats */

/**
 * @brief Resets the statistics.
 *
 * @param thread_data
 */
void reset_threads_stats(pthread_data_t *thread_data)
{
	thread_data->total_enqueues                 = 0;
	thread_data->total_dequeues                 = 0;
	thread_data->total_spawned_tasks            = 0;
	thread_data->total_task_descriptors_created = 0;
	thread_data->total_immediate_executed_tasks = 0;
	thread_data->total_steals                   = 0;
	thread_data->total_steal_attempts           = 0;
	thread_data->thread_stop_time               = 0;
	thread_data->thread_start_time              = 0;
	thread_data->exec_time                      = 0;
	thread_data->total_tasks                    = 0;
	thread_data->highest_exec_task_time         = 0;
	thread_data->total_runtime_time             = 0;
	thread_data->total_idle_time                = 0;
	thread_data->total_enqueue_time             = 0;
	thread_data->total_dequeue_time             = 0;
	/*thread_data->total_alloc_time = 0;*/
}

#else /* ifdef ENABLE_STATS */
#define print_stats(x)
#endif /* ifdef ENABLE_STATS */
