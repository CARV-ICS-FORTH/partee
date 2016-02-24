#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

void task_init(int *array, int size)
{
	int i;

	assert(array && size > 0);
	for (int i = 0; i < size; i++) {
		array[i] = 0;
	}
}

void task_set(int *array, int size)
{
	int i;

	assert(array && size > 0);
	for (int i = 0; i < size; i++) {
		array[i] = i * size;
	}
}

void task_print(int *array, int size)
{
	int i;

	assert(array && size > 0);
	printf("array = [");
	for (int i = 0; i < size; i++) {
		printf("%c%d ", (i == 0) ? ' ' : ',', array[i]);
	}
	printf("]\n");
}

void task_unset(int *array, int size)
{
	int i;

	assert(array && size > 0);
	for (int i = 0; i < size; i++) {
		array[i] = -1;
	}
}

int main(int argc, char **argv)
{
	int *array, *block;
	int array_size, block_size;
	int nThreads;
	int i;

	if (argc != 3) {
		printf("Usage:\n\t%s <array_size> <nthreads>\n", argv[0]);
		exit(1);
	}
	array_size = atoi(argv[1]);
	nThreads   = atoi(argv[2]);
	block_size = array_size / nThreads;
	assert(array_size > 0);
	assert(block_size > 0);
#pragma css start(nThreads)

	/* alloc spaces for the option data */
#pragma css malloc
	array = (int*)malloc( array_size * sizeof(int) );
#pragma css sync
	for (i = 0; i < nThreads; i++) {
		block = array + i * block_size;
#pragma css task inout(block[block_size]) in(block_size)
		task_init(block, block_size);
	}
	for (i = 0; i < nThreads; i++) {
		block = array + i * block_size;
#pragma css task inout(block[block_size]) in(block_size)
		task_set(block, block_size);
	}
#pragma css task in(array[array_size]) in(array_size)
	task_print(array, array_size);

/*#pragma css sync*/

	for (i = 0; i < nThreads; i++) {
		block = array + i * block_size;
#pragma css task inout(block[block_size]) in(block_size)
		task_unset(block, block_size);
	}

#pragma css task in(array[array_size]) in(array_size)
	task_print(array, array_size);
#pragma css finish
	return 0;
} /* main */
