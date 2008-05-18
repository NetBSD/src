/*	$NetBSD: file2.c,v 1.1.30.1 2008/05/18 12:30:48 yamt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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
 * This test used to trigger deadlock caused by problem fixed
 * in revision 1.79.2.10 if sys/kern/kern_descrip.c
 */

#include <sys/param.h>
#include <sys/event.h>
#include <sys/mount.h>

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
	int fd1, fd2, kq, n;
	struct kevent event[1];

	if (argc != 3)
		errx(1, "Usage: %s file1 file2", argv[0]);
	
	fd1 = open(argv[1], O_RDONLY|O_CREAT, 0644);
	if (fd1 < 0)
		err(1, "open: %s", argv[1]);

	fd2 = open(argv[2], O_RDONLY|O_CREAT, 0644);
	if (fd2 < 0)
		err(1, "open: %s", argv[2]);
	
#if 1		/* XXX: why was this disabled? */
	lseek(fd1, 0, SEEK_END);
#endif

        kq = kqueue();
        if (kq < 0)
                err(1, "kqueue");

	event[0].ident = fd1;
	event[0].filter = EVFILT_READ;
	event[0].flags = EV_ADD | EV_ENABLE;
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n < 0)
		err(1, "kevent(1)");
	
	if (dup2(fd2, fd1) <0)
		err(1, "dup2");

	printf("file2: exiting\n");
	exit(0);
}
