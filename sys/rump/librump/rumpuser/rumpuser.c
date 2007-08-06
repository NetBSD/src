/*	$NetBSD: rumpuser.c,v 1.2 2007/08/06 22:20:57 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rumpuser.h"

#define DOCALL(rvtype, call)						\
do {									\
	rvtype rv;							\
	rv = call;							\
	if (rv == -1)							\
		*error = errno;						\
	return rv;							\
} while (/*CONSTCOND*/0)

int
rumpuser_stat(const char *path, struct stat *sb, int *error)
{

	DOCALL(int, (stat(path, sb)));
}

int
rumpuser_lstat(const char *path, struct stat *sb, int *error)
{

	DOCALL(int, (lstat(path, sb)));
}

void *
_rumpuser_malloc(size_t howmuch, int canfail, const char *func, int line)
{
	void *rv;

	rv = malloc(howmuch);
	if (rv == NULL && canfail == 0) {
		warn("malloc failed %s (%d)", func, line);
		abort();
	}

	return rv;
}

void *
_rumpuser_realloc(void *ptr, size_t howmuch, int canfail,
	const char *func, int line)
{
	void *rv;

	rv = realloc(ptr, howmuch);
	if (rv == NULL && canfail == 0) {
		warn("realloc failed %s (%d)", func, line);
		abort();
	}

	return rv;
}

void
rumpuser_free(void *ptr)
{

	free(ptr);
}

int
rumpuser_open(const char *path, int flags, int *error)
{

	DOCALL(int, (open(path, flags)));
}

int
rumpuser_ioctl(int fd, u_long cmd, void *data, int *error)
{

	DOCALL(int, (ioctl(fd, cmd, data)));
}

void
rumpuser_close(int fd)
{

	close(fd);
}

ssize_t
rumpuser_pread(int fd, void *data, size_t size, off_t offset, int *error)
{

	DOCALL(ssize_t, (pread(fd, data, size, offset)));
}

ssize_t
rumpuser_pwrite(int fd, const void *data, size_t size, off_t offset, int *error)
{

	DOCALL(ssize_t, (pwrite(fd, data, size, offset)));
}

int
rumpuser_gettimeofday(struct timeval *tv)
{

	return gettimeofday(tv, NULL);
}

int
rumpuser_gethostname(char *name, size_t namelen, int *error)
{

	DOCALL(int, (gethostname(name, namelen)));
}

/* urgh */
uint16_t
rumpuser_bswap16(uint16_t value)
{

	return bswap16(value);
}

uint32_t
rumpuser_bswap32(uint32_t value)
{

	return bswap32(value);
}

uint64_t
rumpuser_bswap64(uint64_t value)
{

	return bswap64(value);
}

char *
rumpuser_realpath(const char *path, char resolvedname[MAXPATHLEN])
{

	return realpath(path, resolvedname);
}
