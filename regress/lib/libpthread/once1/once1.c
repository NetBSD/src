/*	$NetBSD: once1.c,v 1.1 2003/01/30 19:32:00 thorpej Exp $	*/

#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_once_t once = PTHREAD_ONCE_INIT;
static int x;

void ofunc(void);

int
main(int argc, char *argv[])
{

	printf("1: Test 1 of pthread_once()\n");

	pthread_once(&once, ofunc);
	pthread_once(&once, ofunc);

	printf("1: X has value %d\n",x );
	assert(x == 1);

	return 0;
}

void
ofunc(void)
{

	printf("Variable x has value %d\n", x);
	x++;
}
