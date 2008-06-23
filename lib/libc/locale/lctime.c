/*	$NetBSD: lctime.c,v 1.1.4.2 2008/06/23 05:02:09 wrstuden Exp $	*/
/*
 * Copyright (c) 2008, The NetBSD Foundation, Inc.
 * All rights reserved.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brian Ginsbach.
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: lctime.c,v 1.1.4.2 2008/06/23 05:02:09 wrstuden Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/localedef.h>

#include <assert.h>

#include "localeio.h"
#include "lctime.h"

#define NSTRINGS (sizeof(_TimeLocale)/sizeof(const char **))

int
__loadtime(const char *filename)
{

	_DIAGASSERT(filename != NULL);

	return (__loadlocale(filename, NSTRINGS, 0, sizeof(_TimeLocale),
			     &_CurrentTimeLocale, &_DefaultTimeLocale));
}

#ifdef LOCALE_DEBUG

#include "namespace.h"

#include <stdio.h>

void
__dumptimelocale()
{
	int i;

	for (i = 0; i < 7; i++)
		(void)printf("abday[%d] = \"%s\"\n",
			     i, _CurrentTimeLocale->abday[i]);

	for (i = 0; i < 7; i++)
		(void)printf("day[%d] = \"%s\"\n",
			     i, _CurrentTimeLocale->day[i]);

	for (i = 0; i < 12; i++)
		(void)printf("abmon[%d] = \"%s\"\n",
			     i, _CurrentTimeLocale->abmon[i]);

	for (i = 0; i < 12; i++)
		(void)printf("mon[%d] = \"%s\"\n",
			     i, _CurrentTimeLocale->mon[i]);

	for (i = 0; i < 2; i++)
		(void)printf("am_pm[%d] = \"%s\"\n",
			     i, _CurrentTimeLocale->am_pm[i]);

	(void)printf("d_t_fmt = \"%s\"\n", _CurrentTimeLocale->d_t_fmt);
	(void)printf("d_fmt = \"%s\"\n", _CurrentTimeLocale->d_fmt);
	(void)printf("t_fmt = \"%s\"\n", _CurrentTimeLocale->t_fmt);
	(void)printf("t_fmt_ampm = \"%s\"\n", _CurrentTimeLocale->t_fmt_ampm);
}
#endif	/* LOCALE_DEBUG */
