/*	$NetBSD: time.c,v 1.9 2022/09/23 12:15:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include <isc/assertions.h>
#include <isc/once.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/tm.h>
#include <isc/util.h>

/*
 * struct FILETIME uses "100-nanoseconds intervals".
 * NS / S = 1000000000 (10^9).
 * While it is reasonably obvious that this makes the needed
 * conversion factor 10^7, it is coded this way for additional clarity.
 */
#define NS_PER_S	1000000000
#define NS_INTERVAL	100
#define INTERVALS_PER_S (NS_PER_S / NS_INTERVAL)

/***
 *** Absolute Times
 ***/

static const isc_time_t epoch = { { 0, 0 } };
LIBISC_EXTERNAL_DATA const isc_time_t *const isc_time_epoch = &epoch;

/***
 *** Intervals
 ***/

static const isc_interval_t zero_interval = { 0 };
LIBISC_EXTERNAL_DATA const isc_interval_t *const isc_interval_zero =
	&zero_interval;

void
isc_interval_set(isc_interval_t *i, unsigned int seconds,
		 unsigned int nanoseconds) {
	REQUIRE(i != NULL);
	REQUIRE(nanoseconds < NS_PER_S);

	/*
	 * This rounds nanoseconds up not down.
	 */
	i->interval = (LONGLONG)seconds * INTERVALS_PER_S +
		      (nanoseconds + NS_INTERVAL - 1) / NS_INTERVAL;
}

bool
isc_interval_iszero(const isc_interval_t *i) {
	REQUIRE(i != NULL);
	if (i->interval == 0) {
		return (true);
	}

	return (false);
}

void
isc_time_set(isc_time_t *t, unsigned int seconds, unsigned int nanoseconds) {
	SYSTEMTIME epoch1970 = { 1970, 1, 4, 1, 0, 0, 0, 0 };
	FILETIME temp;
	ULARGE_INTEGER i1;

	REQUIRE(t != NULL);
	REQUIRE(nanoseconds < NS_PER_S);

	SystemTimeToFileTime(&epoch1970, &temp);

	i1.LowPart = temp.dwLowDateTime;
	i1.HighPart = temp.dwHighDateTime;

	/* cppcheck-suppress unreadVariable */
	i1.QuadPart += (unsigned __int64)nanoseconds / 100;
	/* cppcheck-suppress unreadVariable */
	i1.QuadPart += (unsigned __int64)seconds * 10000000;

	t->absolute.dwLowDateTime = i1.LowPart;
	t->absolute.dwHighDateTime = i1.HighPart;
}

void
isc_time_settoepoch(isc_time_t *t) {
	REQUIRE(t != NULL);

	t->absolute.dwLowDateTime = 0;
	t->absolute.dwHighDateTime = 0;
}

bool
isc_time_isepoch(const isc_time_t *t) {
	REQUIRE(t != NULL);

	if (t->absolute.dwLowDateTime == 0 && t->absolute.dwHighDateTime == 0) {
		return (true);
	}

	return (false);
}

