/*	$NetBSD: rumpuser.c,v 1.13.6.6 2009/01/17 13:29:37 mjf Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code
 * and the Finnish Cultural Foundation.
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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: rumpuser.c,v 1.13.6.6 2009/01/17 13:29:37 mjf Exp $");
#endif /* !lint */

/* thank the maker for this */
#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64
#include <features.h>
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <rump/rumpuser.h>

#include "rumpuser_int.h"

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

int
rumpuser_nanosleep(const struct timespec *rqtp, struct timespec *rmtp, 
		   int *error)
{

	DOCALL_KLOCK(int, (nanosleep(rqtp, rmtp)));
}

void *
rumpuser__malloc(size_t howmuch, int canfail, const char *func, int line)
{
	void *rv;

	rv = malloc(howmuch);
	if (rv == NULL && canfail == 0) {
		warn("malloc failed %s (%d)", func, line);
		abort();
	}

	if (rv)
		memset(rv, 0, howmuch);

	return rv;
}

void *
rumpuser__realloc(void *ptr, size_t howmuch, int canfail,
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

	DOCALL_KLOCK(int, (ioctl(fd, cmd, data)));
}

int
rumpuser_close(int fd, int *error)
{

	DOCALL(int, close(fd));
}

int
rumpuser_fsync(int fd, int *error)
{

	DOCALL_KLOCK(int, fsync(fd));
}

ssize_t
rumpuser_read(int fd, void *data, size_t size, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = read(fd, data, size));
	if (rv == -1)
		*error = errno;

	return rv;
}

ssize_t
rumpuser_pread(int fd, void *data, size_t size, off_t offset, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = pread(fd, data, size, offset));
	if (rv == -1)
		*error = errno;

	return rv;
}

ssize_t 
rumpuser_readv(int fd, const struct iovec *iov, int iovcnt, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = readv(fd, iov, iovcnt));
	if (rv == -1)
		*error = errno;

	return rv;
}

void
rumpuser_read_bio(int fd, void *data, size_t size, off_t offset,
	rump_biodone_fn biodone, void *biodonecookie)
{
	ssize_t rv;
	int error = 0;

	KLOCK_WRAP(rv = rumpuser_pread(fd, data, size, offset, &error));
	/* check against <0 instead of ==-1 to get typing below right */
	if (rv < 0)
		rv = 0;
		
	/* LINTED: see above */
	biodone(biodonecookie, rv, error);
}

ssize_t
rumpuser_write(int fd, const void *data, size_t size, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = write(fd, data, size));
	if (rv == -1)
		*error = errno;

	return rv;
}

ssize_t
rumpuser_pwrite(int fd, const void *data, size_t size, off_t offset, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = pwrite(fd, data, size, offset));
	if (rv == -1)
		*error = errno;

	return rv;
}

ssize_t 
rumpuser_writev(int fd, const struct iovec *iov, int iovcnt, int *error)
{
	ssize_t rv;

	KLOCK_WRAP(rv = writev(fd, iov, iovcnt));
	if (rv == -1)
		*error = errno;

	return rv;
}

void
rumpuser_write_bio(int fd, const void *data, size_t size, off_t offset,
	rump_biodone_fn biodone, void *biodonecookie)
{
	ssize_t rv;
	int error = 0;

	KLOCK_WRAP(rv = rumpuser_pwrite(fd, data, size, offset, &error));
	/* check against <0 instead of ==-1 to get typing below right */
	if (rv < 0)
		rv = 0;

	/* LINTED: see above */
	biodone(biodonecookie, rv, error);
}

int
rumpuser_gettimeofday(struct timeval *tv, int *error)
{

	DOCALL(int, gettimeofday(tv, NULL));
}

int
rumpuser_getenv(const char *name, char *buf, size_t blen, int *error)
{

	DOCALL(int, getenv_r(name, buf, blen));
}

int
rumpuser_gethostname(char *name, size_t namelen, int *error)
{

	DOCALL(int, (gethostname(name, namelen)));
}

char *
rumpuser_realpath(const char *path, char resolvedname[MAXPATHLEN], int *error)
{
	char *rv;

	rv = realpath(path, resolvedname);
	if (rv == NULL)
		*error = errno;
	else
		*error = 0;

	return rv;
}

int
rumpuser_poll(struct pollfd *fds, int nfds, int timeout, int *error)
{

	DOCALL_KLOCK(int, (poll(fds, (nfds_t)nfds, timeout)));
}

int
rumpuser_putchar(int c, int *error)
{

	DOCALL(int, (putchar_unlocked(c)));
}

void
rumpuser_panic()
{

	abort();
}
