/*
 * Copyright (c) 1993,94 Winning Strategies, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Winning Strategies, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$Id: localeconv.c,v 1.4 1994/05/25 01:49:33 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#define _LOCALE_PRIVATE
#include <locale.h>

/* 
 * The localeconv() function constructs a struct lconv from the current
 * monetary and numeric locales.
 *
 * Because localeconv() may be called many times (especially by library
 * routines like printf() & strtod()), the approprate members of the 
 * lconv structure are computed only when the monetary or numeric 
 * locale has been changed.
 */
int __mlocale_changed = 1;
int __nlocale_changed = 1;

/*
 * Return the current locale conversion.
 */
struct lconv *
localeconv()
{
    static struct lconv ret;

    if (__mlocale_changed) {
	/* LC_MONETARY */
	ret.int_curr_symbol	= _CurrentMonetaryLocale->int_curr_symbol;
	ret.currency_symbol	= _CurrentMonetaryLocale->currency_symbol;
	ret.mon_decimal_point	= _CurrentMonetaryLocale->mon_decimal_point;
	ret.mon_thousands_sep	= _CurrentMonetaryLocale->mon_thousands_sep;
	ret.mon_grouping	= _CurrentMonetaryLocale->mon_grouping;
	ret.positive_sign	= _CurrentMonetaryLocale->positive_sign;
	ret.negative_sign	= _CurrentMonetaryLocale->negative_sign;
	ret.int_frac_digits	= _CurrentMonetaryLocale->int_frac_digits;
	ret.frac_digits		= _CurrentMonetaryLocale->frac_digits;
	ret.p_cs_precedes	= _CurrentMonetaryLocale->p_cs_precedes;
	ret.p_sep_by_space	= _CurrentMonetaryLocale->p_sep_by_space;
	ret.n_cs_precedes	= _CurrentMonetaryLocale->n_cs_precedes;
	ret.n_sep_by_space	= _CurrentMonetaryLocale->n_sep_by_space;
	ret.p_sign_posn		= _CurrentMonetaryLocale->p_sign_posn;
	ret.n_sign_posn		= _CurrentMonetaryLocale->n_sign_posn;
	__mlocale_changed = 0;
    }

    if (__nlocale_changed) {
	/* LC_NUMERIC */
	ret.decimal_point	= (char *) _CurrentNumericLocale->decimal_point;
	ret.thousands_sep	= (char *) _CurrentNumericLocale->thousands_sep;
	ret.grouping		= (char *) _CurrentNumericLocale->grouping;
	__nlocale_changed = 0;
    }

    return (&ret);
}
