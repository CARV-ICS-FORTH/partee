#ifndef __METADATA_H__
#define __METADATA_H__

#include <stdint.h>
#include <list.h>
#include <partee.h>

struct Metadata_element {
	/* Memory operation type: IN, OUT, INOUT, SAFE */
	uint32_t type;

	/* uint32_t level; */
#ifdef DEBUG_DEPS

	uint32_t waiting_tasks;
	uint32_t total_readers;

#endif /* ifdef DEBUG_DEPS */

	struct list_t               *readers;
	struct _tpc_task_descriptor *owner;
	struct Metadata_element     *dependent_metadata;
};

typedef struct Metadata_element Metadata_element;

#endif /* ifndef __METADATA_H__ */
