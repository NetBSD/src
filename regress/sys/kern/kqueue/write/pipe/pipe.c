/*	$NetBSD: pipe.c,v 1.2 2008/04/28 20:23:06 martin Exp $	*/

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
 * test EVFILT_WRITE for pipe; this used to trigger problem fixed in
 * rev. 1.5.2.9 of sys/kern/sys_pipe.c
 */

#include <sys/param.h>
#include <sys/event.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

int
main(int argc, char **argv)
{
	int kq, n;
	struct kevent event[1];
	char buffer[128];
	int fds[2];
	pid_t child;
	int status;

	if (pipe(fds) < 0)
		err(1, "pipe");

        kq = kqueue();
        if (kq < 0)
                err(1, "kqueue");

	event[0].ident = fds[1];
	event[0].filter = EVFILT_WRITE;
	event[0].flags = EV_ADD | EV_ENABLE;
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n < 0)
		err(1, "kevent(1)");
	
	/* spawn child reader */
	if ((child = fork()) == 0) {
		int sz = read(fds[0], buffer, 128);
		if (sz > 0)
			printf("pipe: child read '%.*s'\n", sz, buffer);
		exit(sz <= 0);
	}

	n = kevent(kq, NULL, 0, event, 1, NULL);
	if (n < 0)
		err(1, "kevent(2)");

	printf("kevent num %d flags: %#x, fflags: %#x, data: %" PRId64 "\n",
		    n, event[0].flags, event[0].fflags, event[0].data);

	n = write(fds[1], "foo", 3);
	close(fds[1]);

	(void) waitpid(child, &status, 0);

	printf("pipe: successful end\n");
	exit(0);
}
