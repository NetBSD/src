/*	$NetBSD: sig.c,v 1.3 2008/12/29 05:56:02 christos Exp $	*/

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

#include <sys/param.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

int
main(int argc, char **argv)
{
	struct timespec		timeout;
	struct timeval		then, now, diff;
	struct kfilter_mapping	km;
	struct kevent		event[1];
	int	kq, n;
	pid_t	pid, child;
	int num = 0, status;

	pid = getpid();
	printf("my pid: %d\n", pid);

	/* fork a child to send signals */
	if ((child = fork()) == 0) {
		int i;
		sleep(2);
		for(i=0; i < 10; i++) {
			kill(pid, SIGUSR1);
			sleep(2);
		}
		_exit(0);
	}

        kq = kqueue();
        if (kq == -1)
                err(1, "kqueue");
	km.name = "EVFILT_SIGNAL";
	if (ioctl(kq, KFILTER_BYNAME, &km) == -1)
		err(1, "KFILTER_BYNAME `%s'", km.name);
	else
		printf("got %d as filter number for `%s'.\n",
		    km.filter, km.name);

	/* ignore the signal to avoid taking it for real */
	signal(SIGUSR1, SIG_IGN);

	event[0].ident = SIGUSR1;
	event[0].filter = km.filter;
	event[0].flags = EV_ADD | EV_ENABLE;
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n == -1)
		err(1, "kevent(1)");

	sleep(1);

	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;

	for (; num < 10;) {
		if (gettimeofday(&then, NULL) == -1)
			err(1, "gettimeofday then");
		n = kevent(kq, NULL, 0, event, 1, &timeout);
		num += n;
		if (gettimeofday(&now, NULL) == -1)
			err(1, "gettimeofday now");
		timersub(&now, &then, &diff);
		printf("sig: kevent returned %d in %lld.%06ld\n", n,
		    (long long)diff.tv_sec, (long)diff.tv_usec);
		if (n == -1)
			err(1, "kevent(2)");
		else if (n == 0)
			continue;
		printf(
		    "sig: kevent flags: 0x%x, data: %" PRId64 " (# times signal posted)\n",
		    event[0].flags, event[0].data);
	}

	(void) waitpid(child, &status, 0);

	printf("sig: finished successfully\n");
	exit(0);
}
