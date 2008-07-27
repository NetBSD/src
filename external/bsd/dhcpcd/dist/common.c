/* 
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
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

#include <sys/param.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#ifdef BSD
#  include <paths.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "logger.h"

#ifndef _PATH_DEVNULL
#  define _PATH_DEVNULL "/dev/null"
#endif

/* Handy routine to read very long lines in text files.
 * This means we read the whole line and avoid any nasty buffer overflows. */
ssize_t
get_line(char **line, size_t *len, FILE *fp)
{
	char *p;
	size_t last = 0;

	while(!feof(fp)) {
		if (*line == NULL || last != 0) {
			*len += BUFSIZ;
			*line = xrealloc(*line, *len);
		}
		p = *line + last;
		memset(p, 0, BUFSIZ);
		fgets(p, BUFSIZ, fp);
		last += strlen(p);
		if (last && (*line)[last - 1] == '\n') {
			(*line)[last - 1] = '\0';
			break;
		}
	}
	return last;
}

/* Simple hack to return a random number without arc4random */
#ifndef HAVE_ARC4RANDOM
uint32_t arc4random(void)
{
	int fd;
	static unsigned long seed = 0;

	if (!seed) {
		fd = open("/dev/urandom", 0);
		if (fd == -1 || read(fd,  &seed, sizeof(seed)) == -1)
			seed = time(0);
		if (fd >= 0)
			close(fd);
		srandom(seed);
	}

	return (uint32_t)random();
}
#endif

/* strlcpy is nice, shame glibc does not define it */
#if HAVE_STRLCPY
#else
size_t
strlcpy(char *dst, const char *src, size_t size)
{
	const char *s = src;
	size_t n = size;

	if (n && --n)
		do {
			if (!(*dst++ = *src++))
				break;
		} while (--n);

	if (!n) {
		if (size)
			*dst = '\0';
		while (*src++);
	}

	return src - s - 1;
}
#endif

#if HAVE_CLOSEFROM
#else
int
closefrom(int fd)
{
	int max = getdtablesize();
	int i;
	int r = 0;

	for (i = fd; i < max; i++)
		r += close(i);
	return r;
}
#endif

/* Close our fd's */
int
close_fds(void)
{
	int fd;

	if ((fd = open(_PATH_DEVNULL, O_RDWR)) == -1)
		return -1;

	dup2(fd, fileno(stdin));
	dup2(fd, fileno(stdout));
	dup2(fd, fileno(stderr));
	if (fd > 2)
		close(fd);
	return 0;
}

int
set_cloexec(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFD, 0)) == -1
	    || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
	{
		logger(LOG_ERR, "fcntl: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int
set_nonblock(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1
	    || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		logger(LOG_ERR, "fcntl: %s", strerror(errno));
		return -1;
	}
	return 0;
}

/* Handy function to get the time.
 * We only care about time advancements, not the actual time itself
 * Which is why we use CLOCK_MONOTONIC, but it is not available on all
 * platforms.
 */
int
clock_monotonic(struct timeval *tp)
{
#if defined(_POSIX_MONOTONIC_CLOCK) && defined(CLOCK_MONOTONIC)
	struct timespec ts;
	static clockid_t posix_clock;
	static int posix_clock_set = 0;

	if (!posix_clock_set) {
		if (sysconf(_SC_MONOTONIC_CLOCK) >= 0)
			posix_clock = CLOCK_MONOTONIC;
		else
			posix_clock = CLOCK_REALTIME;
		posix_clock_set = 1;
	}

	if (clock_gettime(posix_clock, &ts) == -1)
		return -1;

	tp->tv_sec = ts.tv_sec;
	tp->tv_usec = ts.tv_nsec / 1000;
	return 0;
#else
	return gettimeofday(tp, NULL);
#endif
}

time_t
uptime(void)
{
	struct timeval tv;

	if (clock_monotonic(&tv) == -1)
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
	snprintf(spid, sizeof(spid), "%u", pid);
	len = pwrite(fd, spid, strlen(spid), (off_t)0);
	if (len != (ssize_t)strlen(spid))
		return -1;
	return 0;
}

void *
xmalloc(size_t s)
{
	void *value = malloc(s);

	if (value)
		return value;
	logger(LOG_ERR, "memory exhausted");
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

	if (value)
		return (value);
	logger(LOG_ERR, "memory exhausted");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

char *
xstrdup(const char *str)
{
	char *value;

	if (!str)
		return NULL;

	if ((value = strdup(str)))
		return value;

	logger(LOG_ERR, "memory exhausted");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
