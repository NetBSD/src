#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/* Test that signal masks are respected while threads are running. */
volatile sig_atomic_t flag;

void handler1(int, siginfo_t *, void *);
void handler2(int, siginfo_t *, void *);
void *threadroutine(void *);

void
handler1(int sig, siginfo_t *info, void *ctx)
{

	kill(getpid(), SIGUSR2);
	/*
	 * If the mask is properly set, SIGUSR2 will not be handled
	 * until this handler returns.
	 */
	flag = 1;
}

void
handler2(int sig, siginfo_t *info, void *ctx)
{
	if (flag == 1)
		flag = 2;
}

void *
threadroutine(void *arg)
{

	kill(getpid(), SIGUSR1);

	if (flag == 2)
		printf("Success: Both handlers ran in order\n");
	else {
		printf("Failure: flag was %d\n", flag);
		exit(1);
	}

	return NULL;
}

int
main(void)
{
	struct sigaction act;
	pthread_t thread;
	int ret;

	act.sa_sigaction = handler1;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGUSR2);
	act.sa_flags = SA_SIGINFO;

	ret = sigaction(SIGUSR1, &act, NULL);
	if (ret) {
		printf("sigaction: %d\n", ret);
		exit(1);
	}

	act.sa_sigaction = handler2;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	ret = sigaction(SIGUSR2, &act, NULL);

	ret = pthread_create(&thread, NULL, threadroutine, NULL);
	if (ret) {
		printf("pthread_create: %d\n", ret);
		exit(1);
	}
	ret = pthread_join(thread, NULL);
	if (ret) {
		printf("pthread_join: %d\n", ret);
		exit(1);
	}
	
	return 0;
}
