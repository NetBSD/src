/*	$NetBSD: time.h,v 1.31 2003/08/07 09:44:11 agc Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)time.h	8.3 (Berkeley) 1/21/94
 */

#ifndef _TIME_H_
#define	_TIME_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <machine/ansi.h>

#include <sys/null.h>

#ifdef	_BSD_CLOCK_T_
typedef	_BSD_CLOCK_T_	clock_t;
#undef	_BSD_CLOCK_T_
#endif

#ifdef	_BSD_TIME_T_
typedef	_BSD_TIME_T_	time_t;
#undef	_BSD_TIME_T_
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_CLOCKID_T_
typedef	_BSD_CLOCKID_T_	clockid_t;
#undef	_BSD_CLOCKID_T_
#endif

#ifdef	_BSD_TIMER_T_
typedef	_BSD_TIMER_T_	timer_t;
#undef	_BSD_TIMER_T_
#endif

#define CLOCKS_PER_SEC	100

struct tm {
	int	tm_sec;		/* seconds after the minute [0-61] */
	int	tm_min;		/* minutes after the hour [0-59] */
	int	tm_hour;	/* hours since midnight [0-23] */
	int	tm_mday;	/* day of the month [1-31] */
	int	tm_mon;		/* months since January [0-11] */
	int	tm_year;	/* years since 1900 */
	int	tm_wday;	/* days since Sunday [0-6] */
	int	tm_yday;	/* days since January 1 [0-365] */
	int	tm_isdst;	/* Daylight Savings Time flag */
	long	tm_gmtoff;	/* offset from UTC in seconds */
	__aconst char *tm_zone;	/* timezone abbreviation */
};

__BEGIN_DECLS
char *asctime __P((const struct tm *));
clock_t clock __P((void));
char *ctime __P((const time_t *));
double difftime __P((time_t, time_t));
struct tm *gmtime __P((const time_t *));
struct tm *localtime __P((const time_t *));
time_t mktime __P((struct tm *));
size_t strftime __P((char * __restrict, size_t, const char * __restrict,
    const struct tm * __restrict));
time_t time __P((time_t *));

#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || \
    defined(_NETBSD_SOURCE)

#ifdef __LIBC12_SOURCE__
#define CLK_TCK 100
#else

/*
 * CLK_TCK uses libc's internal __sysconf() to retrieve the machine's
 * HZ. The value of _SC_CLK_TCK is 39 -- we hard code it so we do not
 * need to include unistd.h
 */
long __sysconf __P((int));
#define CLK_TCK		(__sysconf(39))

#endif

extern __aconst char *tzname[2];
void tzset __P((void));

/*
 * X/Open Portability Guide >= Issue 4
 */
#if (_XOPEN_SOURCE - 0) >= 4 || defined(_NETBSD_SOURCE)
extern int daylight;
#ifndef __LIBC12_SOURCE__
extern long int timezone __RENAME(__timezone13);
#endif
char *strptime __P((const char * __restrict, const char * __restrict,
    struct tm * __restrict));
#endif

#if (_POSIX_C_SOURCE - 0) >= 199309L || (_XOPEN_SOURCE - 0) >= 500 || \
    defined(_NETBSD_SOURCE)
#include <sys/time.h>		/* XXX for struct timespec */
struct sigevent;
struct itimerspec;
int clock_getres __P((clockid_t, struct timespec *));
int clock_gettime __P((clockid_t, struct timespec *));
int clock_settime __P((clockid_t, const struct timespec *));
int nanosleep __P((const struct timespec *, struct timespec *));
int timer_create __P((clockid_t, struct sigevent *, timer_t *));
int timer_delete __P((timer_t));
int timer_getoverrun __P((timer_t));
int timer_gettime __P((timer_t, struct itimerspec *));
int timer_settime __P((timer_t, int, const struct itimerspec *, 
    struct itimerspec *));
#endif /* _POSIX_C_SOURCE >= 199309 || _XOPEN_SOURCE >= 500 || ... */

#if (_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500 || \
    defined(_REENTRANT) || defined(_NETBSD_SOURCE)
char *asctime_r __P((const struct tm * __restrict, char * __restrict));
char *ctime_r __P((const time_t *, char *));
struct tm *gmtime_r __P((const time_t * __restrict, struct tm * __restrict));
struct tm *localtime_r __P((const time_t * __restrict, struct tm * __restrict));
#endif

#if defined(_NETBSD_SOURCE)
time_t time2posix __P((time_t));
time_t posix2time __P((time_t));
time_t timegm __P((struct tm *const));
time_t timeoff __P((struct tm *const, const long));
time_t timelocal __P((struct tm *const));
#ifdef __LIBC12_SOURCE__
char *timezone __P((int, int));
#endif
void tzsetwall __P((void));
struct tm *offtime __P((const time_t *const, const long));
#endif /* _NETBSD_SOURCE */

#endif /* !_ANSI_SOURCE */
__END_DECLS

#endif /* !_TIME_H_ */
