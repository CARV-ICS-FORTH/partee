/**
 * @file list.c
 * @brief This file describes the implementation of a Double Linked List.
 * @author Nikos Papakonstantinou <nikpapac@ics.forth.gr>
 * @version v1.0
 * @date 2014-05-08
 */
#include <string.h>
#include <list.h>

void new_List(list_t **List)
{
	*List = (list_t*)malloc( sizeof(list_t) );
	bzero( *List, sizeof(list_t) );
	assert(*List);
}

void init_List(list_t *List)
{
	assert(List);
	bzero( List, sizeof(list_t) );
}

void release_List(list_t *List)
{
	assert(List);
	/*assert( isEmpty_List(List) );*/
	free(List);
}

static inline uint8_t isEmpty_List(list_t *List)
{
	assert(List);
	return List->total_entries == 0;
}

static inline uint8_t isFull_List(list_t *List)
{
	assert(List);
	return List->total_entries == MAX_LIST_ENTRIES;
}

void insert_front(tpc_task_descriptor *task, list_t *List)
{
	list_entry_t *tmp;

	assert(task);
	assert(List);

	tmp = (list_entry_t*)malloc( sizeof(list_entry_t) );
	assert(tmp);
	bzero( tmp, sizeof(list_entry_t) );
	tmp->task = task;
	/* tmp->next = NULL;
	 * tmp->previous = NULL; */
	if ( isEmpty_List(List) ) {
		assert(List->head == NULL /* && List->tail == NULL */);
		List->head = tmp;
		/* List->tail = tmp; */
	} else {
		assert( !(List->head == NULL /* && List->tail == NULL */) );
		tmp->next = List->head;
		/* List->head->previous = tmp; */
		List->head = tmp;
	}
	List->total_entries++;
}

tpc_task_descriptor* get_front(list_t *List)
{
	tpc_task_descriptor *ret_val = NULL;
	list_entry_t        *tmp;

	assert(List);
	if ( isEmpty_List(List) )
		return NULL;

	ret_val    = List->head->task;
	tmp        = List->head;
	List->head = List->head->next;
	free(tmp);
	List->total_entries--;

	return ret_val;
}

#if 0
void insert_back(tpc_task_descriptor *task, list_t *List)
{
	list_entry_t *tmp;

	assert(task);
	assert(List);

	tmp = (list_entry_t*)malloc( sizeof(list_entry_t) );
	assert(tmp);
	tmp->task     = task;
	tmp->next     = NULL;
	tmp->previous = NULL;
	/*if( isEmpty_List( List ) ){*/
	if (List->head == NULL && List->tail == NULL) {
		List->head = tmp;
		List->tail = tmp;
	} else {
		tmp->previous    = List->tail;
		List->tail->next = tmp;
		List->tail       = List->tail->next;
	}
	List->total_entries++;
}

void remove_front(list_t *List)
{
	list_entry_t *tmp;

	assert(List);

	/*if( !isEmpty_List(List) ){*/
	if (List->total_entries != 0) {
		tmp        = List->head;
		List->head = List->head->next;
		free(tmp);
		List->total_entries--;
	}
}

void remove_back(list_t *List)
{
	list_entry_t *tmp;

	assert(List);
	/*if( !isEmpty_List(List) ){*/
	if (List->total_entries != 0) {
		tmp        = List->tail;
		List->tail = List->tail->previous;
		free(tmp);
		List->total_entries--;
	}
}

tpc_task_descriptor* get_back(list_t *List)
{
	tpc_task_descriptor *ret_val = NULL;
	list_entry_t        *tmp;

	assert(List);
	/*if( !isEmpty_List(List) ){*/
	if (List->total_entries != 0) {
		ret_val    = List->tail->task;
		tmp        = List->tail;
		List->tail = List->tail->previous;
		free(tmp);
		List->total_entries--;
	}
	return ret_val;
}

tpc_task_descriptor* get_this(list_entry_t *list_entry, list_t *List)
{
	tpc_task_descriptor *ret_val = NULL;

	assert(List);
	/*if( !isEmpty_List(List) ){*/
	if (List->total_entries != 0) {
		ret_val = list_entry->task;
		if (list_entry == List->head && list_entry == List->tail) {
			List->head = NULL;
			List->tail = NULL;
		} else if (list_entry == List->head &&
		           list_entry != List->tail) {
			list_entry->next->previous = NULL; /* list_entry->previous; */
		} else if (list_entry != List->head &&
		           list_entry == List->tail) {
			list_entry->previous->next = NULL; /* list_entry->next; */
		} else {
			list_entry->previous->next = list_entry->next;
			list_entry->next->previous = list_entry->previous;
		}
		free(list_entry);
		List->total_entries--;
	}
	return ret_val;
}

#endif /* if 0 */
