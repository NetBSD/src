/*	$NetBSD: utils.c,v 1.5.4.1 2017/04/21 16:54:16 bouyer Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: utils.c,v 1.5.4.1 2017/04/21 16:54:16 bouyer Exp $");

#include <sys/types.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

/* XXX Seems to be missing from <stdio.h>...  */
int	snprintf_ss(char *restrict, size_t, const char *restrict, ...)
	    __printflike(3, 4);
int	vsnprintf_ss(char *restrict, size_t, const char *restrict, va_list)
	    __printflike(3, 0);

#include "utils.h"

/*
 * Read, returning partial data only at end of file.
 */
ssize_t
read_block(int fd, void *buf, size_t len)
{
	char *p = buf;
	size_t n = len;
	const char *const end __diagused = p + n;
	ssize_t nread = 0;

	while (0 < n && (nread = read(fd, p, n)) != 0) {
		if (nread == -1)
			return -1;
		p += MIN(n, (size_t)nread);
		n -= MIN(n, (size_t)nread);
		assert(p + n == end);
	}

	assert(n == 0 || nread == 0); /* complete read or EOF */
	return len - n;
}

/*
 * Read from a specified position, returning partial data only at end
 * of file.
 */
ssize_t
pread_block(int fd, void *buf, size_t len, off_t fdpos)
{
	char *p = buf;
	size_t n = len;
	const char *const end __diagused = p + n;
	ssize_t nread = 0;

	assert(0 <= fdpos);
	assert(n <= OFF_MAX - (uintmax_t)fdpos);
	const off_t endpos __diagused = fdpos + n;

	while (0 < n && (nread = pread(fd, p, n, fdpos)) != 0) {
		if (nread == -1)
			return -1;
		fdpos += MIN(n, (size_t)nread);
		p += MIN(n, (size_t)nread);
		n -= MIN(n, (size_t)nread);
		assert(p + n == end);
		assert(fdpos + (off_t)n == endpos);
	}

	assert(n == 0 || nread == 0); /* complete read or EOF */
	return len - n;
}

/*
 * Signal-safe err/warn utilities.  The errno varieties are limited to
 * having no format arguments for reasons of laziness.
 */

void
err_ss(int exit_value, const char *msg)
{
	warn_ss(msg);
	_Exit(exit_value);
}

void
errx_ss(int exit_value, const char *format, ...)
{
	va_list va;

	va_start(va, format);
	vwarnx_ss(format, va);
	va_end(va);
	_Exit(exit_value);
}

void
warn_ss(const char *msg)
{
	int error = errno;

	warnx_ss("%s: %s", msg, strerror(error));

	errno = error;
}

void
warnx_ss(const char *format, ...)
{
	va_list va;

	va_start(va, format);
	vwarnx_ss(format, va);
	va_end(va);
}

void
vwarnx_ss(const char *format, va_list va)
{
	char buf[128];

	(void)strlcpy(buf, getprogname(), sizeof(buf));
	(void)strlcat(buf, ": ", sizeof(buf));

	const int n = vsnprintf_ss(&buf[strlen(buf)], (sizeof(buf) -
		strlen(buf)), format, va);
	if (n <= 0) {
		const char fallback[] =
		    "vndcompress: Help!  I'm trapped in a signal handler!\n";
		(void)write(STDERR_FILENO, fallback, __arraycount(fallback));
	} else {
		(void)strlcat(buf, "\n", sizeof(buf));
		(void)write(STDERR_FILENO, buf, strlen(buf));
	}
}

void
block_signals(sigset_t *old_sigmask)
{
	sigset_t block;

	(void)sigfillset(&block);
	(void)sigprocmask(SIG_BLOCK, &block, old_sigmask);
}

void
restore_sigmask(const sigset_t *sigmask)
{

	(void)sigprocmask(SIG_SETMASK, sigmask, NULL);
}
