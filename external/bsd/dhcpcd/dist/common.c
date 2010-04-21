/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2009 Roy Marples <roy@marples.name>
 * All rights reserved

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

/* Needed define to get at getline for glibc and FreeBSD */
#define _GNU_SOURCE

#include <sys/cdefs.h>

#ifdef __APPLE__
#  include <mach/mach_time.h>
#  include <mach/kern_return.h>
#endif

#include <sys/param.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#ifdef BSD
#  include <paths.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

#ifndef _PATH_DEVNULL
#  define _PATH_DEVNULL "/dev/null"
#endif

int clock_monotonic;
static char *lbuf;
static size_t lbuf_len;
#ifdef DEBUG_MEMORY
static char lbuf_set;
#endif

#ifdef DEBUG_MEMORY
static void
free_lbuf(void)
{
	free(lbuf);
	lbuf = NULL;
}
#endif

/* Handy routine to read very long lines in text files.
 * This means we read the whole line and avoid any nasty buffer overflows.
 * We strip leading space and avoid comment lines, making the code that calls
 * us smaller.
 * As we don't use threads, this API is clean too. */
char *
get_line(FILE * __restrict fp)
{
	char *p;
	ssize_t bytes;

#ifdef DEBUG_MEMORY
	if (lbuf_set == 0) {
		atexit(free_lbuf);
		lbuf_set = 1;
	}
#endif

	do {
		bytes = getline(&lbuf, &lbuf_len, fp);
		if (bytes == -1)
			return NULL;
		for (p = lbuf; *p == ' ' || *p == '\t'; p++)
			;
	} while (*p == '\0' || *p == '\n' || *p == '#' || *p == ';');
	if (lbuf[--bytes] == '\n')
		lbuf[bytes] = '\0';
	return p;
}

int
set_cloexec(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFD, 0)) == -1 ||
	    fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
	{
		syslog(LOG_ERR, "fcntl: %m");
		return -1;
	}
	return 0;
}

int
set_nonblock(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1 ||
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		syslog(LOG_ERR, "fcntl: %m");
		return -1;
	}
	return 0;
}

/* Handy function to get the time.
 * We only care about time advancements, not the actual time itself
 * Which is why we use CLOCK_MONOTONIC, but it is not available on all
 * platforms.
 */
#define NO_MONOTONIC "host does not support a monotonic clock - timing can skew"
int
get_monotonic(struct timeval *tp)
{
	static int posix_clock_set = 0;
#if defined(_POSIX_MONOTONIC_CLOCK) && defined(CLOCK_MONOTONIC)
	struct timespec ts;
	static clockid_t posix_clock;

	if (!posix_clock_set) {
		if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
			posix_clock = CLOCK_MONOTONIC;
			clock_monotonic = posix_clock_set = 1;
		}
	}

	if (clock_monotonic) {
		if (clock_gettime(posix_clock, &ts) == 0) {
			tp->tv_sec = ts.tv_sec;
			tp->tv_usec = ts.tv_nsec / 1000;
			return 0;
		}
	}
#elif defined(__APPLE__)
#define NSEC_PER_SEC 1000000000
	/* We can use mach kernel functions here.
	 * This is crap though - why can't they implement clock_gettime?*/
	static struct mach_timebase_info info = { 0, 0 };
	static double factor = 0.0;
	uint64_t nano;
	long rem;

	if (!posix_clock_set) {
		if (mach_timebase_info(&info) == KERN_SUCCESS) {
			factor = (double)info.numer / (double)info.denom;
			clock_monotonic = posix_clock_set = 1;
		}
	}
	if (clock_monotonic) {
		nano = mach_absolute_time();
		if ((info.denom != 1 || info.numer != 1) && factor != 0.0)
			nano *= factor;
		tp->tv_sec = nano / NSEC_PER_SEC;
		rem = nano % NSEC_PER_SEC;
		if (rem < 0) {
			tp->tv_sec--;
			rem += NSEC_PER_SEC;
		}
		tp->tv_usec = rem / 1000;
		return 0;
	}
#endif

	/* Something above failed, so fall back to gettimeofday */
	if (!posix_clock_set) {
		syslog(LOG_WARNING, NO_MONOTONIC);
		posix_clock_set = 1;
	}
	return gettimeofday(tp, NULL);
}

time_t
uptime(void)
{
	struct timeval tv;

	if (get_monotonic(&tv) == -1)
		return -1;
	return tv.tv_sec;
}

int
writepid(int fd, pid_t pid)
{
	char spid[16];
	ssize_t len;

	if (ftruncate(fd, (off_t)0) == -1)
		return -1;
	snprintf(spid, sizeof(spid), "%u\n", pid);
	len = pwrite(fd, spid, strlen(spid), (off_t)0);
	if (len != (ssize_t)strlen(spid))
		return -1;
	return 0;
}

void *
xmalloc(size_t s)
{
	void *value = malloc(s);

	if (value != NULL)
		return value;
	syslog(LOG_ERR, "memory exhausted (xalloc %zu bytes)", s);
	exit (EXIT_FAILURE);
	/* NOTREACHED */
}

void *
xzalloc(size_t s)
{
	void *value = xmalloc(s);

	memset(value, 0, s);
	return value;
}

void *
xrealloc(void *ptr, size_t s)
{
	void *value = realloc(ptr, s);

	if (value != NULL)
		return value;
	syslog(LOG_ERR, "memory exhausted (xrealloc %zu bytes)", s);
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

char *
xstrdup(const char *str)
{
	char *value;

	if (str == NULL)
		return NULL;

	if ((value = strdup(str)) != NULL)
		return value;

	syslog(LOG_ERR, "memory exhausted (xstrdup)");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
