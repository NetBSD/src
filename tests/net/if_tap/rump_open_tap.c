/* $NetBSD: rump_open_tap.c,v 1.1 2020/09/30 14:43:15 roy Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: rump_open_tap.c,v 1.1 2020/09/30 14:43:15 roy Exp $");

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>
#include <rump/rumpclient.h>

int
main(int argc, char **argv)
{
	int fd;

	if (argc < 2)
		errx(EXIT_FAILURE, "No path specified");

	rumpclient_init();

	fd = rump_sys_open(argv[1], O_RDWR);
	if (fd == -1)
		err(EXIT_FAILURE, "open: %s", argv[1]);

	if (argc > 2) {
		FILE *fp;
		pid_t pid;

		fp = fopen(argv[2], "w");
		if (fp == NULL)
			err(EXIT_FAILURE, "fopen: %s", argv[2]);

		pid = fork();
		switch (pid) {
		case -1:
			err(EXIT_FAILURE, "fork");
		case 0:
			break;
		default:
			fprintf(fp, "%d\n", pid);
			exit(EXIT_SUCCESS);
		}
	}

	/* Spin with the fd open */
	for (;;)
		sleep(100);
}
