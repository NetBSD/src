/*	$NetBSD: proc1.c,v 1.3 2003/02/08 07:47:14 cgd Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn and Jaromir Dolecek.
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

/*
 * test basic EVFILT_PROC functionality
 * this used to trigger problem fixed in rev. 1.1.1.1.2.13 of
 *   sys/kern/kern_event.c, too
 */

#include <sys/param.h>
#include <sys/event.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

static int doevents(void);

static int
doevents(void)
{
	pid_t ch;
	int status;
	char * const argv[] = { "true", NULL };
	char * const envp[] = { "FOO=BAZ", NULL };

	/* Ensure parent is ready */
	sleep(1);

	/* Do fork */
	switch((ch = fork())) {
	case 0:
		exit(EXIT_SUCCESS);
	default:
		wait(&status);
		break;
	}

	/* Exec */
	execve("/usr/bin/true", argv, envp);
	
	/* NOTREACHED */
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	int kq, n;
	struct kevent event[1];
	pid_t child;
	int want;
	int status;

        kq = kqueue();
        if (kq < 0)
                err(1, "kqueue");

	/* fork a child for doing the events */
	switch((child = fork())) {
	case 0:
		return (doevents());
	case -1:
		err(EXIT_FAILURE, "fork");
		/* NOTREACHED */
	default:
		break;
	}
		
	sleep(1); /* give child some time to come up */

	event[0].ident = child;
	event[0].filter = EVFILT_PROC;
	event[0].flags = EV_ADD | EV_ENABLE;
	event[0].fflags = NOTE_EXIT | NOTE_FORK | NOTE_EXEC; /* | NOTE_TRACK;*/
	want = NOTE_EXIT | NOTE_FORK | NOTE_EXEC;
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n < 0)
		err(1, "kevent(1)");

	/* wait until we get all events we want */
	for (;want;) {
		n = kevent(kq, NULL, 0, event, 1, NULL);
		if (n < 0)
			err(1, "kevent(2)");
		printf("PID %5ld ", (long) event[0].ident);
		if (event[0].fflags & NOTE_EXIT) {
			want &= ~NOTE_EXIT;
			printf(" EXIT");
		}
		if (event[0].fflags & NOTE_EXEC) {
			want &= ~NOTE_EXEC;
			printf(" EXEC");
		}
		if (event[0].fflags & NOTE_FORK) {
			want &= ~NOTE_FORK;
			printf(" FORK");
		}
		if (event[0].fflags & NOTE_CHILD)
			printf(" FORK, parent = %" PRId64, event[0].data);
		printf("\n");
	}

	(void) waitpid(child, &status, 0);

	return ((want == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
