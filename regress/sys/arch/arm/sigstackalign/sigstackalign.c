/*	$NetBSD: sigstackalign.c,v 1.1 2001/08/06 22:29:59 bjh21 Exp $       */

#include <sys/types.h>

__RCSID("$NetBSD: sigstackalign.c,v 1.1 2001/08/06 22:29:59 bjh21 Exp $");

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define RANGE 16
#define STACKALIGN 8
#define BLOCKSIZE (MINSIGSTKSZ + RANGE)

extern void getstackptr(int);

void *stackptr;

int
main(int argc, char **argv)
{
	char *stackblock;
	int i, ret;
	struct sigaction sa;
	stack_t ss;

	ret = 0;

	sa.sa_handler = getstackptr;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_ONSTACK;
	if (sigaction(SIGUSR1, &sa, NULL) != 0)
		err(1, "sigaction");

	stackblock = malloc(BLOCKSIZE);
	for (i = 0; i < RANGE; i++) {
		ss.ss_sp = stackblock;
		ss.ss_size = MINSIGSTKSZ + i;
		ss.ss_flags = 0;
		if (sigaltstack(&ss, NULL) != 0)
			err(1, "sigaltstack");
		kill(getpid(), SIGUSR1);
		if ((u_int)stackptr % STACKALIGN != 0) {
			fprintf(stderr, "Bad stack pointer %p\n", stackptr);
			ret = 1;
		}
#if 0
		printf("i = %d, stackptr = %p\n", i, stackptr);
#endif
	}

	return ret;
}
		
