/*	$NetBSD: context.c,v 1.1 2003/01/30 19:47:00 thorpej Exp $	*/

#include <assert.h>
#include <stdio.h>
#include <ucontext.h>

int
main(int argc, char *argv[])
{
	ucontext_t u, v, w;
	volatile int x, y;

	x = 0;
	y = 0;

	printf("Start\n");

	getcontext(&u);
	y++;

	printf(" X is %d\n", x);

	getcontext(&v);

	if ( x < 20 ) {
		x++;
		getcontext(&w);
		printf("Adding one and going around.\n");
		setcontext(&u);
	}

	printf("End, y = %d\n", y);
	assert(y == 21);

	return 0;
}
