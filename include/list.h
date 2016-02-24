#ifndef __LIST_H__
#define __LIST_H__

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <partee.h>
#include <config.h>

#define MAX_LIST_ENTRIES 128

struct _list_entry_t {
	struct _tpc_task_descriptor *task;
	uint32_t                    task_id;
	struct _list_entry_t        *next;

	/* struct _list_entry_t * previous; */
} __attribute__( ( aligned(CACHE_LINE) ) );

typedef struct _list_entry_t list_entry_t;

struct _list_t {
	uint32_t     total_entries;
	list_entry_t *head;

	/* list_entry_t *tail; */
} __attribute__( ( aligned(CACHE_LINE) ) );

typedef struct _list_t list_t;

void new_List(list_t**);

void init_List(list_t*);

void release_List(list_t*);

void insert_front(struct _tpc_task_descriptor*, list_t*);

struct _tpc_task_descriptor* get_front(list_t*);

#if 0
void remove_front(list_t*);

void remove_back(list_t*);

struct _tpc_task_descriptor* get_back(list_t*);
struct _tpc_task_descriptor* get_this(list_entry_t*, list_t*);

#endif /* if 0 */

#endif /* ifndef __LIST_H__ */
