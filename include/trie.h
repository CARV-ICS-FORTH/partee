#ifndef __TRIE_H__
#define __TRIE_H__

#include <metadata.h>
#include <regions.h>

#ifdef WITH_REGIONS

Metadata_element*** new_trie(region_t);

#else /* ifdef WITH_REGIONS */

Metadata_element*** new_trie();

#endif /* ifdef WITH_REGIONS */

void release_trie(Metadata_element***);

#ifdef WITH_REGIONS

Metadata_element* get_or_create_node(Metadata_element***, int, region_t);

#else /* ifdef WITH_REGIONS */

Metadata_element* get_or_create_node(Metadata_element***, int);

#endif /* ifdef WITH_REGIONS */

Metadata_element* get_node(Metadata_element***, int);

void set_node(Metadata_element***, int, Metadata_element*);

#endif /* ifndef __TRIE_H__ */
