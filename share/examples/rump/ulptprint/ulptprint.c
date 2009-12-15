/*	$NetBSD: ulptprint.c,v 1.1 2009/12/15 16:01:50 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/dirent.h>
#include <sys/mount.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Proof-of-concept program:
 *
 * Prints given (postscript) file by "catting" into the printer.
 */

int
main(int argc, char *argv[])
{
	char buf[8192];
	ssize_t n;
	int probeonly = 0;
	int fd_src, fd_dst;

	if (argc != 2)
		errx(1, "need 2 args");

	if (strcmp(argv[1], "probe") == 0)
		probeonly = 1;

	if (probeonly)
		rump_boot_sethowto(RUMP_AB_VERBOSE);
	rump_init();
	if (probeonly)
		exit(0);

	fd_dst = rump_sys_open("/dev/ulpt0", O_RDWR);
	if (fd_dst == -1)
		err(1, "printer open");

	fd_src = open(argv[1], O_RDONLY);
	if (fd_src == -1)
		err(1, "open source");

	for (;;) {
		n = read(fd_src, buf, sizeof(buf));
		if (n == 0)
			break;
		if (n == -1)
			err(1, "read");

		if (rump_sys_write(fd_dst, buf, n) != n)
			err(1, "write to printer");
	}
	rump_sys_close(fd_dst);

	printf("done\n");
}
