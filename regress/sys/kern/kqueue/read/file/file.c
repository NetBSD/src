/*	$NetBSD: file.c,v 1.4 2006/09/29 14:18:25 christos Exp $	*/

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
#include <sys/mount.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

int
main(int argc, char **argv)
{
	int fd, kq, n;
	struct kevent event[1];
	char buffer[128];
	int num = 999;

	if (argc != 2 && argc != 4) {
	    usg:
		errx(1, "Usage: %s file", argv[0]);
	}
	
	if (argc == 4) {
		if (strcmp(argv[1], "-n") == 0)
			num = atoi(argv[2]);
		else
			goto usg;	
		argv += 2;
	}

	fd = open(argv[1], O_RDONLY|O_CREAT, 0644);
	if (fd < 0)
		err(1, "open: %s", argv[1]);

#if 1		/* XXX: why was this disabled? */
	lseek(fd, 0, SEEK_END);
#endif

        kq = kqueue();
        if (kq < 0)
                err(1, "kqueue");

	event[0].ident = fd;
	event[0].filter = EVFILT_READ;
	event[0].flags = EV_ADD | EV_ENABLE;
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n < 0)
		err(1, "kevent(1)");
	
	for (;num > 0;) {
		n = kevent(kq, NULL, 0, event, 1, NULL);
		if (n < 0)
			err(1, "kevent(2)");
		num -= n;
		printf("kevent num %d flags: %#x, fflags: %#x, data: %" PRId64 "\n",
		    n, event[0].flags, event[0].fflags, event[0].data);
		if (event[0].data < 0)
#if 1	/* XXXLUKEM */
			lseek(fd, 0, SEEK_END);
#else
			lseek(fd, event[0].data, SEEK_END);
#endif

		n = read(fd, buffer, 128);
		if (n < 0)
			err(1, "read");
		buffer[n] = '\0';
		printf("file(%d): %s", num, buffer);
	}

	printf("read: successful end\n");
	exit(0);
}
