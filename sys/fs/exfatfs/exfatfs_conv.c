/*	$NetBSD: exfatfs_conv.c,v 1.1.2.3 2024/07/31 03:43:16 perseant Exp $	*/

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
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
/*-
 * Copyright (C) 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exfatfs_conv.c,v 1.1.2.3 2024/07/31 03:43:16 perseant Exp $");

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#ifndef _KERNEL
#include <assert.h>
#define KASSERT(x)     assert(x)
#endif

/*
 * System include files.
 */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/endian.h>
#ifdef _KERNEL
#include <sys/dirent.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <dev/clock_subr.h>
#else
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/queue.h>
#endif

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_conv.h>

static int ucs2utf8(const u_int16_t *, u_int8_t *, int);
static int utf8ucs2(const u_int8_t *, int, u_int16_t *);

/*
 * The number of seconds between Jan 1, 1970 and Jan 1, 1980. In that
 * interval there were 8 regular years and 2 leap years.
 * There were also nine leap seconds, but not in UTC.
 */
#define	DOSBIASYEAR	1980
#define	SECONDSTO1980	(((8 * 365) + (2 * 366)) * (24 * 60 * 60) /* + 9 */)
/*
 * DOS date format cannot store dates beyond the year 2234
 */
#define DOSMAXYEAR	((DT_YEAR_MASK >> DT_YEAR_SHIFT) + DOSBIASYEAR)

/*
 * This is the format of the contents of the deTime field in the direntry
 * structure.
 * We don't use bitfields because we don't know how compilers for
 * arbitrary machines will lay them out.
 */
#define DT_2SECONDS_MASK        0x1F    /* seconds divided by 2 */
#define DT_2SECONDS_SHIFT       0
#define DT_MINUTES_MASK         0x7E0   /* minutes */
#define DT_MINUTES_SHIFT        5
#define DT_HOURS_MASK           0xF800  /* hours */
#define DT_HOURS_SHIFT          11

/*
 * This is the format of the contents of the deDate field in the direntry
 * structure.
 */
#define DT_DAY_MASK             0x1F0000    /* day of month */
#define DT_DAY_SHIFT            16
#define DT_MONTH_MASK           0x1E00000   /* month */
#define DT_MONTH_SHIFT          21
#define DT_YEAR_MASK            0xFE000000  /* year - 1980 */
#define DT_YEAR_SHIFT           25


#ifdef _KERNEL
/*
 * Convert the unix version of time to dos's idea of time to be used in
 * file timestamps. The passed in unix time is assumed to be in GMT.
 */
void
exfatfs_unix2dostime(const struct timespec *tsp, int gmtoff, uint32_t *dtp,
	uint8_t *dhp)
{
	u_long t;
	struct clock_ymdhms ymd;

	t = tsp->tv_sec + gmtoff; /* time zone correction */

	/*
	 * DOS timestamps can not represent dates before 1980.
	 */
	if (t < SECONDSTO1980)
		goto invalid_dos_date;

	/*
	 * DOS granularity is 2 seconds
	 */
	t &= ~1;

	/*
	 * Convert to year/month/day/.. format
	 */
	clock_secs_to_ymdhms(t, &ymd);
	if (ymd.dt_year > DOSMAXYEAR)
		goto invalid_dos_date;

	/*
	 * Now transform to DOS format
	 */
	*dtp = ((ymd.dt_year - DOSBIASYEAR) << DT_YEAR_SHIFT)
		+ (ymd.dt_mon               << DT_MONTH_SHIFT)
		+ (ymd.dt_day               << DT_DAY_SHIFT)
		+ (ymd.dt_hour              << DT_HOURS_SHIFT)
		+ (ymd.dt_min               << DT_MINUTES_SHIFT)
		+ ((ymd.dt_sec / 2)         << DT_2SECONDS_SHIFT);

	if (dhp)
		*dhp = (tsp->tv_sec & 1) * 100 + tsp->tv_nsec / 10000000;
	if (dtp)
	return;

invalid_dos_date:
	*dtp = 0;
	if (dhp)
		*dhp = 0;
}

