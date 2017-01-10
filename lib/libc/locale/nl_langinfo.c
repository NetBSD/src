/* $NetBSD: nl_langinfo.c,v 1.18 2017/01/10 17:50:24 christos Exp $ */

/*-
 * Copyright (c)2008 Citrus Project,
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: nl_langinfo.c,v 1.18 2017/01/10 17:50:24 christos Exp $");

#include "namespace.h"
#include <sys/types.h>
#include <sys/localedef.h>
#include <langinfo.h>
#include <stddef.h>
#define __SETLOCALE_SOURCE__
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "runetype_local.h"
#include "setlocale_local.h"

static const char langinfo_category[] = {
    [ D_T_FMT ] = LC_TIME,
    [ D_FMT ] = LC_TIME,
    [ T_FMT ] = LC_TIME,
    [ T_FMT_AMPM ] = LC_TIME,
    [ AM_STR ] = LC_TIME,
    [ PM_STR ] = LC_TIME,
    [ DAY_1 ] = LC_TIME,
    [ DAY_2 ] = LC_TIME,
    [ DAY_3 ] = LC_TIME,
    [ DAY_4 ] = LC_TIME,
    [ DAY_5 ] = LC_TIME,
    [ DAY_6 ] = LC_TIME,
    [ DAY_7 ] = LC_TIME,
    [ ABDAY_1 ] = LC_TIME,
    [ ABDAY_2 ] = LC_TIME,
    [ ABDAY_3 ] = LC_TIME,
    [ ABDAY_4 ] = LC_TIME,
    [ ABDAY_5 ] = LC_TIME,
    [ ABDAY_6 ] = LC_TIME,
    [ ABDAY_7 ] = LC_TIME,
    [ MON_1 ] = LC_TIME,
    [ MON_2 ] = LC_TIME,
    [ MON_3 ] = LC_TIME,
    [ MON_4 ] = LC_TIME,
    [ MON_5 ] = LC_TIME,
    [ MON_6 ] = LC_TIME,
    [ MON_7 ] = LC_TIME,
    [ MON_8 ] = LC_TIME,
    [ MON_9 ] = LC_TIME,
    [ MON_10 ] = LC_TIME,
    [ MON_11 ] = LC_TIME,
    [ MON_12 ] = LC_TIME,
    [ ABMON_1 ] = LC_TIME,
    [ ABMON_2 ] = LC_TIME,
    [ ABMON_3 ] = LC_TIME,
    [ ABMON_4 ] = LC_TIME,
    [ ABMON_5 ] = LC_TIME,
    [ ABMON_6 ] = LC_TIME,
    [ ABMON_7 ] = LC_TIME,
    [ ABMON_8 ] = LC_TIME,
    [ ABMON_9 ] = LC_TIME,
    [ ABMON_10 ] = LC_TIME,
    [ ABMON_11 ] = LC_TIME,
    [ ABMON_12 ] = LC_TIME,
    [ RADIXCHAR ] = LC_NUMERIC,
    [ THOUSEP ] = LC_NUMERIC,
    [ YESSTR ] = LC_MESSAGES,
    [ YESEXPR ] = LC_MESSAGES,
    [ NOSTR ] = LC_MESSAGES,
    [ NOEXPR ] = LC_MESSAGES,
    [ CRNCYSTR ] = 0,
    [ CODESET ] = LC_CTYPE,
    [ ERA ] = 0,
    [ ERA_D_FMT ] = 0,
    [ ERA_D_T_FMT ] = 0,
    [ ERA_T_FMT ] = 0,
    [ ALT_DIGITS ] = 0,
};

#define offsetofu16(a, b) ((uint16_t)offsetof(a, b))

static const uint16_t langinfo_offset[] = {
    [ D_T_FMT ] = offsetofu16(_TimeLocale, d_t_fmt),
    [ D_FMT ] = offsetofu16(_TimeLocale, d_fmt),
    [ T_FMT ] = offsetofu16(_TimeLocale, t_fmt),
    [ T_FMT_AMPM ] = offsetofu16(_TimeLocale, t_fmt_ampm),
    [ AM_STR ] = offsetofu16(_TimeLocale, am_pm[0]),
    [ PM_STR ] = offsetofu16(_TimeLocale, am_pm[1]),
    [ DAY_1 ] = offsetofu16(_TimeLocale, day[0]),
    [ DAY_2 ] = offsetofu16(_TimeLocale, day[1]),
    [ DAY_3 ] = offsetofu16(_TimeLocale, day[2]),
    [ DAY_4 ] = offsetofu16(_TimeLocale, day[3]),
    [ DAY_5 ] = offsetofu16(_TimeLocale, day[4]),
    [ DAY_6 ] = offsetofu16(_TimeLocale, day[5]),
    [ DAY_7 ] = offsetofu16(_TimeLocale, day[6]),
    [ ABDAY_1 ] = offsetofu16(_TimeLocale, abday[0]),
    [ ABDAY_2 ] = offsetofu16(_TimeLocale, abday[1]),
    [ ABDAY_3 ] = offsetofu16(_TimeLocale, abday[2]),
    [ ABDAY_4 ] = offsetofu16(_TimeLocale, abday[3]),
    [ ABDAY_5 ] = offsetofu16(_TimeLocale, abday[4]),
    [ ABDAY_6 ] = offsetofu16(_TimeLocale, abday[5]),
    [ ABDAY_7 ] = offsetofu16(_TimeLocale, abday[6]),
    [ MON_1 ] = offsetofu16(_TimeLocale, mon[0]),
    [ MON_2 ] = offsetofu16(_TimeLocale, mon[1]),
    [ MON_3 ] = offsetofu16(_TimeLocale, mon[2]),
    [ MON_4 ] = offsetofu16(_TimeLocale, mon[3]),
    [ MON_5 ] = offsetofu16(_TimeLocale, mon[4]),
    [ MON_6 ] = offsetofu16(_TimeLocale, mon[5]),
    [ MON_7 ] = offsetofu16(_TimeLocale, mon[6]),
    [ MON_8 ] = offsetofu16(_TimeLocale, mon[7]),
    [ MON_9 ] = offsetofu16(_TimeLocale, mon[8]),
    [ MON_10 ] = offsetofu16(_TimeLocale, mon[9]),
    [ MON_11 ] = offsetofu16(_TimeLocale, mon[10]),
    [ MON_12 ] = offsetofu16(_TimeLocale, mon[11]),
    [ ABMON_1 ] = offsetofu16(_TimeLocale, abmon[0]),
    [ ABMON_2 ] = offsetofu16(_TimeLocale, abmon[1]),
    [ ABMON_3 ] = offsetofu16(_TimeLocale, abmon[2]),
    [ ABMON_4 ] = offsetofu16(_TimeLocale, abmon[3]),
    [ ABMON_5 ] = offsetofu16(_TimeLocale, abmon[4]),
    [ ABMON_6 ] = offsetofu16(_TimeLocale, abmon[5]),
    [ ABMON_7 ] = offsetofu16(_TimeLocale, abmon[6]),
    [ ABMON_8 ] = offsetofu16(_TimeLocale, abmon[7]),
    [ ABMON_9 ] = offsetofu16(_TimeLocale, abmon[8]),
    [ ABMON_10 ] = offsetofu16(_TimeLocale, abmon[9]),
    [ ABMON_11 ] = offsetofu16(_TimeLocale, abmon[10]),
    [ ABMON_12 ] = offsetofu16(_TimeLocale, abmon[11]),
    [ RADIXCHAR ] = offsetofu16(_NumericLocale, decimal_point),
    [ THOUSEP ] = offsetofu16(_NumericLocale, thousands_sep),
    [ YESSTR ] = offsetofu16(_MessagesLocale, yesstr),
    [ YESEXPR ] = offsetofu16(_MessagesLocale, yesexpr),
    [ NOSTR ] = offsetofu16(_MessagesLocale, nostr),
    [ NOEXPR ] = offsetofu16(_MessagesLocale, noexpr),
    [ CRNCYSTR ] = 0,
    [ CODESET ] = offsetofu16(_RuneLocale, rl_codeset),
    [ ERA ] = 0,
    [ ERA_D_FMT ] = 0,
    [ ERA_D_T_FMT ] = 0,
    [ ERA_T_FMT ] = 0,
    [ ALT_DIGITS ] = 0,
};

__weak_alias(nl_langinfo_l, _nl_langinfo_l)

char *
nl_langinfo(nl_item item)
{

	return nl_langinfo_l(item, _current_locale());
}

char *
nl_langinfo_l(nl_item item, locale_t loc)
{
	char *s;
	int category;
	size_t offset;

	if (item < 0 || item >= (long)__arraycount(langinfo_category))
		return __UNCONST(""); /* Outside the defined range */

	category = langinfo_category[item];
	if (category == 0)
		return __UNCONST(""); /* Not in use */
	offset = langinfo_offset[item];

	memcpy(&s, (char *)loc->part_impl[category] + offset, sizeof(s));
	if (s == NULL)
		return __UNCONST("");
	else
		return s;
}
