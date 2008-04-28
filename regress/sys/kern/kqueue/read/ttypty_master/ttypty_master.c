/*	$NetBSD: ttypty_master.c,v 1.2 2008/04/28 20:23:06 martin Exp $	*/

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
#include <sys/mount.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>
#include <termios.h>
#include <inttypes.h>

int
main(int argc, char **argv)
{
	int kq, n;
	struct kevent event[1];
	int amaster, aslave;
	char slavetty[1024];
	char buffer[128];
	pid_t child;
	int status;
#if 0
	int fl;
#endif
	struct pollfd pfd;
	struct termios tio;
	
	if (openpty(&amaster, &aslave, slavetty, NULL, NULL) < 0)
		err(1, "openpty");

	printf("tty: openpty master %d slave %d tty '%s'\n",
		amaster, aslave, slavetty);

	if ((child = fork()) == 0) {
		sleep(1);

		/* write something to the 'master' */
		printf("tty: child writing 'f00\\n'\n");
		(void) write(aslave, "f00\n", 4);
	
		_exit(0);
	}
	
	/* switch ONLCR off, to not get confused by newline translation */
	if (tcgetattr(amaster, &tio) == -1) {
		err(1, "tcgetattr");
	}
	tio.c_oflag &= ~ONLCR;
	if (tcsetattr(amaster, TCSADRAIN, &tio) == -1)
		err(1, "tcsetattr");

	pfd.fd = amaster;
	pfd.events = POLLIN;
	printf("tty: polling ...\n");
	if (poll(&pfd, 1, INFTIM) < 0)
		err(1, "poll");
	printf("tty: returned from poll - %d\n", pfd.revents);

#if 0
	fl = 1;
	if (ioctl(amaster, TIOCPKT, &fl) < 0)
		err(1, "ioctl");
#endif

        kq = kqueue();
        if (kq < 0)
                err(1, "kqueue");

	event[0].ident = amaster;
	event[0].filter = EVFILT_READ;
	event[0].flags = EV_ADD | EV_ENABLE;
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n < 0)
		err(1, "kevent(1)");
	
	n = kevent(kq, NULL, 0, event, 1, NULL);
	if (n < 0)
		err(1, "kevent(2)");

	printf("kevent num %d filt %d flags: %#x, fflags: %#x, data: %" PRId64 "\n",
		n, event[0].filter,
		event[0].flags, event[0].fflags, event[0].data);
	if (event[0].filter != EVFILT_READ)
		printf("tty: incorrect filter, expecting EVFILT_READ\n");

	n = read(amaster, buffer, 128);
	printf("tty: read '%.*s' (n=%d)\n", n, buffer, n);

	(void) waitpid(child, &status, 0);

	printf("tty: successful end\n");
	exit(0);
}
