/* $NetBSD: common.h,v 1.2 2014/10/06 18:22:29 roy Exp $ */

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

#ifndef COMMON_H
#define COMMON_H

#include <sys/param.h>
#include <sys/time.h>
#include <stdio.h>

#include "config.h"
#include "defs.h"

#ifndef HOSTNAME_MAX_LEN
#define HOSTNAME_MAX_LEN	250	/* 255 - 3 (FQDN) - 2 (DNS enc) */
#endif

#ifndef MIN
#define MIN(a,b)		((/*CONSTCOND*/(a)<(b))?(a):(b))
#define MAX(a,b)		((/*CONSTCOND*/(a)>(b))?(a):(b))
#endif

#define UNCONST(a)		((void *)(unsigned long)(const void *)(a))
#define STRINGIFY(a)		#a
#define TOSTRING(a)		STRINGIFY(a)

#define USECINSEC		1000000
#define timeval_to_double(tv)						\
	((double)(tv)->tv_sec + (double)((tv)->tv_usec) * 1.0e-6)
#define timernorm(tv) do {						\
	while ((tv)->tv_usec >=  USECINSEC) {				\
		(tv)->tv_sec++;						\
		(tv)->tv_usec -= USECINSEC;				\
	}								\
} while (0 /* CONSTCOND */);
#define tv_to_ms(ms, tv) do {						\
	ms = (tv)->tv_sec * 1000;					\
	ms += (tv)->tv_usec / 1000;					\
} while (0 /* CONSTCOND */);
#define ms_to_tv(tv, ms) do {						      \
	(tv)->tv_sec = ms / 1000;					      \
	(tv)->tv_usec = (suseconds_t)(ms - ((tv)->tv_sec * 1000)) * 1000;     \
} while (0 /* CONSTCOND */);

#ifndef TIMEVAL_TO_TIMESPEC
#define	TIMEVAL_TO_TIMESPEC(tv, ts) do {				\
	(ts)->tv_sec = (tv)->tv_sec;					\
	(ts)->tv_nsec = (tv)->tv_usec * 1000;				\
} while (0 /* CONSTCOND */)
#endif

#if __GNUC__ > 2 || defined(__INTEL_COMPILER)
# ifndef __dead
#  define __dead __attribute__((__noreturn__))
# endif
# ifndef __packed
#  define __packed   __attribute__((__packed__))
# endif
# ifndef __unused
#  define __unused   __attribute__((__unused__))
# endif
#else
# ifndef __dead
#  define __dead
# endif
# ifndef __packed
#  define __packed
# endif
# ifndef __unused
#  define __unused
# endif
#endif

/* We don't really need this as our supported systems define __restrict
 * automatically for us, but it is here for completeness. */
#ifndef __restrict
# if defined(__lint__)
#  define __restrict
# elif __STDC_VERSION__ >= 199901L
#  define __restrict restrict
# elif !(2 < __GNUC__ || (2 == __GNU_C && 95 <= __GNUC_VERSION__))
#  define __restrict
# endif
#endif

void get_line_free(void);
const char *get_hostname(char *, size_t, int);
extern int clock_monotonic;
int get_monotonic(struct timeval *);
ssize_t setvar(char ***, const char *, const char *, const char *);
ssize_t setvard(char ***, const char *, const char *, size_t);
time_t uptime(void);

char *hwaddr_ntoa(const unsigned char *, size_t, char *, size_t);
size_t hwaddr_aton(unsigned char *, const char *);
#endif
