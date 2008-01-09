/*	$NetBSD: rumpuser.c,v 1.6.4.2 2008/01/09 01:58:02 matt Exp $	*/

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

#define malloc(a) __real_malloc(a)

/* thank the maker for this */
#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64
#include <features.h>

#include <byteswap.h>
#define bswap16 bswap_16
#define bswap32 bswap_32
#define bswap64 bswap_64
#endif


#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rumpuser.h"

#define DOCALL(rvtype, call)						\
do {									\
	rvtype rv;							\
	rv = call;							\
	if (rv == -1)							\
		*error = errno;						\
	else								\
		*error = 0;						\
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

int
rumpuser_usleep(unsigned long sec, int *error)
{

	DOCALL(int, (usleep(sec)));
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

	if (rv)
		memset(rv, 0, howmuch);

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

int
rumpuser_close(int fd, int *error)
{

	DOCALL(int, close(fd));
}

int
rumpuser_fsync(int fd, int *error)
{

	DOCALL(int, fsync(fd));
}

ssize_t
rumpuser_read(int fd, void *data, size_t size, int *error)
{
	ssize_t rv;

	rv = read(fd, data, size);
	if (rv == -1)
		*error = errno;

	return rv;
}

ssize_t
rumpuser_pread(int fd, void *data, size_t size, off_t offset, int *error)
{
	ssize_t rv;

	rv = pread(fd, data, size, offset);
	if (rv == -1)
		*error = errno;

	return rv;
}

void
rumpuser_read_bio(int fd, void *data, size_t size, off_t offset,
	void *biodonecookie)
{
	ssize_t rv;
	int error = 0;

	rv = rumpuser_pread(fd, data, size, offset, &error);
	/* check against <0 instead of ==-1 to get typing below right */
	if (rv < 0)
		rv = 0;
		
	rump_biodone(biodonecookie, rv, error);
}

ssize_t
rumpuser_write(int fd, const void *data, size_t size, int *error)
{
	ssize_t rv;

	rv = write(fd, data, size);
	if (rv == -1)
		*error = errno;

	return rv;
}

ssize_t
rumpuser_pwrite(int fd, const void *data, size_t size, off_t offset, int *error)
{
	ssize_t rv;

	rv = pwrite(fd, data, size, offset);
	if (rv == -1)
		*error = errno;

	return rv;
}

void
rumpuser_write_bio(int fd, const void *data, size_t size, off_t offset,
	void *biodonecookie)
{
	ssize_t rv;
	int error = 0;

	rv = rumpuser_pwrite(fd, data, size, offset, &error);
	/* check against <0 instead of ==-1 to get typing below right */
	if (rv < 0)
		rv = 0;

	rump_biodone(biodonecookie, rv, error);
}

int
rumpuser_gettimeofday(struct timeval *tv, int *error)
{

	DOCALL(int, gettimeofday(tv, NULL));
}

int
rumpuser_gethostname(char *name, size_t namelen, int *error)
{

	DOCALL(int, (gethostname(name, namelen)));
}

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

#ifdef __linux__
/* eewww */
size_t strlcpy(char *, const char *, size_t);
uint32_t arc4random(void);
size_t
strlcpy(char *dest, const char *src, size_t size)
{

	strncpy(dest, src, size-1);
	dest[size-1] = '\0';

	return strlen(dest);
}

uint32_t
arc4random()
{

	return (uint32_t)random();
}
#endif
