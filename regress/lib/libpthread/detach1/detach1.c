/*	$NetBSD: detach1.c,v 1.1 2005/01/21 11:54:24 yamt Exp $	*/

#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define	NITER	100000

void *func(void *);
int main(void);

void *
func(void *dummy)
{

	return NULL;
}

int
main()
{
	int i;

	for (i = 0; i < NITER; i++) {
		pthread_t t;
		int error;

		if (pthread_create(&t, NULL, func, NULL)) {
			/*
			 * sleep and retry once for the case that
			 * the child threads are not finished yet.
			 */
			printf("%d sleeping...\n", i);
			sleep(10);
			if (pthread_create(&t, NULL, func, NULL))
				err(1, "create");
		}

		if (i & 1)
			sched_yield(); /* give a chance thread to finish */
		if (pthread_detach(t))
			err(1, "detach");

		error = pthread_join(t, NULL);
		if (error != ESRCH && error != EINVAL) {
			printf("unexpected error %d\n", error);
			exit(3);
		}
	}

	exit(0);
}
