/* $NetBSD: pipe1.c,v 1.2 2001/09/29 13:54:50 jdolecek Exp $ */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>

/*
 * Test sanity ws. interrupted & restarted write(2) calls.
 */

pid_t pid;

/*
 * This is used for both parent and child. Handle parent's SIGALRM,
 * the childs SIGINFO doesn't need anything.
 */
void
sighand(int sig)
{
	if (sig == SIGALRM)
		kill(pid, SIGINFO);
}

int
main()
{
	int pp[2], st;
	ssize_t sz, todo, done;
	char *f;

	todo = 2 * 1024 * 1024;
	f = (char *) malloc(todo);

	pipe(pp);

	switch((pid = fork())) {
	case 0: /* child */
		close(pp[1]);
		signal(SIGINFO, sighand);

		/* Do inital write. This should succeed, make
		 * the other side do partial write and wait for us to pick
		 * rest up.
		 */
		done = read(pp[0], f, 128 * 1024);

		/* Wait until parent is alarmed and awakens us */
		pause();

		/* Read all what parent wants to give us */
		while((sz = read(pp[0], f, 1024 * 1024)) > 0)
			done += sz;

		/*
		 * Exit with 1 if number of bytes read doesn't match
		 * number of expected bytes
		 */
		exit(done != todo);

		break;

	case -1: /* error */
		perror("fork");
		_exit(1);
		/* NOTREACHED */

	default:
		signal(SIGALRM, sighand);
		close(pp[0]);

		/*
		 * Arrange for alarm after two seconds. Since we have
		 * handler setup for SIGARLM, the write(2) call should
		 * be restarted internally by kernel.
		 */
		alarm(2);

		/* We write exactly 'todo' bytes. The very first write(2)
		 * should partially succeed, block and eventually
		 * be restarted by kernel
		 */
		while(todo > 0 && ((sz = write(pp[1], f, todo)) > 0))
			todo -= sz;
		
		/* Close the pipe, so that child would stop reading */
		close(pp[1]);

		/* And pickup child's exit status */
		waitpid(pid, &st, 0);

		exit(WEXITSTATUS(st) != 0);
		/* NOTREACHED */
	}

	return (2);
}
