/*	$NetBSD: ioctl.c,v 1.1 2001/09/20 16:56:53 atatat Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ioctl.c,v 1.1 2001/09/20 16:56:53 atatat Exp $");
#endif

#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>

#if defined(__NetBSD__)
#include <util.h>
#elif defined(__bsdi__)
int openpty(int *, int *, char *, struct termios *, struct winsize *);
#elif defined(__FreeBSD__)
#include <libutil.h>
#else
#error where openpty?
#endif

/* ARGSUSED */
static void
sigchld(int nsig)
{
	if (wait(NULL) == -1)
		err(1, "wait");
}

/* ARGSUSED */
int
main(int argc, char *argv[])
{
	int m, s, rc;
	char name[128], buf[128];
	struct termios term;
	struct sigaction sa;

	/* unbuffer stdout */
	setbuf(stdout, NULL);

	/* get terminal settings for later use */
	if (tcgetattr(STDIN_FILENO, &term) == -1)
		err(1, "tcgetattr");

	/* get a tty */
	if (openpty(&m, &s, name, &term, NULL) == -1)
		err(1, "openpty"); 

	switch (fork()) {
	case -1:
		err(1, "fork");
		/* NOTREACHED */
	case 0:
		/* wait for parent to get set up */
		(void) sleep(1);
		(void) printf("child1: exiting\n");
		exit(0);
		/* NOTREACHED */
	default:
		(void) printf("parent: spawned child1\n");
		break;
	}

	switch (fork()) {
	case -1:
		err(1, "fork");
		/* NOTREACHED */
	case 0:
		/* wait for parent to get upset */
		(void) sleep(2);
		/* drain the tty q */
		if (read(m, buf, sizeof(buf)) == -1)
			err(1, "read");
		(void) printf("child2: exiting\n");
		exit(0);
		/* NOTREACHED */
	default:
		(void) printf("parent: spawned child2\n");
		break;
	}

	/* set up a restarting signal handler */
	(void) sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigchld;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		err(1, "sigaction");
	
	/* put something in the output q */
	if (write(s, "Hello world\n", 12) == -1)
		err(1, "write");

	/* ask for output to drain but don't drain it */
	rc = 0;
	if (tcsetattr(s, TCSADRAIN, &term) == -1) {
		(void) printf("parent: tcsetattr: %s\n", strerror(errno));
		rc = 1;
	}

	/* wait for last child */
        sa.sa_handler = SIG_DFL;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		err(1, "sigaction");
	(void) wait(NULL);

	exit(rc);
	/* NOTREACHED */
	exit(rc);
	return 0;
}
