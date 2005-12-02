/* $NetBSD: fpu.c,v 1.2 2005/12/02 16:44:54 is Exp $ */

/*
 * This is adapted from part of csw/cstest of the MPD implementation by
 * the University of Arizona CS department (http://www.cs.arizona.edu/sr/)
 * which is in the public domain:
 *
 * "The MPD system is in the public domain and you may use and distribute it
 *  as you wish.  We ask that you retain credits referencing the University
 *  of Arizona and that you identify any changes you make.
 *  
 *  We can't provide a warranty with MPD; it's up to you to determine its
 *  suitability and reliability for your needs.  We would like to hear of
 *  any problems you encounter but we cannot promise a timely correction."
 *
 * It was changed to use pthread_create() and sched_yield() instead of
 * the internal MPD context switching primitives by Ignatios Souvatzis
 * <is@netbsd.org>.
 */

#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N_RECURSE 10

double mul3(double, double, double);
void *stir(void *);
void *bar(void *);
void dotest(void);

#define RCTEST(code, mess) if (rc) {fprintf(stderr, "%s\n", mess); exit(1);}
#define YIELD() if (sched_yield()) {fprintf(stderr, "sched_yield"); exit(1);}
#define LOCK(l) if (pthread_mutex_lock(l)) {fprintf(stderr, "lock"); exit(1);}
#define UNLOCK(l) if (pthread_mutex_unlock(l)) {fprintf(stderr, "unlock"); exit(1);}

int recursion_depth = 0;
pthread_mutex_t recursion_depth_lock;
pthread_t s5;
double stirseed[] = {1.7, 3.2, 2.4};
int verbose = 0;

#define Dprintf(x) if(verbose) printf(x)

int
main(int argc, char *argv[]) {

	int rc;

	printf("Testing threaded floating point computations...");
	if (argc > 1  && argv[1][0] == '-' && argv[1][1] == 'v') {
		verbose = 1;
		printf("\n");
	}
	rc = pthread_mutex_init(&recursion_depth_lock, 0);
	if (0 != rc) {
		fprintf(stderr, "pthread_mutex_init");
		exit(1);
	}
	pthread_create(&s5, 0, stir, stirseed);
	dotest();

	fprintf(stderr, "\n exiting from main\n");
	exit(1);
}

void *
stir(void *p) {
	double *q = (double *)p;
	double x = *q++;
	double y = *q++;
	double z = *q++;
	for (;;) {
		Dprintf("stirring...");
		x = sin ((y = cos (x + y + .4)) - (z = cos (x + z + .6)));
		YIELD();
	}
}

double mul3(double x, double y, double z) {
	Dprintf("mul3...");
	YIELD();
	Dprintf("returning...\n");
	return x * y * z;
}

void *bar(void *p) {
	double d;
	int rc;

	d = mul3(mul3(2., 3., 5.), mul3(7., 11., 13.),
	    mul3(17., 19., 23.));

	if (d != 223092870.) {
		printf("\noops - product returned was %.20g\nFAILED\n", d);
		exit(1);
	}

	LOCK(&recursion_depth_lock);
	rc = recursion_depth++;
	UNLOCK(&recursion_depth_lock);
	if (rc < N_RECURSE) {
		dotest();
	}
	Dprintf("\n");
	printf("OK\n");
	exit(0);
}

void dotest() {
	pthread_t s2;
	Dprintf("recursing...");
	pthread_create(&s2, 0, bar, 0);
	sleep(20); /* XXX must be long enough for our slowest machine */
}
