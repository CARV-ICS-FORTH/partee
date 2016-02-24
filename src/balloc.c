#include <balloc.h>

void          *grid_t;
char          *Top;
unsigned long grid_size_g  = GRID_SZ; /* default grid size */
unsigned long block_size_g = BLOCK_SZ; /* default block size */
static inline void* newGrid()
{
	/*grid_t = (void *)memalign((size_t) grid_size_g, (size_t)
	 * grid_size_g);*/
	grid_t = (void*)malloc( (size_t)grid_size_g );
	assert(grid_t);
	bzero(grid_t, (size_t)grid_size_g);
	/*if( (unsigned long)grid_t % grid_size_g != 0 ){*/
	/*fprintf(stderr, "newGrid failed: grid could not be aligned \*/
	/*(%p %% 0x%lx != 0)\n", grid_t, grid_size_g);*/
	/*exit(1);*/
	/*}*/
	Top = grid_t;
	return grid_t;
}

void balloc_init(size_t size)
{
	grid_size_g = size;
	newGrid();
}

void balloc_finalize()
{
	assert(grid_t);
	free(grid_t);
}

void* balloc_alloc(size_t size)
{
	void       *ret_val;
	uint_ptr_t segment = size + block_size_g - (size % block_size_g);

	/* TODO: check if the size of grid is is ok */
	ret_val = Top;
	Top    += segment;
	return ret_val;
}

void bfree(void *mem, size_t size)
{
	/* TODO: Implement free */
}
