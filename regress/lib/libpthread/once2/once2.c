/*	$NetBSD: once2.c,v 1.1 2003/01/30 19:32:00 thorpej Exp $	*/

#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_once_t once = PTHREAD_ONCE_INIT;
static int x;

#define NTHREADS 25

void ofunc(void);
void* threadfunc(void *);

int
main(int argc, char *argv[])
{
	pthread_t  threads[NTHREADS];
	int id[NTHREADS];
	int i;

	printf("1: Test 2 of pthread_once()\n");

	for (i=0; i < NTHREADS; i++) {
		id[i] = i;
		pthread_create(&threads[i], NULL, threadfunc, &id[i]);
	}

	for (i=0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);

	printf("1: X has value %d\n",x );
	assert(x == 2);

	return 0;
}


void
ofunc(void)
{
	x++;
	printf("ofunc: Variable x has value %d\n", x);
	x++;
}

void *
threadfunc(void *arg)
{
	int num;

	pthread_once(&once, ofunc);

	num = *(int *)arg;
	printf("Thread %d sees x with value %d\n", num, x);
	assert(x == 2);

	return NULL;
}
