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
		array[i] = i;
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

int main(int argc, char **argv)
{
	int *array;
	int array_size;
	int nThreads;
	int i;

	if (argc != 3) {
		printf("Usage:\n\t%s <array_size> <nthreads>\n", argv[0]);
		exit(1);
	}
	array_size = atoi(argv[1]);
	nThreads   = atoi(argv[2]);
	assert(array_size > 0);
#pragma css start(nThreads)

	/* alloc spaces for the option data */
#pragma css malloc
	array = (int*)malloc( array_size * sizeof(int) );
#pragma css sync
#pragma css task inout(array[array_size]) in(array_size) safe(array_size)
	task_init(array, array_size);
#pragma css task inout(array[array_size]) in(array_size) safe(array_size)
	task_print(array, array_size);
#pragma css finish
	return 0;
}
