/*
 * Create a thread that is running [the sleeper] with a signal blocked
 * and use the other thread to block on select. This is so that we can
 * send the signal on the "select" thread. Interrupt the selected thread
 * with alarm. Detects bug in libpthread where we did not use the proper
 * signal mask, so only the first signal got delivered.
 */
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>

static sig_atomic_t count = 0;

static void
handler(int sig)
{
	count++;
}

static void *
sleeper(void* arg)
{
    int i;
    for (i = 0; i < 100; i++)
	    sleep(1);
    exit(1);
}

int main(int argc, char** argv)
{
	pthread_t id;
	struct sigaction act;

	act.sa_sigaction = NULL;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = handler;

	/* set to handle SIGALRM */
	if (sigaction(SIGALRM, &act, NULL) == -1)
		err(1, "sigaction");

	sigaddset(&act.sa_mask, SIGALRM);
	pthread_sigmask(SIG_SETMASK, &act.sa_mask, NULL);

	pthread_create(&id, NULL, sleeper, NULL);
	sleep(1);

	sigemptyset(&act.sa_mask);
	pthread_sigmask(SIG_SETMASK, &act.sa_mask, NULL);

	for (;;) {
		alarm(1);
		if (select(1, NULL, NULL, NULL, NULL) == -1 && errno == EINTR)
			if (count == 2)
				return 0;
	}
}
