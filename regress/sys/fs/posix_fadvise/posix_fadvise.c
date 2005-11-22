/*	$NetBSD: posix_fadvise.c,v 1.1 2005/11/22 12:18:43 yamt Exp $	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <fcntl.h>

#if POSIX_FADV_NORMAL == -1 || \
    POSIX_FADV_SEQUENTIAL == -1 || \
    POSIX_FADV_RANDOM == -1 || \
    POSIX_FADV_WILLNEED == -1 || \
    POSIX_FADV_DONTNEED == -1 || \
    POSIX_FADV_NOREUSE == -1
#error XXX
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	int fd = STDIN_FILENO;
	int error;
	int pipe_fds[2];
	int badfd = 10;

	close(badfd);
	if (pipe(pipe_fds)) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	errno = 999;

	/*
	 * it's hard to check if posix_fadvise is working properly.
	 * only check return values here.
	 */

#define	TEST(f, adv, expected) \
	error = posix_fadvise(f, 0, 0, adv); \
	if (error != expected) { \
		printf(#f "-" #adv "-" #expected ": error=%d\n", error); \
		exit(EXIT_FAILURE); \
	}

	TEST(fd, -1, EINVAL)
	TEST(pipe_fds[0], POSIX_FADV_NORMAL, ESPIPE)
	TEST(badfd, POSIX_FADV_NORMAL, EBADF)
	TEST(fd, POSIX_FADV_NORMAL, 0)
	TEST(fd, POSIX_FADV_SEQUENTIAL, 0)
	TEST(fd, POSIX_FADV_RANDOM, 0)
	TEST(fd, POSIX_FADV_WILLNEED, 0)
	TEST(fd, POSIX_FADV_DONTNEED, 0)
	TEST(fd, POSIX_FADV_NOREUSE, 0)

	/* posix_fadvise doesn't affect errno. */
	if (errno != 999) {
		printf("errno changed %d\n", errno);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
