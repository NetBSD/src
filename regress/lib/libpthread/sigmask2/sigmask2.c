#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/* Test that signal masks are respected before threads are started */
volatile sig_atomic_t flag;

void handler1(int sig, siginfo_t *info, void *ctx);
void handler2(int sig, siginfo_t *info, void *ctx);

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

int
main(void)
{
	struct sigaction act;
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

	kill(getpid(), SIGUSR1);

	if (flag == 2)
		printf("Success: Both handlers ran in order\n");
	else {
		printf("Failure: flag was %d\n", flag);
		exit(1);
	}

	return 0;
}
