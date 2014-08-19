#include <sys/cdefs.h>
 __RCSID("$NetBSD: common.c,v 1.1.1.10.6.3 2014/08/19 23:46:43 tls Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#ifndef __sun
#  include <sys/cdefs.h>
#endif

#ifdef __APPLE__
#  include <mach/mach_time.h>
#  include <mach/kern_return.h>
#endif

#include <sys/param.h>
#include <sys/time.h>

#include <ctype.h>
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

const char *
get_hostname(char *buf, size_t buflen, int short_hostname)
{
	char *p;

	if (gethostname(buf, buflen) != 0)
		return NULL;
	buf[buflen - 1] = '\0';
	if (strcmp(buf, "(none)") == 0 ||
	    strcmp(buf, "localhost") == 0 ||
	    strncmp(buf, "localhost.", strlen("localhost.")) == 0 ||
	    buf[0] == '.')
		return NULL;

	if (short_hostname) {
		p = strchr(buf, '.');
		if (p)
			*p = '\0';
	}

	return buf;
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
#if defined(_POSIX_MONOTONIC_CLOCK) && defined(CLOCK_MONOTONIC)
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		tp->tv_sec = ts.tv_sec;
		tp->tv_usec = (suseconds_t)(ts.tv_nsec / 1000);
		return 0;
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

#if 0
	/* Something above failed, so fall back to gettimeofday */
	if (!posix_clock_set) {
		syslog(LOG_WARNING, NO_MONOTONIC);
		posix_clock_set = 1;
	}
#endif
	return gettimeofday(tp, NULL);
}

ssize_t
setvar(char ***e, const char *prefix, const char *var, const char *value)
{
	size_t len = strlen(var) + strlen(value) + 3;

	if (prefix)
		len += strlen(prefix) + 1;
	**e = malloc(len);
	if (**e == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return -1;
	}
	if (prefix)
		snprintf(**e, len, "%s_%s=%s", prefix, var, value);
	else
		snprintf(**e, len, "%s=%s", var, value);
	(*e)++;
	return (ssize_t)len;
}

ssize_t
setvard(char ***e, const char *prefix, const char *var, size_t value)
{
	char buffer[32];

	snprintf(buffer, sizeof(buffer), "%zu", value);
	return setvar(e, prefix, var, buffer);
}


time_t
uptime(void)
{
	struct timeval tv;

	if (get_monotonic(&tv) == -1)
		return -1;
	return tv.tv_sec;
}

char *
hwaddr_ntoa(const unsigned char *hwaddr, size_t hwlen, char *buf, size_t buflen)
{
	char *p;
	size_t i;

	if (buf == NULL) {
		return NULL;
	}

	if (hwlen * 3 > buflen) {
		errno = ENOBUFS;
		return 0;
	}

	p = buf;
	for (i = 0; i < hwlen; i++) {
		if (i > 0)
			*p ++= ':';
		p += snprintf(p, 3, "%.2x", hwaddr[i]);
	}
	*p ++= '\0';
	return buf;
}

size_t
hwaddr_aton(unsigned char *buffer, const char *addr)
{
	char c[3];
	const char *p = addr;
	unsigned char *bp = buffer;
	size_t len = 0;

	c[2] = '\0';
	while (*p) {
		c[0] = *p++;
		c[1] = *p++;
		/* Ensure that digits are hex */
		if (isxdigit((unsigned char)c[0]) == 0 ||
		    isxdigit((unsigned char)c[1]) == 0)
		{
			errno = EINVAL;
			return 0;
		}
		/* We should have at least two entries 00:01 */
		if (len == 0 && *p == '\0') {
			errno = EINVAL;
			return 0;
		}
		/* Ensure that next data is EOL or a seperator with data */
		if (!(*p == '\0' || (*p == ':' && *(p + 1) != '\0'))) {
			errno = EINVAL;
			return 0;
		}
		if (*p)
			p++;
		if (bp)
			*bp++ = (unsigned char)strtol(c, NULL, 16);
		len++;
	}
	return len;
}
