/*	$NetBSD: clonetest.c,v 1.2 2001/07/18 19:24:02 thorpej Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <err.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int	newclone(void *);

int	main(int, char *[]);

#define	STACKSIZE	(8 * 1024)

#define	FROBVAL		41973
#define	CHILDEXIT	0xa5

int
main(int argc, char *argv[])
{
	sigset_t mask;
	void *stack;
	__volatile int frobme = 0;
	pid_t pid;
	int stat;

	stack = mmap(NULL, STACKSIZE, PROT_READ|PROT_WRITE|PROT_EXEC,
	    MAP_PRIVATE|MAP_ANON, -1, (off_t) 0);
	if (stack == MAP_FAILED)
		err(1, "mmap stack");

	/*
	 * XXX FIX ME FOR UPWARD-GROWING STACK
	 */
	stack = (caddr_t) stack + STACKSIZE;

	printf("parent: stack = %p\n", stack);
	fflush(stdout);

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		err(1, "sigprocmask (SIGUSR1)");

	pid = __clone(newclone, stack,
	    CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|SIGUSR1,
	    (void *) &frobme);
	if (pid == -1)
		err(1, "clone");

	while (waitpid(pid, &stat, __WCLONE) != pid)
		/* spin */ ;

	if (WIFEXITED(stat) == 0)
		errx(1, "child didn't exit\n");

	printf("parent: childexit = 0x%x frobme = %d\n",
	    WEXITSTATUS(stat), frobme);

	if (WEXITSTATUS(stat) != CHILDEXIT || frobme != FROBVAL)
		exit(1);

	exit(0);
}

int
newclone(void *arg)
{
	int *frobp = arg;

	printf("child: stack ~= %p\n", &frobp);
	fflush(stdout);

	*frobp = FROBVAL;

	return (CHILDEXIT);
}
