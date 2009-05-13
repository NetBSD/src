/* $NetBSD: findthreads.c,v 1.1.2.2 2009/05/13 19:18:48 jym Exp $ */

#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>

static void *
f(void *arg)
{

	sleep(1000);
	return 0;
}

#define NUM 100
int
main(int argc, char **argv)
{
	pthread_t thr[NUM];
	int seed, i, j, res, errors;
	char nam[20];
	struct rlimit sl;

	if (argc > 1)
		seed = atoi(argv[1]);
	else
		seed = time(0);
	srandom(seed);
	getrlimit(RLIMIT_STACK, &sl);

	errors = 0;
	for (i = 0; i < NUM; i++) {
		res = pthread_create(&thr[i], 0, f, 0);
		if (res)
			errx(1, "pthread_create: %s", strerror(res));
		for (j = 0; j <= i; j++) {
			res = pthread_getname_np(thr[j], nam, sizeof(nam));
			if (res) {
				warnx("getname(%d/%d): %s\n", i, j,
							strerror(res));
				errors++;
			}
		}
		if (errors)
			break;
		malloc((random() & 7) * sl.rlim_cur);
	}
	if (errors) {
		printf("%d errors\n", errors);
		if (argc <= 1)
			printf("seed was %d\n", seed);
	}
	return (!!errors);
}