isc_result_t
isc_time_now(isc_time_t *t) {
	REQUIRE(t != NULL);

	GetSystemTimeAsFileTime(&t->absolute);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_time_now_hires(isc_time_t *t) {
	REQUIRE(t != NULL);

	GetSystemTimePreciseAsFileTime(&t->absolute);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_time_nowplusinterval(isc_time_t *t, const isc_interval_t *i) {
	ULARGE_INTEGER i1;

	REQUIRE(t != NULL);
	REQUIRE(i != NULL);

	GetSystemTimeAsFileTime(&t->absolute);

	i1.LowPart = t->absolute.dwLowDateTime;
	i1.HighPart = t->absolute.dwHighDateTime;

	if (UINT64_MAX - i1.QuadPart < (unsigned __int64)i->interval) {
		return (ISC_R_RANGE);
	}

	/* cppcheck-suppress unreadVariable */
	i1.QuadPart += i->interval;

	t->absolute.dwLowDateTime = i1.LowPart;
	t->absolute.dwHighDateTime = i1.HighPart;

	return (ISC_R_SUCCESS);
}

int
isc_time_compare(const isc_time_t *t1, const isc_time_t *t2) {
	REQUIRE(t1 != NULL && t2 != NULL);

	return ((int)CompareFileTime(&t1->absolute, &t2->absolute));
}

isc_result_t
isc_time_add(const isc_time_t *t, const isc_interval_t *i, isc_time_t *result) {
	ULARGE_INTEGER i1;

	REQUIRE(t != NULL && i != NULL && result != NULL);

	i1.LowPart = t->absolute.dwLowDateTime;
	i1.HighPart = t->absolute.dwHighDateTime;

	if (UINT64_MAX - i1.QuadPart < (unsigned __int64)i->interval) {
		return (ISC_R_RANGE);
	}

	/* cppcheck-suppress unreadVariable */
	i1.QuadPart += i->interval;

	result->absolute.dwLowDateTime = i1.LowPart;
	result->absolute.dwHighDateTime = i1.HighPart;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_time_subtract(const isc_time_t *t, const isc_interval_t *i,
		  isc_time_t *result) {
	ULARGE_INTEGER i1;

	REQUIRE(t != NULL && i != NULL && result != NULL);

	i1.LowPart = t->absolute.dwLowDateTime;
	i1.HighPart = t->absolute.dwHighDateTime;

	if (i1.QuadPart < (unsigned __int64)i->interval) {
		return (ISC_R_RANGE);
	}

	/* cppcheck-suppress unreadVariable */
	i1.QuadPart -= i->interval;

	result->absolute.dwLowDateTime = i1.LowPart;
	result->absolute.dwHighDateTime = i1.HighPart;

	return (ISC_R_SUCCESS);
}

uint64_t
isc_time_microdiff(const isc_time_t *t1, const isc_time_t *t2) {
	ULARGE_INTEGER i1, i2;
	LONGLONG i3;

	REQUIRE(t1 != NULL && t2 != NULL);

	/* cppcheck-suppress unreadVariable */
	i1.LowPart = t1->absolute.dwLowDateTime;
	/* cppcheck-suppress unreadVariable */
	i1.HighPart = t1->absolute.dwHighDateTime;
	/* cppcheck-suppress unreadVariable */
	i2.LowPart = t2->absolute.dwLowDateTime;
	/* cppcheck-suppress unreadVariable */
	i2.HighPart = t2->absolute.dwHighDateTime;

	if (i1.QuadPart <= i2.QuadPart) {
		return (0);
	}

	/*
	 * Convert to microseconds.
	 */
	i3 = (i1.QuadPart - i2.QuadPart) / 10;

	return (i3);
}

uint32_t
isc_time_seconds(const isc_time_t *t) {
	SYSTEMTIME epoch1970 = { 1970, 1, 4, 1, 0, 0, 0, 0 };
	FILETIME temp;
	ULARGE_INTEGER i1, i2;
	LONGLONG i3;

	SystemTimeToFileTime(&epoch1970, &temp);

	/* cppcheck-suppress unreadVariable */
	i1.LowPart = t->absolute.dwLowDateTime;
	/* cppcheck-suppress unreadVariable */
	i1.HighPart = t->absolute.dwHighDateTime;
	/* cppcheck-suppress unreadVariable */
	i2.LowPart = temp.dwLowDateTime;
	/* cppcheck-suppress unreadVariable */
	i2.HighPart = temp.dwHighDateTime;

	i3 = (i1.QuadPart - i2.QuadPart) / 10000000;

	return ((uint32_t)i3);
}

isc_result_t
isc_time_secondsastimet(const isc_time_t *t, time_t *secondsp) {
	time_t seconds;

	REQUIRE(t != NULL);

	seconds = (time_t)isc_time_seconds(t);

	INSIST(sizeof(unsigned int) == sizeof(uint32_t));
	INSIST(sizeof(time_t) >= sizeof(uint32_t));

	if (isc_time_seconds(t) > (~0U >> 1) && seconds <= (time_t)(~0U >> 1)) {
		return (ISC_R_RANGE);
	}

	*secondsp = seconds;

	return (ISC_R_SUCCESS);
}

uint32_t
isc_time_nanoseconds(const isc_time_t *t) {
	ULARGE_INTEGER i;

	i.LowPart = t->absolute.dwLowDateTime;
	i.HighPart = t->absolute.dwHighDateTime;
	return ((uint32_t)(i.QuadPart % 10000000) * 100);
}

void
isc_time_formattimestamp(const isc_time_t *t, char *buf, unsigned int len) {
	FILETIME localft;
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToLocalFileTime(&t->absolute, &localft) &&
	    FileTimeToSystemTime(&localft, &st))
	{
		GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "dd-MMM-yyyy",
			      DateBuf, 50);
		GetTimeFormat(LOCALE_USER_DEFAULT,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      NULL, TimeBuf, 50);

		snprintf(buf, len, "%s %s.%03u", DateBuf, TimeBuf,
			 st.wMilliseconds);
	} else {
		strlcpy(buf, "99-Bad-9999 99:99:99.999", len);
	}
}

void
isc_time_formathttptimestamp(const isc_time_t *t, char *buf, unsigned int len) {
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	/* strftime() format: "%a, %d %b %Y %H:%M:%S GMT" */

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToSystemTime(&t->absolute, &st)) {
		GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "ddd',' dd MMM yyyy",
			      DateBuf, 50);
		GetTimeFormat(LOCALE_USER_DEFAULT,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      "hh':'mm':'ss", TimeBuf, 50);

		snprintf(buf, len, "%s %s GMT", DateBuf, TimeBuf);
	} else {
		buf[0] = 0;
	}
}

