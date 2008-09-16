/*      $NetBSD: sigrunning.c,v 1.1.10.2 2008/09/16 18:49:33 bouyer Exp $        */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int pthread__maxconcurrency, pthread__concurrency;

pthread_mutex_t 	start_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		start_cond = PTHREAD_COND_INITIALIZER;

/*
 * Define some infrastructure so we can wrap lots of tests in
 * one program.
 */
struct testInfo;

typedef intptr_t (testRunner_t)(pthread_cond_t *);
typedef int (testStopper_t)(pthread_t);

struct testInfo {
	testRunner_t	*ti_runner;	/* in thread 2, runs test & reports */
	testStopper_t	*ti_stopper;	/* in thread 1, sends signal */
	char		*ti_name;
};

/*
 * Now define a signal handler that just incriments a global
 */
int	sigCounter;

void doIncCounter(int sig, siginfo_t *info, void *ctx);

extern int __pthread_running_kills;
void
doIncCounter(int sig, siginfo_t *info, void *ctx)
{
	if (__pthread_running_kills)
		sigCounter++;
}

/*
 * Now make the test thread
 *
 * First build a routine that gobbles an amount of CPU. Then build a
 * routine on top of it to test the pthread function we want to test.
 */

int	 scratch;
void	 doStore(int);
void	 burnCPU(int);
void	*runnerThread(void *);

/* Routine to help ensure that the compiler doesn't optimize away our loops */
void
doStore(int foo)
{
	scratch = foo;
}

void
burnCPU(int m)
{
	int i;

	for(i = 0; i < m; i++)
		doStore(i);
}


void *
runnerThread(void *v)
{
	struct testInfo 	*ti = v;

	/* Interlock w/ main thread */
	pthread_mutex_lock(&start_mtx);
	pthread_mutex_unlock(&start_mtx);

	return (void *)ti->ti_runner(&start_cond);
}

/*
 * Now that we have infrastructure, start making tests
 */

/* First a simple kill routine */
testStopper_t	justKill;
testStopper_t	waitKill;

int
justKill(pthread_t t)
{
	return pthread_kill(t, SIGUSR1);
}

/*
 * Now an interlocked kill. We wait for the running thread to tell us
 * it's about to start eating CPU, so we can now send a signal to its
 * running self.
 */
int
waitKill(pthread_t t)
{
	while (sigCounter == 0) {
		pthread_mutex_lock(&start_mtx);
		pthread_cond_wait(&start_cond, &start_mtx);
		pthread_mutex_unlock(&start_mtx);
		if (sigCounter == 0)
			pthread_kill(t, SIGUSR1);
	}

	return 0;
}

/*
 * Now test sched_yield() noticing signals
 */
testRunner_t schedYieldRunner;

struct testInfo schedYieldInfo = {
	schedYieldRunner,
	justKill,
	"sched_yield() test"
};

intptr_t
schedYieldRunner(pthread_cond_t *c)
{
	pthread_cond_signal(c);

	while (sigCounter == 0) {
		burnCPU(1000000);
		sched_yield();
	}

	return 0;
}

/*
 * Now test pthread_cond_wait().
 * We create two threads, and have them ping-pong back and forth on
 * a condvar. This lets us test what happens as we sleep on it.
 *
 * Other tests also use these variables.
 */
pthread_mutex_t 	 pingpong_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		 pingpong_cond = PTHREAD_COND_INITIALIZER;
int			 pingpong_running = 1;
void			*echoBack(void *);

testRunner_t condWaitRunner;
testRunner_t condTimedWaitRunner;

struct testInfo condWaitInfo = {
	condWaitRunner,
	waitKill,
	"pthread_cond_wait() test"
};
struct testInfo condTimedWaitInfo = {
	condTimedWaitRunner,
	waitKill,
	"pthread_cond_timedwait() test"
};

/*
 * Now the routine that runs in the third thread. It just locks the pingpong
 * mutex, sleeps on the condvar, wakes the condvar when it gets woken,
 * then sleeps again if we're still running.
 */
void *
echoBack(void *v)
{
	pthread_mutex_lock(&pingpong_mtx);
	while (pingpong_running) {
		pthread_cond_signal(&pingpong_cond);
		pthread_cond_wait(&pingpong_cond, &pingpong_mtx);
	}
	pthread_mutex_unlock(&pingpong_mtx);
	return NULL;
}

