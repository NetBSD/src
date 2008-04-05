/*	$NetBSD: lockf2.c,v 1.1 2008/04/05 03:34:47 yamt Exp $	*/

/*-
 * Copyright (c)2008 YAMAMOTO Takashi,
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

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static int
dolock(int fd, int op, off_t off, off_t size)
{
	off_t result;
	int ret;

	result = lseek(fd, off, SEEK_SET);
	if (result == -1) {
		return errno;
	}
	assert(result == off);
	ret = lockf(fd, op, size);
	if (ret == -1) {
		return errno;
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	int fd = STDOUT_FILENO;
	int error;
	int ret;
	pid_t pid;

	error = dolock(fd, F_LOCK, 0, 1);
	assert(error == 0);

	pid = fork();
	assert(pid != -1);
	if (pid == 0) {
		error = dolock(fd, F_LOCK, 1, 1);
		assert(error == 0);
		dolock(fd, F_LOCK, 0, 1);	/* will block */
		assert(0);
	}
	sleep(1);

	error = dolock(fd, F_LOCK, 1, 1);
	assert(error == EDEADLK);
	ret = kill(pid, SIGKILL);
	assert(ret == 0);

	exit(EXIT_SUCCESS);
}