isc_result_t
isc_time_parsehttptimestamp(char *buf, isc_time_t *t) {
	struct tm t_tm;
	time_t when;
	char *p;

	REQUIRE(buf != NULL);
	REQUIRE(t != NULL);

	p = isc_tm_strptime(buf, "%a, %d %b %Y %H:%M:%S", &t_tm);
	if (p == NULL) {
		return (ISC_R_UNEXPECTED);
	}
	when = isc_tm_timegm(&t_tm);
	if (when == -1) {
		return (ISC_R_UNEXPECTED);
	}
	isc_time_set(t, (unsigned int)when, 0);
	return (ISC_R_SUCCESS);
}

void
isc_time_formatISO8601L(const isc_time_t *t, char *buf, unsigned int len) {
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	/* strtime() format: "%Y-%m-%dT%H:%M:%S" */

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToSystemTime(&t->absolute, &st)) {
		GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "yyyy-MM-dd",
			      DateBuf, 50);
		GetTimeFormat(LOCALE_USER_DEFAULT,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      "hh':'mm':'ss", TimeBuf, 50);
		snprintf(buf, len, "%sT%s", DateBuf, TimeBuf);
	} else {
		buf[0] = 0;
	}
}

void
isc_time_formatISO8601Lms(const isc_time_t *t, char *buf, unsigned int len) {
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	/* strtime() format: "%Y-%m-%dT%H:%M:%S.SSS" */

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToSystemTime(&t->absolute, &st)) {
		GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "yyyy-MM-dd",
			      DateBuf, 50);
		GetTimeFormat(LOCALE_USER_DEFAULT,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      "hh':'mm':'ss", TimeBuf, 50);
		snprintf(buf, len, "%sT%s.%03u", DateBuf, TimeBuf,
			 st.wMilliseconds);
	} else {
		buf[0] = 0;
	}
}

void
isc_time_formatISO8601Lus(const isc_time_t *t, char *buf, unsigned int len) {
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	/* strtime() format: "%Y-%m-%dT%H:%M:%S.SSSSSS" */

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToSystemTime(&t->absolute, &st)) {
		ULARGE_INTEGER i;

		GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "yyyy-MM-dd",
			      DateBuf, 50);
		GetTimeFormat(LOCALE_USER_DEFAULT,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      "hh':'mm':'ss", TimeBuf, 50);
		i.LowPart = t->absolute.dwLowDateTime;
		i.HighPart = t->absolute.dwHighDateTime;
		snprintf(buf, len, "%sT%s.%06u", DateBuf, TimeBuf,
			 (uint32_t)(i.QuadPart % 10000000) / 10);
	} else {
		buf[0] = 0;
	}
}

void
isc_time_formatISO8601(const isc_time_t *t, char *buf, unsigned int len) {
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	/* strtime() format: "%Y-%m-%dT%H:%M:%SZ" */

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToSystemTime(&t->absolute, &st)) {
		GetDateFormat(LOCALE_NEUTRAL, 0, &st, "yyyy-MM-dd", DateBuf,
			      50);
		GetTimeFormat(LOCALE_NEUTRAL,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      "hh':'mm':'ss", TimeBuf, 50);
		snprintf(buf, len, "%sT%sZ", DateBuf, TimeBuf);
	} else {
		buf[0] = 0;
	}
}

void
isc_time_formatISO8601ms(const isc_time_t *t, char *buf, unsigned int len) {
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	/* strtime() format: "%Y-%m-%dT%H:%M:%S.SSSZ" */

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToSystemTime(&t->absolute, &st)) {
		GetDateFormat(LOCALE_NEUTRAL, 0, &st, "yyyy-MM-dd", DateBuf,
			      50);
		GetTimeFormat(LOCALE_NEUTRAL,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      "hh':'mm':'ss", TimeBuf, 50);
		snprintf(buf, len, "%sT%s.%03uZ", DateBuf, TimeBuf,
			 st.wMilliseconds);
	} else {
		buf[0] = 0;
	}
}