intptr_t
condWaitRunner(pthread_cond_t *c)
{
	pthread_t	pp;
	int		i;
	/*
	 * Make the pingpong thread before releasing the main
	 * thread
	 */
	pthread_mutex_lock(&pingpong_mtx);
	if ((i = pthread_create(&pp, NULL, echoBack, NULL)))
		errx(1, "condWaitRunner: pthread_create returned %d", i);

	pthread_cond_wait(&pingpong_cond, &pingpong_mtx);
	pthread_cond_signal(c);
	burnCPU(1000);	// Increase chances that main thread sees concurrency

	/* Now for the test. */
	while (sigCounter == 0) {
		/* Wake the kill-delivering thread if sleeping */
		pthread_cond_signal(c);
		/* Now wake echoBack */
		pthread_cond_signal(&pingpong_cond);

		/* Now burn CPU and wait for our wake */
		burnCPU(1000000);
		pthread_cond_wait(&pingpong_cond, &pingpong_mtx);
	}

	/* Now clean up */
	pingpong_running = 0;
	pthread_mutex_unlock(&pingpong_mtx);
	pthread_cond_signal(&pingpong_cond);
	pthread_cond_signal(c);
	if ((i = pthread_join(pp, NULL)) != 0)
		errx(1, "condWaitRunner: pthread_join returned %d", i);

	return 0;
}

/* Now do the same thing w/ pthread_cond_timedwait() */
intptr_t
condTimedWaitRunner(pthread_cond_t *c)
{
	pthread_t		pp;
	int			i;
	struct timespec 	ts;
	struct timeval		tv;
	/*
	 * Make the pingpong thread before releasing the main
	 * thread
	 */
	pthread_mutex_lock(&pingpong_mtx);
	if ((i = pthread_create(&pp, NULL, echoBack, NULL)))
		errx(1, "condWaitRunner: pthread_create returned %d", i);

	pthread_cond_wait(&pingpong_cond, &pingpong_mtx);
	pthread_cond_signal(c);
	burnCPU(1000);	// Increase chances that main thread sees concurrency

	/* Prep the timeout. Just set it a silly-amount in the future */
	gettimeofday(&tv, NULL);
	tv.tv_sec += 24*60*60;		// Tomorrow...
	TIMEVAL_TO_TIMESPEC(&tv, &ts);

	/* Now for the test. */
	while (sigCounter == 0) {
		/* Wake the kill-delivering thread if sleeping */
		pthread_cond_signal(c);
		/* Now wake echoBack */
		pthread_cond_signal(&pingpong_cond);

		/* Now burn CPU and wait for our wake */
		burnCPU(1000000);
		pthread_cond_timedwait(&pingpong_cond, &pingpong_mtx, &ts);
	}

	/* Now clean up */
	pingpong_running = 0;
	pthread_mutex_unlock(&pingpong_mtx);
	pthread_cond_signal(&pingpong_cond);
	pthread_cond_signal(c);
	if ((i = pthread_join(pp, NULL)) != 0)
		errx(1, "condWaitRunner: pthread_join returned %d", i);

	return 0;
}

/*
 * And now for the main thread.
 */

struct testInfo *tests[] = {
	&schedYieldInfo,
	&condWaitInfo,
	&condTimedWaitInfo,
};

#define NUMTESTS (sizeof(tests)/sizeof(struct testInfo *))

void usage(void);

void
usage(void)
{
	int	i;

	fprintf(stderr, "usage: sigrunning <testnum> where testnum "
		"is one of:\n");
	for(i = 0; i < NUMTESTS; i++)
		fprintf(stderr, "\t%2d\t%s\n", i, tests[i]->ti_name);
	exit(1);
}

int
main(int argc, char *argv[])
{       
	int			 i;
	struct sigaction	 sa;
	pthread_t		 t;
	struct testInfo 	*ti;

	/* Make sure we can run multi-proc */
	if (pthread__maxconcurrency < 2)
		errx(1, "pthread_maxconcurrency only %d, set "
			"PTHREAD_CONCURRENCY", pthread__maxconcurrency);

	if (argc != 2) {
		usage();
	}

	i = atoi(argv[1]);
	if ((i < 0) || (i >=  NUMTESTS))
		usage();

	ti = tests[i];

	/*
	 * Set up the signal handler
	 */
	sa.sa_sigaction = doIncCounter;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	if (sigaction(SIGUSR1, &sa, NULL))
		err(1, "in sigaction");

	pthread_mutex_lock(&start_mtx);

	/*
	 * Now create the thread. If we are on a multi-proc, our concurrency
	 * will go up.
	 */
	if ((i = pthread_create(&t, NULL, runnerThread, ti)))
		errx(1, "pthread_create returned %d", i);

	pthread_cond_wait(&start_cond, &start_mtx);
	pthread_mutex_unlock(&start_mtx);

	if (pthread__concurrency < 2)
		warnx("We don't have two threads running at once. "
			"Concurrency only %d", pthread__concurrency);

	sleep(1);

	if ((i = (ti->ti_stopper)(t)) != 0)
		errx(1, "pthread_kill returned %d", i);

	if ((i = pthread_join(t, NULL)) != 0)
		errx(1, "pthread_join returned %d", i);

	printf("join returned 0\n");

	return 0;
}
