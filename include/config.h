#ifndef __CONFIG_H__
#define __CONFIG_H__

/* #define DEBUG_FILES */

/* #define DEBUG_WORKER */
/* #define DEBUG_SYNC */
/* #define DEBUG_THREADS */
/* #define DEBUG_QUEUE */
/* #define DEBUG_LIST */
/* #define DEBUG_TASK_ALLOC */
/* #define DEBUG_ALLOC */
/* #define DEBUG_DEPS */
/* #define DEBUG_TRIE */
/* #define DEBUG_CNT_TASKS */

#ifdef __i386__
#define uint_ptr_t uint32_t
#endif /* ifdef __i386__ */

#ifdef __x86_64__
#define uint_ptr_t uint64_t
#endif /* ifdef __x86_64__ */

/* #define ENABLE_STATS */
#define BACKOFF

#define MAX_THREADS       64
#define MAX_QUEUE_ENTRIES 128

#define CACHE_LINE 64U

#define BLOCK_SZ (2048UL) /* TOFIX */
#define GRID_SZ  (1024UL * 1024UL * 1024UL)

#ifdef BACKOFF
#if !defined (BACKOFF_MIN)
#define BACKOFF_MIN (2)
#endif /* if !defined (BACKOFF_MIN) */
#if !defined (BACKOFF_MAX)
#define BACKOFF_MAX (BACKOFF_MIN << 8)
#endif /* if !defined (BACKOFF_MAX) */
#endif /* ifdef BACKOFF */

extern __thread uint32_t thread_id;

#define tprintf(format, ...)                  \
	fprintf(stderr, "Thread %d: " format, \
	        thread_id, ## __VA_ARGS__); fflush(stdout)


#endif /* ifndef __CONFIG_H__ */