/*
 * Convert from dos' idea of time to unix'. This will probably only be
 * called from the stat(), and fstat() system calls and so probably need
 * not be too efficient.
 */
struct timespec *
exfatfs_dos2unixtime(uint32_t dt, u_int8_t dh, int gmtoff,
	struct timespec *tsp)
{
	time_t seconds;
	struct clock_ymdhms ymd;

	if (dt == 0) {
		/*
		 * Uninitialized field, return the epoch.
		 */
		tsp->tv_sec = 0;
		tsp->tv_nsec = 0;
		return NULL;
	}

	memset(&ymd, 0, sizeof(ymd));
	ymd.dt_year = ((dt & DT_YEAR_MASK)     >> DT_YEAR_SHIFT) + DOSBIASYEAR;
	ymd.dt_mon =  (dt & DT_MONTH_MASK)     >> DT_MONTH_SHIFT;
	ymd.dt_day =  (dt & DT_DAY_MASK)       >> DT_DAY_SHIFT;
	ymd.dt_hour = (dt & DT_HOURS_MASK)     >> DT_HOURS_SHIFT;
	ymd.dt_min =  (dt & DT_MINUTES_MASK)   >> DT_MINUTES_SHIFT;
	ymd.dt_sec =  ((dt & DT_2SECONDS_MASK) >> DT_2SECONDS_SHIFT) * 2;

	seconds = clock_ymdhms_to_secs(&ymd);

	tsp->tv_sec = seconds + (dh / 100);
	tsp->tv_sec -= gmtoff;	/* time zone correction */
	tsp->tv_nsec = (dh % 100) * 10000000;

	return tsp;
}
#else /* ! _KERNEL */
/*
 * User-space versions of the above.
 */
void
exfatfs_unix2dostime(const struct timespec *tsp, int gmtoff, uint32_t *dtp,
	uint8_t *dhp)
{
	time_t t;
	struct tm *tm;

	t = tsp->tv_sec + gmtoff; /* time zone correction */

	/*
	 * DOS timestamps can not represent dates before 1980.
	 */
	if (t < SECONDSTO1980)
		goto invalid_dos_date;

	/*
	 * DOS granularity is 2 seconds
	 */
	t &= ~1;

	/*
	 * Convert to year/month/day/.. format
	 */
	tm = gmtime(&t);
	if (tm->tm_year > (int)DOSMAXYEAR)
		goto invalid_dos_date;

	/*
	 * Now transform to DOS format
	 */
	*dtp = ((tm->tm_year + 1900 - DOSBIASYEAR) << DT_YEAR_SHIFT)
		+ ((tm->tm_mon + 1)         << DT_MONTH_SHIFT)
		+ (tm->tm_mday              << DT_DAY_SHIFT)
		+ (tm->tm_hour              << DT_HOURS_SHIFT)
		+ (tm->tm_min               << DT_MINUTES_SHIFT)
		+ ((tm->tm_sec / 2)         << DT_2SECONDS_SHIFT);

	if (dhp)
		*dhp = (tsp->tv_sec & 1) * 100 + tsp->tv_nsec / 10000000;
	if (dtp)
	return;

invalid_dos_date:
	*dtp = 0;
	if (dhp)
		*dhp = 0;
}

