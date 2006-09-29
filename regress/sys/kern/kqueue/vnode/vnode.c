/*	$NetBSD: vnode.c,v 1.3 2006/09/29 14:18:25 christos Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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

#include <sys/param.h>
#include <sys/event.h>
#include <sys/time.h>

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
	struct timespec	timeout;
	struct timeval	then, now, diff;
	struct kevent	event[2];
	char		buffer[128], *pref;
	int		fd, kq, n, i;

	if (argc != 2)
		errx(1, "Usage: %s file", argv[0]);
	
	fd = open(argv[1], O_RDONLY|O_CREAT, 0644);
	if (fd == -1)
		err(1, "open: %s", argv[1]);

#if 0
	sprintf(buffer, "/mnt/fd/%d", fd);
	if ((fd = open(buffer, O_RDONLY)) < 0)
		err(1, "open fdesc: %s", buffer);
#endif

#ifdef READ_TEST
	lseek(fd, 0, SEEK_END);
#endif

        kq = kqueue();
        if (kq == -1)
                err(1, "kqueue");

	i=0;
	EV_SET(&event[i], fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
		NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB |
		NOTE_LINK | NOTE_RENAME | NOTE_REVOKE, 0, 0);
	i++;

#ifdef READ_TEST
	EV_SET(&event[i], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, NULL, NULL);
	i++;
	printf("i is %d\n", i);
#endif

	n = kevent(kq, event, i, NULL, 0, NULL);
	if (n == -1)
		err(1, "kevent(1)");
	
	timeout.tv_sec = 60;
	timeout.tv_nsec = 0;

	for (;;) {
		pref="";
		if (gettimeofday(&then, NULL) == -1)
			err(1, "gettimeofday then");
		n = kevent(kq, NULL, 0, event, 1, &timeout);
		if (gettimeofday(&now, NULL) == -1)
			err(1, "gettimeofday now");
		timersub(&now, &then, &diff);
		printf("vnode '%s': kevent returned %d in %ld.%06ld\n",
			argv[1],
			n, diff.tv_sec, diff.tv_usec);

		if (n == -1)
			err(1, "kevent");
		else if (n == 0)
			continue;

		printf("vnode '%s': kevent: filter %d flags: 0x%02x, fflags: 0x%02x [",
		    argv[1],
		    event[0].filter, event[0].flags, event[0].fflags);

#define DUMPFLAG(x) \
	do \
		if (event[0].fflags & x) { \
			printf ("%s%s", pref, #x); \
			pref=", "; \
		} \
	while (0)

		DUMPFLAG(NOTE_DELETE);
		DUMPFLAG(NOTE_WRITE);
		DUMPFLAG(NOTE_EXTEND);
		DUMPFLAG(NOTE_ATTRIB);
		DUMPFLAG(NOTE_LINK);
		DUMPFLAG(NOTE_RENAME);
		DUMPFLAG(NOTE_REVOKE);
		printf("], data %" PRId64 "\n", event[0].data);

		if (event[0].filter == EVFILT_READ) {
			if (event[0].data < 0)
				lseek(fd, 0, SEEK_END);
			n = read(fd, buffer, 128);
			if (n < 0)
				err(1, "read");
			buffer[n] = '\0';
			printf("[%d] %s", n, buffer);
		}

		if (event[0].fflags & NOTE_REVOKE) {
			printf("%s revoked\n", argv[1]);
			break;
		}
	}
	return (0);
}
