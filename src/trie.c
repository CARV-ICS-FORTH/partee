/**
 * @file trie.c
 * @brief Implementation of Trie Data structure
 * @author Nikos Papakonstantinou <nikpapac@ics.forth.gr>
 * @version v1.0
 * @date 2014-05-10
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <trie.h>

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

#define UP_BITS   10
#define DOWN_BITS 9
#define FIRST_LEVEL(x)  (x >> DOWN_BITS)
#define SECOND_LEVEL(x) ( x - (FIRST_LEVEL(x) << DOWN_BITS) )

extern uint_ptr_t total_allocated_space;

/**
 * @brief Allocates the appropriate space in memory for first level of Trie.
 *
 * @return Metadata_element ***
 */
#ifdef WITH_REGIONS

Metadata_element*** new_trie(region_t r)
{
#else /* ifdef WITH_REGIONS */

Metadata_element*** new_trie()
{
#endif /* ifdef WITH_REGIONS */

	Metadata_element ***Trie;

#ifdef WITH_REGIONS
	Trie =
	        (Metadata_element***)__rc_rstralloc0( r,
	                                              (1 << UP_BITS) *
	                                              sizeof(Metadata_element *
	                                                     *) );
#else /* ifdef WITH_REGIONS */
	Trie =
	        (Metadata_element***)malloc( (1 << UP_BITS) *
	                                     sizeof(Metadata_element * *) );
#endif /* ifdef WITH_REGIONS */
	assert(Trie);
#ifdef ENABLE_STATS
	total_allocated_space +=
	        ( (1 << UP_BITS) * sizeof(Metadata_element * *) );
#endif /* ifdef ENABLE_STATS */
#ifdef DEBUG_TRIE
	dprintf("First level created and initialized at %p\n", Trie);
#endif /* ifdef DEBUG_TRIE */
	return Trie;
}

/**
 * @brief Release whole allocated memory sequentially.
 *
 * @param Trie a pointer to trie.
 */
void release_trie(Metadata_element ***Trie)
{
	int i;

	assert(Trie);

	for (i = 0; i < (1 << UP_BITS); ++i) {
		free(Trie[i]);
	}
	free(Trie);
}

/**
 * @brief Get a pointer in root of Trie and an index of an array and returns the
 *                              corresponding pointer to Metadata_element if
 *exist. Otherwise, create
 *                              the pointer by allocating the appropriate space
 *and initializing it
 *                              with NULL.
 *
 * @param Trie  A pointer to trie.
 * @param index The index of seconds level array.
 *
 * @return Metadata_element *
 */
#ifdef WITH_REGIONS
Metadata_element* get_or_create_node(Metadata_element ***Trie, int index,
                                     region_t r)
{
#else /* ifdef WITH_REGIONS */

Metadata_element* get_or_create_node(Metadata_element ***Trie, int index)
{
#endif /* ifdef WITH_REGIONS */
	assert(Trie);
#ifdef DEBUG_TRIE
	dprintf("Trie:%p, index:%d.\n", Trie, index);
#endif /* ifdef DEBUG_TRIE */
	if (!Trie[FIRST_LEVEL(index)]) {
		Trie[FIRST_LEVEL(index)] =
#ifdef WITH_REGIONS
		        (Metadata_element**)__rc_rstralloc0( r,
		                                             (1 << DOWN_BITS) *
		                                             sizeof(
		                                                     Metadata_element
		                                                     *) );
#else /* ifdef WITH_REGIONS */
		        (Metadata_element**)malloc( (1 << DOWN_BITS) *
		                                    sizeof(Metadata_element*) );
#endif /* ifdef WITH_REGIONS */
		assert(Trie[FIRST_LEVEL(index)]);
#ifdef ENABLE_STATS
		total_allocated_space +=
		        ( (1 << DOWN_BITS) * sizeof(Metadata_element*) );
#endif /* ifdef ENABLE_STATS */
#ifdef DEBUG_TRIE
		dprintf("Create Second level at %p[%d] :: %p = %p\n", Trie,
		        ( FIRST_LEVEL(index) ), &Trie[FIRST_LEVEL(index)],
		        Trie[FIRST_LEVEL(index)]);
#endif /* ifdef DEBUG_TRIE */
	}
#ifdef DEBUG_TRIE
	dprintf("Return Trie:%p[%d]= %p[%d] :: %p == %p.\n", Trie,
	        FIRST_LEVEL(index), Trie[FIRST_LEVEL(index)], SECOND_LEVEL(
	                index),
	        &(Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)]),
	        Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)]);
#endif /* ifdef DEBUG_TRIE */
	return Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)];
}

/**
 * @brief Get a pointer in root of Trie and an index of an array and returns the
 *                              corresponding pointer to Metadata_element if
 *exist, otherwise, return
 *                              NULL.
 *
 * @param Trie
 * @param index
 *
 * @return Metadata_element *
 */
Metadata_element* get_node(Metadata_element ***Trie, int index)
{
	assert(Trie);
#ifdef DEBUG_TRIE
	dprintf("Trie:%p, index:%d.\n", Trie, index);
#endif /* ifdef DEBUG_TRIE */
#ifdef DEBUG_TRIE
	if (Trie[FIRST_LEVEL(index)])
		dprintf("Return Trie:%p[%d]= %p[%d] :: %p == %p.\n", Trie, FIRST_LEVEL(
		                index), Trie[FIRST_LEVEL(index)], SECOND_LEVEL(
		                index), &(Trie[FIRST_LEVEL(index)][SECOND_LEVEL(
		                                                           index)
		                          ]),
		        Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)]);
	else
		dprintf("Return Trie:%p[%d]= %p.\n", Trie, FIRST_LEVEL(
		                index), Trie[FIRST_LEVEL(index)]);

#endif /* ifdef DEBUG_TRIE */
	return (Trie[FIRST_LEVEL(index)]) ? (Trie[FIRST_LEVEL(index)][
	                                             SECOND_LEVEL(index)]) : (
	        NULL);
}

/**
 * @brief Get a pointer at the root of Trie and an index and sets the new_node
 *                              to the coresponding place in the Trie.
 *
 * @param Trie                  Pointer to Trie's root.
 * @param index         Index to calculate the right place.
 * @param new_node  Pointer to the new node.
 */
void set_node(Metadata_element ***Trie, int index, Metadata_element *new_node)
{
	assert(Trie);
#ifdef DEBUG_TRIE
	dprintf("Set Trie:%p[%d]= %p[%d] :: %p == %p = %p.\n", Trie, FIRST_LEVEL(
	                index), Trie[FIRST_LEVEL(index)], SECOND_LEVEL(index),
	        &(Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)]),
	        Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)], new_node);
#endif /* ifdef DEBUG_TRIE */
	Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)] = new_node;
#ifdef DEBUG_TRIE
	dprintf("Set Trie:%p[%d]= %p[%d] :: %p = %p.\n", Trie, FIRST_LEVEL(
	                index), Trie[FIRST_LEVEL(index)], SECOND_LEVEL(index),
	        &(Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)]),
	        Trie[FIRST_LEVEL(index)][SECOND_LEVEL(index)]);
#endif /* ifdef DEBUG_TRIE */
}
