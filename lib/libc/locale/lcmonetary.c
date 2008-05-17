/*	$NetBSD: lcmonetary.c,v 1.1 2008/05/17 03:49:54 ginsbach Exp $	*/
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
__RCSID("$NetBSD: lcmonetary.c,v 1.1 2008/05/17 03:49:54 ginsbach Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/localedef.h>

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "localeio.h"
#include "lcmonetary.h"

#define NSTRINGS \
	(offsetof(_MonetaryLocale, int_frac_digits)/sizeof(const char **))
#define NCHARS \
	(offsetof(_MonetaryLocale, int_n_sign_posn) - \
	 offsetof(_MonetaryLocale, int_frac_digits) + 1)

int
__loadmonetary(const char *filename)
{
	const char **gpp;

	_DIAGASSERT(filename != NULL);

	if (!__loadlocale(filename, NSTRINGS, NCHARS, sizeof(_MonetaryLocale),
			  &_CurrentMonetaryLocale, &_DefaultMonetaryLocale)) {
		return 0;
	}

	gpp = __UNCONST(&_CurrentMonetaryLocale->mon_grouping);
	*gpp = __convertgrouping(_CurrentMonetaryLocale->mon_grouping);

	return 1;
}

#ifdef LOCALE_DEBUG

#include "namespace.h"

#include <stdio.h>

void
__dumpmonetarylocale()
{
	const char *gp;

	(void)printf("int_curr_symbol = \"%s\"\n",
		     _CurrentMonetaryLocale->int_curr_symbol);
	(void)printf("currency_symbol = \"%s\"\n",
		     _CurrentMonetaryLocale->currency_symbol);
	(void)printf("mon_decimal_point = \"%s\"\n",
		     _CurrentMonetaryLocale->mon_decimal_point);
	(void)printf("mon_thousands_sep = \"%s\"\n",
		     _CurrentMonetaryLocale->mon_thousands_sep);
	(void)printf("mon_grouping = ");
	for (gp = _CurrentMonetaryLocale->mon_grouping; *gp != '\0'; gp++) {
		if (gp != _CurrentMonetaryLocale->mon_grouping)
			(void)fputc(';', stdout);
		(void)printf("%d", *gp);
	}
	(void)fputc('\n', stdout);
	(void)printf("positive_sign = \"%s\"\n",
		     _CurrentMonetaryLocale->positive_sign);
	(void)printf("negative_sign = \"%s\"\n",
		     _CurrentMonetaryLocale->negative_sign);
	(void)printf("int_frac_digits = %d\n",
		     _CurrentMonetaryLocale->int_frac_digits);
	(void)printf("frac_digits = %d\n",
		     _CurrentMonetaryLocale->frac_digits);
	(void)printf("p_cs_precedes = %d\n",
		     _CurrentMonetaryLocale->p_cs_precedes);
	(void)printf("p_sep_by_space = %d\n",
		     _CurrentMonetaryLocale->p_sep_by_space);
	(void)printf("n_cs_precedes = %d\n",
		     _CurrentMonetaryLocale->n_cs_precedes);
	(void)printf("n_sep_by_space = %d\n",
		     _CurrentMonetaryLocale->n_sep_by_space);
	(void)printf("p_sign_posn = %d\n",
		     _CurrentMonetaryLocale->p_sign_posn);
	(void)printf("n_sign_posn = %d\n",
		     _CurrentMonetaryLocale->n_sign_posn);
	(void)printf("int_p_cs_precedes = %d\n",
		     _CurrentMonetaryLocale->int_p_cs_precedes);
	(void)printf("int_n_cs_precedes = %d\n",
		     _CurrentMonetaryLocale->int_n_cs_precedes);
	(void)printf("int_p_sep_by_space = %d\n",
		     _CurrentMonetaryLocale->int_p_sep_by_space);
	(void)printf("int_n_sep_by_space = %d\n",
		     _CurrentMonetaryLocale->int_n_sep_by_space);
	(void)printf("int_p_sign_posn = %d\n",
		     _CurrentMonetaryLocale->int_p_sign_posn);
	(void)printf("int_n_sign_posn = %d\n",
		     _CurrentMonetaryLocale->int_n_sign_posn);
}
#endif
