#ifndef _BALLOC_H_
#define _BALLOC_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <config.h>

/* extern void * grid_t; */
/* extern char * top; */
/* extern unsigned long grid_size_g;    // default grid size */
/* extern unsigned long block_size_g;           // default block size */

void balloc_init(size_t);

void balloc_finalize();

void* balloc_alloc(size_t);

void bfree(void*, size_t);


#endif /* ifndef _BALLOC_H_ */