struct timespec *
exfatfs_dos2unixtime(uint32_t dt, uint8_t dh, int gmtoff, struct timespec *tsp)
{
	struct tm tm;
	time_t seconds;

	memset(&tm, 0, sizeof(tm));
	tm.tm_year = ((dt & DT_YEAR_MASK)       >> DT_YEAR_SHIFT)
							+ DOSBIASYEAR - 1900;
	tm.tm_mon =  ((dt & DT_MONTH_MASK) - 1) >> DT_MONTH_SHIFT;
	tm.tm_mday = (dt & DT_DAY_MASK)         >> DT_DAY_SHIFT;
	tm.tm_hour = (dt & DT_HOURS_MASK)       >> DT_HOURS_SHIFT;
	tm.tm_min =  (dt & DT_MINUTES_MASK)     >> DT_MINUTES_SHIFT;
	tm.tm_sec =  ((dt & DT_2SECONDS_MASK)   >> DT_2SECONDS_SHIFT) * 2;
	tm.tm_isdst = -1;
	tm.tm_gmtoff = 0;

	seconds = mktime_z(NULL, &tm);
	tsp->tv_sec = seconds;
	tsp->tv_sec -= gmtoff;	/* time zone correction */
	tsp->tv_nsec = (dh % 100) * 10000000;

	return tsp;
}
#endif /* ! _KERNEL */

/*
 * Convert UCS-2 character into UTF-8
 * return number of output bytes or 0 if output
 * buffer is too short.
 * We do slash conversion here as well.
 */
static int
ucs2utf8(const u_int16_t *in, u_int8_t *out, int n)
{
	uint16_t inch = le16toh(in[0]);

	if (inch <= 0x007f) {
		if (n < 1) return 0;
		if (inch == '/')
			inch = '\\';
		if (out)
			*out++ = inch;
		return 1;
	} else if (inch <= 0x07ff) {
		if (n < 2) return 0;
		if (out) {
			*out++ = 0xc0 | (inch >> 6);
			*out++ = 0x80 | (inch & 0x3f);
		}
		return 2;
	} else {
		if (n < 3) return 0;
		if (out) {
			*out++ = 0xe0 | (inch >> 12);
			*out++ = 0x80 | ((inch >> 6) & 0x3f);
			*out++ = 0x80 | (inch & 0x3f);
		}
		return 3;
	}
}


/*
 * Convert UTF-8 bytes into UCS-2 character
 * return number of input bytes, 0 if input
 * is too short and -1 if input is invalid.
 * We do slash conversion here as well.
 */
static int
utf8ucs2(const u_int8_t *in, int n, u_int16_t *out)
{
	uint16_t outch;

	if (n < 1) return 0;

	if (in[0] <= 0x7f) {
		outch = in[0];
		if (outch == '\\')
			outch = '/';
		if (out)
			*out = htole16(outch);
		return 1;
	} else if (in[0] <= 0xdf) {
		if (n < 2) return 0;
		outch = (in[0] & 0x1f) << 6 | (in[1] & 0x3f);
		if (out)
			*out = htole16(outch);
		return 2;
	} else if (in[0] <= 0xef) {
		if (n < 3) return 0;
		outch = (in[0] & 0x1f) << 12
			| (in[1] & 0x3f) << 6
			| (in[2] & 0x3f);
		if (out)
			*out = htole16(outch);
		return 3;
	}

	return -1;
}

/*
 * Convert UCS-2 string into UTF-8 string
 * return total number of output bytes
 */
int
exfatfs_ucs2utf8str(const u_int16_t *in, int n, u_int8_t *out, int m)
{
	u_int8_t *p;
	int outlen;

	p = out;
	while (n > 0 && *in != 0) {
		outlen = ucs2utf8(in, out ? p : out, m);
		if (outlen == 0)
			break;
		p += outlen;
		m -= outlen;
		in += 1;
		n -= 1;
	}

	return p - out;
}

/*
 * Convert UTF8 string into UCS-2 string
 * return total number of output characters
 */
int
exfatfs_utf8ucs2str(const u_int8_t *in, int n, u_int16_t *out, int m)
{
	u_int16_t *p;
	int inlen;

	p = out;
	while (n > 0 && *in != 0) {
		if (m < 1)
			break;
		inlen = utf8ucs2(in, n, out ? p : out);
		if (inlen <= 0)
			break;
		in += inlen;
		n -= inlen;
		p += 1;
		m -= 1;
	}

	return p - out;
}
