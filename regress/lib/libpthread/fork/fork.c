/*	$NetBSD: fork.c,v 1.3 2003/07/26 19:38:48 salo Exp $	*/

/*
 * Check that child process doesn't get threads, also make sure sleep
 * works in child.
 *
 * Written by Love Hörnquist Åstrand <lha@NetBSD.org>, March 2003.
 * Public domain.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

static pid_t parent;
static int thread_survived = 0;

static void *
print_pid(void *arg)
{
	sleep(3);

	thread_survived = 1;
	if (parent != getpid()) {
		kill(parent, SIGKILL);
		errx(1, "child thread running");
	}
	return NULL;
}

int
main(int argc, char **argv)
{
	pthread_t p;
	pid_t fork_pid;
	int ret;
	
	parent = getpid();
	
	pthread_create(&p, NULL, print_pid, NULL);

	fork_pid = fork();
	if (fork_pid == -1)
		err(1, "fork failed");
    
	if (fork_pid) {
		ret = pthread_join(p, NULL);
		if (ret && fork_pid != 0)
			errx(1, "join failed in parent");

		if (!thread_survived)
			errx(1, "child_thread did NOT survive");

		if (fork_pid != 0) {
			int status;
			waitpid(fork_pid, &status, 0);
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
				errx(1, "child died wrongly");
		}
	} else {
		sleep(5);

		if (thread_survived) {
			kill(parent, SIGKILL);
			errx(1, "child_thread survived");
		}
	}

	return 0;
}
