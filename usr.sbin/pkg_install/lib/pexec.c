#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "lib.h"

/*
 * If the supplied callback is not NULL, then call it.
 */
static void call_callback(void (*callback)(void))
{
	if (callback != NULL) {
		callback();
	}
}

/*
 * create pipe, fork and exec file with arguments in argv
 * child takes stdin from pipe, set up fp for parent to
 * output to pipe, and return this information.
 */
pipe_to_system_t *pipe_to_system_begin(const char *file, char *const argv[],
	void (*cleanup_callback)(void))
{
	pipe_to_system_t *retval;

	retval = malloc(sizeof(pipe_to_system_t));
	if (retval == NULL) {
		call_callback(cleanup_callback);
		errx(2, "can't get pipe space");
	}

	retval->cleanup = cleanup_callback;

	if (pipe(retval->fds) == -1) {
		call_callback(retval->cleanup);
		errx(2, "cannot create pipe");
	}

	retval->pid = fork();
	if (retval->pid == -1) {
		call_callback(retval->cleanup);
		errx(2, "cannot fork process for %s", file);
	}

	if (retval->pid == 0) {		/* The child */
		if (retval->fds[0] != 0) {
			dup2(retval->fds[0], 0);
			close(retval->fds[0]);
		}
		close(retval->fds[1]);
		execvp(file, argv);
		warn("failed to execute %s command", file);
		_exit(2);
	}

	/* Meanwhile, back in the parent process ... */
	close(retval->fds[0]);
	retval->fp = fdopen(retval->fds[1], "w");
	if (retval->fp == NULL) {
		call_callback(retval->cleanup);
		errx(2, "fdopen failed");
	}
	return retval;
}

/*
 * close pipe and wait for child to exit.  on non-zero exit status,
 * call cleanup callback.  return exit status.
 */
int pipe_to_system_end(pipe_to_system_t *to_pipe)
{
	int status;
	int wait_ret;

	fclose(to_pipe->fp);
	wait_ret = waitpid(to_pipe->pid, &status, 0);
	if (wait_ret < 0) {
		if (errno != EINTR) {
			call_callback(to_pipe->cleanup);
			errx(2, "waitpid returned failure");
		}
	}
	if (!WIFEXITED(status)) {
		call_callback(to_pipe->cleanup);
		errx(2, "waitpid: process terminated abnormally");
	}
	free(to_pipe);
	return WEXITSTATUS(status);
}