void
isc_time_formatISO8601us(const isc_time_t *t, char *buf, unsigned int len) {
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	/* strtime() format: "%Y-%m-%dT%H:%M:%S.SSSSSSZ" */

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToSystemTime(&t->absolute, &st)) {
		ULARGE_INTEGER i;

		GetDateFormat(LOCALE_NEUTRAL, 0, &st, "yyyy-MM-dd", DateBuf,
			      50);
		GetTimeFormat(LOCALE_NEUTRAL,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      "hh':'mm':'ss", TimeBuf, 50);
		i.LowPart = t->absolute.dwLowDateTime;
		i.HighPart = t->absolute.dwHighDateTime;
		snprintf(buf, len, "%sT%s.%06uZ", DateBuf, TimeBuf,
			 (uint32_t)(i.QuadPart % 10000000) / 10);
	} else {
		buf[0] = 0;
	}
}

void
isc_time_formatshorttimestamp(const isc_time_t *t, char *buf,
			      unsigned int len) {
	SYSTEMTIME st;
	char DateBuf[50];
	char TimeBuf[50];

	/* strtime() format: "%Y%m%d%H%M%SSSS" */

	REQUIRE(t != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(len > 0);

	if (FileTimeToSystemTime(&t->absolute, &st)) {
		GetDateFormat(LOCALE_NEUTRAL, 0, &st, "yyyyMMdd", DateBuf, 50);
		GetTimeFormat(LOCALE_NEUTRAL,
			      TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st,
			      "hhmmss", TimeBuf, 50);
		snprintf(buf, len, "%s%s%03u", DateBuf, TimeBuf,
			 st.wMilliseconds);
	} else {
		buf[0] = 0;
	}
}

/*
 * POSIX Shims
 */

struct tm *
gmtime_r(const time_t *clock, struct tm *result) {
	errno_t ret = gmtime_s(result, clock);
	if (ret != 0) {
		errno = ret;
		return (NULL);
	}
	return (result);
}

struct tm *
localtime_r(const time_t *clock, struct tm *result) {
	errno_t ret = localtime_s(result, clock);
	if (ret != 0) {
		errno = ret;
		return (NULL);
	}
	return (result);
}

#define BILLION 1000000000

static isc_once_t nsec_ticks_once = ISC_ONCE_INIT;
static double nsec_ticks = 0;

static void
nsec_ticks_init(void) {
	LARGE_INTEGER ticks;
	RUNTIME_CHECK(QueryPerformanceFrequency(&ticks) != 0);
	nsec_ticks = (double)ticks.QuadPart / 1000000000.0;
	RUNTIME_CHECK(nsec_ticks != 0.0);
}

int
nanosleep(const struct timespec *req, struct timespec *rem) {
	int_fast64_t sleep_msec;
	uint_fast64_t ticks, until;
	LARGE_INTEGER before, after;

	RUNTIME_CHECK(isc_once_do(&nsec_ticks_once, nsec_ticks_init) ==
		      ISC_R_SUCCESS);

	if (req->tv_nsec < 0 || BILLION <= req->tv_nsec) {
		errno = EINVAL;
		return (-1);
	}

	/* Sleep() is not interruptible; there is no remaining delay ever  */
	if (rem != NULL) {
		rem->tv_sec = 0;
		rem->tv_nsec = 0;
	}

	if (req->tv_sec >= 0) {
		/*
		 * For requested delays of one second or more, 15ms resolution
		 * granularity is sufficient.
		 */
		Sleep(req->tv_sec * 1000 + req->tv_nsec / 1000000);

		return (0);
	}

	/* Sleep has <-8,8> ms precision, so substract 10 milliseconds */
	sleep_msec = (int64_t)req->tv_nsec / 1000000 - 10;
	ticks = req->tv_nsec * nsec_ticks;

	RUNTIME_CHECK(QueryPerformanceCounter(&before) != 0);

	until = before.QuadPart + ticks;

	if (sleep_msec > 0) {
		Sleep(sleep_msec);
	}

	while (true) {
		LARGE_INTEGER after;
		RUNTIME_CHECK(QueryPerformanceCounter(&after) != 0);
		if (after.QuadPart >= until) {
			break;
		}
	}

done:
	return (0);
}

int
usleep(useconds_t useconds) {
	struct timespec req;

	req.tv_sec = useconds / 1000000;
	req.tv_nsec = (useconds * 1000) % 1000000000;

	nanosleep(&req, NULL);

	return (0);
}
