/*
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)locale.h	5.2 (Berkeley) 2/24/91
 *	$Id: locale.h,v 1.4 1994/05/25 01:14:14 jtc Exp $
 */

#ifndef _LOCALE_H_
#define _LOCALE_H_

struct lconv {
	char	*decimal_point;
	char	*thousands_sep;
	char	*grouping;
	char	*int_curr_symbol;
	char	*currency_symbol;
	char	*mon_decimal_point;
	char	*mon_thousands_sep;
	char	*mon_grouping;
	char	*positive_sign;
	char	*negative_sign;
	char	int_frac_digits;
	char	frac_digits;
	char	p_cs_precedes;
	char	p_sep_by_space;
	char	n_cs_precedes;
	char	n_sep_by_space;
	char	p_sign_posn;
	char	n_sign_posn;
};

#ifndef NULL
#define	NULL	0
#endif

#define	LC_ALL		0
#define	LC_COLLATE	1
#define	LC_CTYPE	2
#define	LC_MONETARY	3
#define	LC_NUMERIC	4
#define	LC_TIME		5
#define LC_MESSAGES	6

#define	_LC_LAST	7		/* marks end */

#include <sys/cdefs.h>

__BEGIN_DECLS
struct lconv	*localeconv __P((void));
char		*setlocale __P((int, const char *));
__END_DECLS


#ifdef _LOCALE_PRIVATE

#include <sys/param.h>
#include <sys/types.h>

#define _LOCALE_MAGIC	0x12345678


/* On disk structure of LC_MONETARY locale data */
typedef struct
{
	u_long  magic;
	u_short len;

	u_short int_curr_symbol_off;
	u_short currency_symbol_off;
	u_short mon_decimal_point_off;
	u_short mon_thousands_sep_off;
	u_short mon_grouping_off;
	u_short positive_sign_off;
	u_short negative_sign_off;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
} _MonetaryLocaleHeader;

/* In core structure of LC_MONETARY locale data */
typedef struct
{
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;

	char data[0];
} _MonetaryLocale;

extern const _MonetaryLocale *_CurrentMonetaryLocale;
extern const _MonetaryLocale  _DefaultMonetaryLocale;

	
/* On disk structure of LC_NUMERIC locale data */
typedef struct
{
	u_long  magic;
	u_short len;

	u_short decimal_point_off;
	u_short thousands_sep_off;
	u_short grouping_off;
} _NumericLocaleHeader;

/* In core structure of LC_NUMERIC locale data */
typedef struct
{
	const char *decimal_point;
	const char *thousands_sep;
	const char *grouping;

	char        data[0];
} _NumericLocale;

extern const _NumericLocale *_CurrentNumericLocale;
extern const _NumericLocale  _DefaultNumericLocale;


/* On disk structure of LC_TIME locale data */
typedef struct 
{
	u_long	magic;		/* magic number */
	u_short len;		/* len of variable-size data after hdr */

	u_short	abday_off[7];	/* offsets of strings in data block */
	u_short day_off[7];
	u_short abmon_off[12];
	u_short mon_off[12];
	u_short am_pm_off[2];
	u_short d_t_fmt_off;
	u_short d_fmt_off;
	u_short t_fmt_off;
	u_short t_fmt_ampm_off;
} _TimeLocaleHeader;

/* In core structure of LC_TIME locale data */
typedef struct {
	const char *abday[7];
	const char *day[7];
	const char *abmon[12];
	const char *mon[12];
	const char *am_pm[2];
	const char *d_t_fmt;
	const char *d_fmt;
	const char *t_fmt;
	const char *t_fmt_ampm;

	char        data[0];
} _TimeLocale;

extern const _TimeLocale *_CurrentTimeLocale;
extern const _TimeLocale  _DefaultTimeLocale;

#endif /* _LOCALE_PRIVATE */

#endif /* _LOCALE_H_ */
