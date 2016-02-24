#include <stdio.h>
#include <stdlib.h>

void Fibonacci(int *n_out, int n)
{
	if (n == 0) {
		*n_out = 0;
	} else if (n == 1) {
		*n_out = 1;
	} else {
		int tmp_n_1        = n - 1;
		int tmp_n_2        = n - 2;
		int tmp_n_1_out    = 42;
		int tmp_n_2_out    = 42;
		int *__tmp_n_1_out = &tmp_n_1_out;
		int *__tmp_n_2_out = &tmp_n_2_out;

#pragma css task out(__tmp_n_1_out) in(tmp_n_1)
		Fibonacci(__tmp_n_1_out, tmp_n_1);
#pragma css task out(__tmp_n_2_out) in(tmp_n_2)
		Fibonacci(__tmp_n_2_out, tmp_n_2);
#pragma css sync
		*n_out = (tmp_n_1_out + tmp_n_2_out);
	}
}

int main(int argc, char **argv)
{
	int n, i = 0, c, threads;
	int n_out        = 42;
	int *__tmp_n_out = &n_out;

	if (argc != 3) {
		fprintf(stderr, "Usage:\n\t%s <number> <threads>\n", argv[0]);
		exit(0);
	}
	n       = atoi(argv[1]);
	threads = atoi(argv[2]);

#pragma css start(threads)
	;
#pragma css task out(__tmp_n_out) in(n)
	Fibonacci(__tmp_n_out, n);
#pragma css finish
	printf("Fibonacci series: %d\n", n_out);

	return 0;
}
