/*
 * Copyright 2010 Nexenta Systems, Inc.  All rights reserved.
 * Copyright 2015 John Marino <draco@marino.st>
 *
 * This source code is derived from the illumos localedef command, and
 * provided under BSD-style license terms by Nexenta Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * LC_TIME database generation routines for localedef.
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/usr.bin/localedef/time.c 298878 2016-05-01 16:10:56Z pfg $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "localedef.h"
#include "parser.h"
/* #include "timelocal.h" */
#include "citrus_lc_time.h"
#include "mklocaledb.h"

struct lc_time_T {
        const char      *mon[12];
        const char      *month[12];
        const char      *wday[7];
        const char      *weekday[7];
        const char      *X_fmt;
        const char      *x_fmt;
        const char      *c_fmt;
        const char      *am;
        const char      *pm;
        const char      *date_fmt;
        const char      *alt_month[12];
        const char      *md_order;
        const char      *ampm_fmt;
};

struct lc_time_T tm;

void
init_time(void)
{
	(void) memset(&tm, 0, sizeof (tm));
}

void
add_time_str(wchar_t *wcs)
{
	char	*str;

	if ((str = to_mb_string(wcs)) == NULL) {
		INTERR;
		return;
	}
	free(wcs);

	switch (last_kw) {
	case T_D_T_FMT:
		tm.c_fmt = str;
		break;
	case T_D_FMT:
		tm.x_fmt = str;
		break;
	case T_T_FMT:
		tm.X_fmt = str;
		break;
	case T_T_FMT_AMPM:
		tm.ampm_fmt = str;
		break;
	case T_DATE_FMT:
#if 0
		/*
		 * This one is a Solaris extension, Too bad date just
		 * doesn't use %c, which would be simpler.
		 */
		tm.date_fmt = str;
		break;
#endif
	case T_ERA_D_FMT:
	case T_ERA_T_FMT:
	case T_ERA_D_T_FMT:
		/* Silently ignore it. */
		free(str);
		break;
	default:
		free(str);
		INTERR;
		break;
	}
}

static void
add_list(const char *ptr[], char *str, int limit)
{
	int	i;
	for (i = 0; i < limit; i++) {
		if (ptr[i] == NULL) {
			ptr[i] = str;
			return;
		}
	}
	fprintf(stderr,"too many list elements");
}

void
add_time_list(wchar_t *wcs)
{
	char *str;

	if ((str = to_mb_string(wcs)) == NULL) {
		INTERR;
		return;
	}
	free(wcs);

	switch (last_kw) {
	case T_ABMON:
		add_list(tm.mon, str, 12);
		break;
	case T_MON:
		add_list(tm.month, str, 12);
		break;
	case T_ABDAY:
		add_list(tm.wday, str, 7);
		break;
	case T_DAY:
		add_list(tm.weekday, str, 7);
		break;
	case T_AM_PM:
		if (tm.am == NULL) {
			tm.am = str;
		} else if (tm.pm == NULL) {
			tm.pm = str;
		} else {
			fprintf(stderr,"too many list elements");
			free(str);
		}
		break;
	case T_ALT_DIGITS:
	case T_ERA:
		free(str);
		break;
	default:
		free(str);
		INTERR;
		break;
	}
}

void
check_time_list(void)
{
	switch (last_kw) {
	case T_ABMON:
		if (tm.mon[11] != NULL)
			return;
		break;
	case T_MON:
		if (tm.month[11] != NULL)
			return;
		break;
	case T_ABDAY:
		if (tm.wday[6] != NULL)
			return;
		break;
	case T_DAY:
		if (tm.weekday[6] != NULL)
			return;
		break;
	case T_AM_PM:
		if (tm.pm != NULL)
			return;
		break;
	case T_ERA:
	case T_ALT_DIGITS:
		return;
	default:
		fprintf(stderr,"unknown list");
		break;
	}

	fprintf(stderr,"too few items in list (%d)", last_kw);
}

void
reset_time_list(void)
{
	int i;
	switch (last_kw) {
	case T_ABMON:
		for (i = 0; i < 12; i++) {
			free(__UNCONST(tm.mon[i]));
			tm.mon[i] = NULL;
		}
		break;
	case T_MON:
		for (i = 0; i < 12; i++) {
			free(__UNCONST(tm.month[i]));
			tm.month[i] = NULL;
		}
		break;
	case T_ABDAY:
		for (i = 0; i < 7; i++) {
			free(__UNCONST(tm.wday[i]));
			tm.wday[i] = NULL;
		}
		break;
	case T_DAY:
		for (i = 0; i < 7; i++) {
			free(__UNCONST(tm.weekday[i]));
			tm.weekday[i] = NULL;
		}
		break;
	case T_AM_PM:
		free(__UNCONST(tm.am));
		tm.am = NULL;
		free(__UNCONST(tm.pm));
		tm.pm = NULL;
		break;
	}
}

void
dump_time(void)
{
	FILE *f;
	const category_t *cat;

	if ((f = open_category()) == NULL) {
		return;
	}
	if ((cat = mklocaledb_init("TIME")) == NULL) {
		return;
	}

	/*
	 * XXX define the constants as arrays and loop.  This copy-paste job is gross.
	 */
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_1, tm.mon[0]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_2, tm.mon[1]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_3, tm.mon[2]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_4, tm.mon[3]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_5, tm.mon[4]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_6, tm.mon[5]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_7, tm.mon[6]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_8, tm.mon[7]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_9, tm.mon[8]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_10, tm.mon[9]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_11, tm.mon[10]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABMON_12, tm.mon[11]);

	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_1, tm.month[0]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_2, tm.month[1]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_3, tm.month[2]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_4, tm.month[3]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_5, tm.month[4]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_6, tm.month[5]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_7, tm.month[6]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_8, tm.month[7]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_9, tm.month[8]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_10, tm.month[9]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_11, tm.month[10]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_MON_12, tm.month[11]);

	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABDAY_1, tm.wday[0]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABDAY_2, tm.wday[1]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABDAY_3, tm.wday[2]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABDAY_4, tm.wday[3]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABDAY_5, tm.wday[4]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABDAY_6, tm.wday[5]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_ABDAY_7, tm.wday[6]);

	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_DAY_1, tm.weekday[0]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_DAY_2, tm.weekday[1]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_DAY_3, tm.weekday[2]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_DAY_4, tm.weekday[3]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_DAY_5, tm.weekday[4]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_DAY_6, tm.weekday[5]);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_DAY_7, tm.weekday[6]);

	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_D_T_FMT, tm.c_fmt);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_D_FMT, tm.x_fmt);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_T_FMT, tm.X_fmt);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_AM_STR, tm.am);
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_PM_STR, tm.pm);
#if 0
	mklocaledb_record(cat, "date_fmt", tm.date_fmt);
#endif
	mklocaledb_record(cat, _CITRUS_LC_TIME_SYM_T_FMT_AMPM, tm.ampm_fmt);

	mklocaledb_dump(cat, f);
	close_category(f);
}
