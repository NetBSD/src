/*	$NetBSD: not.c,v 1.1.1.2 2009/12/02 00:25:58 haad Exp $	*/

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int args, char **argv) {
	pid_t pid;
	int status;
	int FAILURE = 6;

	if (args < 2) {
		fprintf(stderr, "Need args\n");
		return FAILURE;
	}

	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "Could not fork\n");
		return FAILURE;
	} else if (pid == 0) { 	/* child */
		execvp(argv[1], &argv[1]);
		/* should not be accessible */
		return FAILURE;
	} else {		/* parent */
		waitpid(pid, &status, 0);
		if (!WIFEXITED(status)) {
			if (WIFSIGNALED(status))
				fprintf(stderr,
					"Process %d died of signal %d.\n",
					pid, WTERMSIG(status));
			/* did not exit correctly */
			return FAILURE;
		}
		/* return the opposite */
		return !WEXITSTATUS(status);
	}
	/* not accessible */
	return FAILURE;
}
