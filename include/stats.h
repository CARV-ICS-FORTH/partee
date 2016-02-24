#ifndef _STATS_H_
#define _STATS_H_

double total_exec_time;
long   total_start_time;
long   total_stop_time;
double total_alloc_time;
long   total_start_time_alloc;
long   total_stop_time_alloc;

long usecs(void);
void print_stats(FILE*);

/* void reset_thread_stats(void); */

#endif /* ifndef _STATS_H_ */
