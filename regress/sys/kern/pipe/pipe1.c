/*	$NetBSD: pipe1.c,v 1.4 2003/02/10 12:17:20 pk Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <sys/errno.h>
#include <sys/wait.h>

/*
 * Test sanity ws. interrupted & restarted write(2) calls.
 */

pid_t pid;
int nsiginfo = 0;

/*
 * This is used for both parent and child. Handle parent's SIGALRM,
 * the childs SIGINFO doesn't need anything.
 */
void
sighand(int sig)
{
	if (sig == SIGALRM) {
		kill(pid, SIGINFO);
	}
	if (sig == SIGINFO) {
		nsiginfo++;
	}
}

int
main()
{
	int pp[2], st;
	ssize_t sz, todo, done;
	char *f;
	sigset_t sigset, osigset, emptysigset;

	/* Initialise signal masks */
	if (sigemptyset(&emptysigset) != 0)
		err(1, "sigemptyset1");
	if (sigemptyset(&sigset) != 0)
		err(1, "sigemptyset2");
	if (sigaddset(&sigset, SIGINFO) != 0)
		err(1, "sigaddset");

	/* Register signal handlers for both read and writer */
	signal(SIGINFO, sighand);
	signal(SIGALRM, sighand);

	todo = 2 * 1024 * 1024;
	f = (char *) malloc(todo);

	pipe(pp);

	switch((pid = fork())) {
	case 0: /* child */
		close(pp[1]);

		/* Do inital write. This should succeed, make
		 * the other side do partial write and wait for us to pick
		 * rest up.
		 */
		done = read(pp[0], f, 128 * 1024);

		/* Wait until parent is alarmed and awakens us */
		if (sigprocmask(SIG_BLOCK, &sigset, &osigset) != 0)
			err(1, "sigprocmask1");
		while (nsiginfo == 0) {
			if (sigsuspend(&emptysigset) != -1 || errno != EINTR)
				err(1, "sigsuspend");
		}
		if (sigprocmask(SIG_SETMASK, &osigset, NULL) != 0)
			err(1, "sigprocmask2");

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
