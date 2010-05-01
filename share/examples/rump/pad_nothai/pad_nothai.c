/*	$NetBSD: pad_nothai.c,v 1.1 2010/05/01 23:31:01 pooka Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
#include <sys/dkio.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "musa.c"

int
main(int argc, char *argv[])
{
	char buf[8192];
	int padfd, audiofd;
	ssize_t n;

	if (argc != 2)
		rump_boot_sethowto(RUMP_AB_VERBOSE);

	rump_init();

	if (argc != 2) {
		printf("\nThis program dumps audio on stdout.  redirecting\n");
		printf("it is highly recommended.  Run the program\n");
		printf("with any argument (argc == 2) to trigger output.\n");
		printf("You can test by playing the output with e.g.:\n");
		printf("    audioplay -f -c 2 -P 16 -s 44100 -e slinear_le\n");
		exit(1);
	}

	audiofd = rump_sys_open("/dev/audio0", O_RDWR);
	if (audiofd == -1)
		err(1, "open audio");

	if ((n = rump_sys_write(audiofd, musa, sizeof(musa))) != sizeof(musa))
		err(1, "write");

	padfd = rump_sys_open("/dev/pad0", O_RDONLY);
	if (padfd == -1)
		err(1, "open pad");

	while ((n = rump_sys_read(padfd, buf, sizeof(buf))) > 0) {
		write(STDOUT_FILENO, buf, n);
	}
}
