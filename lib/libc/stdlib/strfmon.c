/*	$NetBSD: strfmon.c,v 1.23 2026/11/17 00:03:57 kre Exp $	*/

/*-
 * Copyright (c) 2001 Alexey Zelkin <phantom@FreeBSD.org>
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
 *
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
__FBSDID("$FreeBSD: src/lib/libc/stdlib/strfmon.c,v 1.14 2003/03/20 08:18:55 ache Exp $");
#else
__RCSID("$NetBSD: strfmon.c,v 1.23 2026/11/17 00:03:57 kre Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <monetary.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "setlocale_local.h"

/* internal flags */
#define	NEED_GROUPING		0x01	/* print digits grouped (default) */
#define	SIGN_POSN_USED		0x02	/* '+' or '(' usage flag */
#define	LOCALE_POSN		0x04	/* use locale defined +/- (default) */
#define	PARENTH_POSN		0x08	/* enclose negative amount in () */
#define	SUPPRESS_CURR_SYMBOL	0x10	/* suppress the currency from output */
#define	LEFT_JUSTIFY		0x20	/* left justify */
#define	USE_INTL_CURRENCY	0x40	/* use international currency symbol */
#define	IS_NEGATIVE		0x80	/* is argument value negative ? */

/* internal macros */
#define	PRINT(CH) do {						\
	if (dst >= s + maxsize) 				\
		goto e2big_error;				\
	*dst++ = CH;						\
} while (0)

#define	PRINTS(STR) do {					\
	const char *tmps = STR;					\
	while (*tmps != '\0')					\
		PRINT(*tmps++);					\
} while (0)

#define PRINT_WIDE(WCH, CH) do {				\
	int __w__ = __add_wide_char(dst, s + maxsize, (WCH));	\
	int __c__ = (CH);					\
	switch (__w__) {					\
	case -2:	goto e2big_error;		break;	\
	case -1:	if (__c__ != 0) PRINT(__c__);	break;	\
	default:	dst += __w__;			break;	\
	}							\
} while (0)

#define	GET_NUMBER(VAR, LOC)	do {				\
	VAR = 0;						\
	while (isdigit_l((unsigned char)*fmt, LOC)) {		\
		if (VAR > INT_MAX / 10)				\
			goto e2big_error;			\
		VAR *= 10;					\
		VAR += *fmt - '0';				\
		if (VAR < 0)					\
			goto e2big_error;			\
		fmt++;						\
	}							\
} while (0)

#define	GRPCPY(howmany) do {					\
	int i = howmany;					\
	while (i-- > 0) {					\
		avalue_size--;					\
		*--bufend = *(avalue + avalue_size + padded);	\
	}							\
} while (0)

#define	GRPSEP do {						\
	bufend -= thousands_sep_size;				\
	memcpy(bufend, thousands_sep, thousands_sep_size);	\
	groups++;						\
} while (0)

static void __setup_vars(int, char *, char *, char *, const char **,
    struct lconv *);
static int __calc_left_pad(int, char *, int, int, struct lconv *);
static int __calc_right_pad(int, char *, int, int, struct lconv *);
static char *__format_grouped_double(double, int *, int *, int, int,
    char *, size_t, struct lconv *, locale_t loc);
static int __columns(const char *, locale_t);
static int __count_grps(int, char *);
static int __add_wide_char(char *, char *, wchar_t);

static char __nullstr[] = "";

/*
 * This would be better as MB_CUR_MAX but that isn't a constant,
 * and not using a constant screws with the stack protector, so
 * let's not do that.   15 is much too big for any rational encoding
 * of a single character, but still so small that an array 16 bytes
 * (15 +1 for a \0) on the stack isn't any kind of a problem.
 */
#define MAX_ENCODED_CHAR_LEN	15

static ssize_t
vstrfmon_l(char * __restrict s, size_t maxsize, locale_t loc,
    const char * __restrict format, va_list ap)
{
	char 		*dst;		/* output destination pointer */
	const char 	*fmt;		/* current format poistion pointer */
	struct lconv 	*lc;		/* pointer to lconv structure */
	char		*asciivalue;	/* formatted double pointer */

	int		flags;		/* formatting options */
	char		pad_char[MAX_ENCODED_CHAR_LEN+1];  /* padding char */
	size_t		pad_char_len;
	int		pad_size;	/* pad size */
	int		width;		/* field width */
	int		left_prec;	/* left precision */
	int		right_prec;	/* right precision */
	double		value;		/* just value */
	wchar_t		space_char = L' '; /* space after currency */

	char		cs_precedes,	/* values gathered from struct lconv */
			sep_by_space,
			sign_posn, osp,
			*currency_symbol;
	const char	*signstr, *othersign;
	int		cs_cols, ss_cols, os_cols;

	int		cols_used;

	mbstate_t	mbs;

	char		*tmpptr;	/* temporary vars */
	int		sverrno;

	mbstate_t	fmt_state;
	wchar_t		fmt_char;
	size_t		fmt_ch_len;

	lc = localeconv_l(loc);
	dst = s;
	fmt = format;
	asciivalue = NULL;
	currency_symbol = NULL;

	(void)mbrtowc(NULL, NULL, 0, &fmt_state);	/* init */
	for (;;) {
		fmt_ch_len = mbrtowc(&fmt_char, fmt, MB_CUR_MAX,
		    &fmt_state);
		if (fmt_ch_len == 0)
			break;
		if (fmt_ch_len == (size_t)-1)	/* errno == EILSEQ */
			goto end_error;
		if (fmt_ch_len == (size_t)-2)
			goto format_error;

		if (fmt_char != L'%')
			goto literal;

		/* '%' found ! */

		/* "%%" mean just '%' */
		if (fmt[1] == '%') {
			fmt++;
	literal:;
			PRINT_WIDE(fmt_char, '?');
			fmt += fmt_ch_len;
			continue;
		}

		/* set up initial values */
		flags = NEED_GROUPING | LOCALE_POSN;
		pad_char[0] = ' ';	/* padding character is "space" */
		pad_char[1] = '\0';
		pad_char_len = 1;
		pad_size = 0;		/* no padding initially */
		left_prec = -1;		/* no left precision specified */
		right_prec = -1;	/* no right precision specified */
		width = -1;		/* no width specified */
		value = 0;		/* we have no value to print now */
		cols_used = 0;		/* output width in columns */

		/* Flags */
		for (;;) {
			/*
			 * Once we are in a conversion description
			 * all chars are required to be ascii, except
			 * the one char after a '=' flag, so we can
			 * just read them as bytes ... or this is close
			 * enough for now.
			 */
			switch (*++fmt) {
			case '=':	/* fill character */
				(void)mbrtowc(NULL, NULL, 0, &mbs);
				pad_char_len = mbrtowc(&space_char,
				    ++fmt, MAX_ENCODED_CHAR_LEN, &mbs);
				if (pad_char_len == 0 ||
				    pad_char_len >= MAX_ENCODED_CHAR_LEN)
					goto format_error;
				memcpy(pad_char, fmt, pad_char_len);
				pad_char[pad_char_len] = '\0';
				fmt += pad_char_len - 1;

				/*
				 * return the borrowed var to its
				 * regularly scheduled purpose
				 */
				space_char = L' ';
				continue;

			case '^':	/* not group currency  */
				flags &= ~NEED_GROUPING;
				continue;

			case '+':	/* use locale defined signs */
				if (flags & (SIGN_POSN_USED|PARENTH_POSN))
					goto format_error;
				flags |= SIGN_POSN_USED;
				continue;

			case '(':	/* enclose negatives with () */
				if (flags & SIGN_POSN_USED)
					goto format_error;
				flags &= ~LOCALE_POSN;	/* was dflt */
				flags |= SIGN_POSN_USED | PARENTH_POSN;
				continue;

			case '!':	/* suppress currency symbol */
				flags |= SUPPRESS_CURR_SYMBOL;
				continue;

			case '-':	/* alignment (left)  */
				flags |= LEFT_JUSTIFY;
				continue;

			default:
				break;
			}
			break;
		}

		/* field Width */
		if (isdigit_l((unsigned char)*fmt, loc)) {
			ptrdiff_t d = dst - s;

			GET_NUMBER(width, loc);

			/*
			 * Do we have enough space to place a number of
			 * the required width ?
			 */
			if ((size_t)(d + width) >= maxsize)
				goto e2big_error;
		}

		/* Left precision */
		if (*fmt == '#') {
			if (!isdigit_l((unsigned char)*++fmt, loc))
				goto format_error;
			GET_NUMBER(left_prec, loc);

			if ((unsigned int)left_prec >= maxsize - (dst - s))
				goto e2big_error;
		}

		/* Right precision */
		if (*fmt == '.') {
			if (!isdigit_l((unsigned char)*++fmt, loc))
				goto format_error;
			GET_NUMBER(right_prec, loc);

			if ((unsigned int)right_prec >=
			    maxsize - (dst - s) - left_prec)
				goto e2big_error;
		}

		/* Conversion Characters */
		switch (*fmt++) {

		case 'i':	/* use international currency format */
			flags |= USE_INTL_CURRENCY;
			break;

		case 'n':	/* use national currency format */
			flags &= ~USE_INTL_CURRENCY;
			break;

		default:	/* required character is missing or
				   premature EOS */
			goto format_error;
		}

		space_char = L' ';	/* to be sure, to be sure... */
		if (currency_symbol != NULL)
			free(currency_symbol);
		if (flags & USE_INTL_CURRENCY) {
			size_t l;

			currency_symbol = strdup(lc->int_curr_symbol);
			if (currency_symbol != NULL &&
			    /*
			     * should be 3 characters, not bytes
			     * but the international currency symbol
			     * is supposed to be from ISO4217:2015, so
			     * only ASCII, so, just leave this (for now)
			     */
			    (l = strlen(currency_symbol)) > 3) {
				size_t mblen;
				const int e = errno;

				(void)mbrtowc_l(NULL, NULL, 0, &mbs, loc);
				/* any errno set here gets ignored, we deal */
				mblen = mbrtowc_l(&space_char,
				    currency_symbol + 3, MAX_ENCODED_CHAR_LEN,
				    &mbs, loc);
				if (mblen == 0 ||
				    mblen > MAX_ENCODED_CHAR_LEN ||
				    wcwidth(space_char) != 1)
					space_char = L' ';
				currency_symbol[3] = '\0';
				errno = e;
			}
		} else
			currency_symbol = strdup(lc->currency_symbol);

		if (currency_symbol == NULL)
			goto end_error;			/* ENOMEM. */

		cs_cols = __columns(currency_symbol, loc);
		if (cs_cols < 0) {
			cs_cols = 0;
			flags |= SUPPRESS_CURR_SYMBOL;
		}

		/* value itself */
		value = va_arg(ap, double);

		/* detect sign */
		if (value < 0) {
			flags |= IS_NEGATIVE;
			value = -value;
		}

		/*
		 * set some variables for later use
		 */
		/* First a couple for the other sign */
		__setup_vars(flags ^ IS_NEGATIVE, &cs_precedes,
		    &sep_by_space, &osp, &othersign, lc);
		/* And then the the actual data to use */
		__setup_vars(flags, &cs_precedes,
		    &sep_by_space, &sign_posn, &signstr, lc);

		ss_cols = __columns(signstr, loc);
		if (ss_cols < 0) {
			signstr = __nullstr;
			ss_cols = 0;
		}
		os_cols = __columns(othersign, loc);
		if (os_cols < 0) {
			othersign = __nullstr;
			os_cols = 0;
		}

#undef	CMP_SP
/*
 * Compare +/- sign positions
 *
 * This gives true if either both are zero, or they are different
 * and false only when they are the same and not zero
 */
#define CMP_SP(a, b) ((~lc-> a##_sign_posn ^ ~lc-> b##_sign_posn) != 0)

		/*
		 * If () will be used for negative numbers, it doesn't
		 * matter what the sign strings are.
		 */
		if (flags & PARENTH_POSN) {
			/*
			 * we were given the '(' flag, so
			 * everything is always OK.   **)**
			 */
		} else if (flags & SIGN_POSN_USED) {
			/*
			 * The '+' flag was used
			 */
			if (ss_cols == 0 && os_cols == 0)
				goto format_error;

			if (strcmp(signstr, othersign) == 0) {
			     /* sign strings identical */
			     /* error if positions the same */
			     if (sign_posn == osp)
				goto format_error;
			}

			if (sign_posn == 0)
				sign_posn = 1;
			/*
			 * We are instructed to use signs,
			 * and we have suiable strings to use
			 * then all is OK
			 */
		} else if (sign_posn == 0) {
			/*
			 * here the locale has told us to use
			 * parens, which is OK, unless it would
			 * also tell us to use parens for the
			 * values of the other sign.
			 */
			if (osp == 0)
				goto format_error;

		} else if (ss_cols != os_cols &&
			    (ss_cols > 0 || os_cols > 0)) {
			/*
			 * signs are clearly different, no issue
			 */
		} else if ((ss_cols > 0 || os_cols > 0) &&
			    (strcmp(signstr, othersign) != 0)) {
			/*
			 * sign strings are different, and at least
			 * one has a visible width, also no issue
			 */
		} else if (!(flags & SIGN_POSN_USED) &&
			    ((ss_cols > 0 || os_cols > 0) &&
			     ((flags & USE_INTL_CURRENCY) ?
			        CMP_SP(int_p, int_n) : CMP_SP(p, n)))) {
			/*
			 * '+' flag not used, and sign strings go in
			 * different places, and at least one of them
			 * will be visible, so OK if they look the
			 * same, no issue
			 */
		} else if (ss_cols <= 0 && os_cols <= 0) {
			/*
			 * Replace the negative sign with "-"
			 */
			if (flags & IS_NEGATIVE) {
				signstr = "-";
				ss_cols = 1;
			} else
				os_cols = 1;	/*othersign not used*/
		} else {
			/*
			 * arrange to place a negative sign before
			 * the value, and the positive sign after it.
			 * Now the cases are different!
			 */
			sign_posn = (flags & IS_NEGATIVE) ? 1 : 2;
		}
#undef	CMP_SP

		if (ss_cols < 0) {
			signstr = __nullstr;
			ss_cols = 0;
		}
		if (os_cols < 0) {
			/* othersign = __nullstr; // not needed */
			os_cols = 0;
		}

		/* fill left_prec with amount of padding chars */
		if (left_prec >= 0) {
			pad_size = __calc_left_pad((flags ^ IS_NEGATIVE),
				        currency_symbol, cs_cols, os_cols, lc)
				 - __calc_left_pad(flags,
					currency_symbol, cs_cols, ss_cols, lc);

			if (pad_size < 0)
				pad_size = 0;
		}

		if (asciivalue != NULL)
			free(asciivalue);
		asciivalue = __format_grouped_double(value, &flags, &cols_used,
		    left_prec, right_prec, pad_char, pad_char_len, lc, loc);
		if (asciivalue == NULL)
			goto end_error;		/* errno already set     */
						/* to ENOMEM by malloc() */

		/*
		 * Description of some LC_MONETARY's values:
		 *
		 * p_cs_precedes & n_cs_precedes
		 *
		 * = 1 - $currency_symbol precedes the value
		 *	 for a monetary quantity with a non-negative value
		 * = 0 - symbol succeeds the value
		 *
		 * p_sep_by_space & n_sep_by_space
		 *
		 * = 0 - no space separates $currency_symbol
		 *	 from the value for a monetary quantity with a
		 *	 non-negative value
		 * = 1 - space separates the symbol from the value
		 * = 2 - space separates the symbol and the sign string,
		 *	 if adjacent.
		 *
		 * p_sign_posn & n_sign_posn
		 *
		 * = 0 - parentheses enclose the quantity and the
		 *	 $currency_symbol
		 * = 1 - the sign string precedes the quantity and the 
		 *	 $currency_symbol
		 * = 2 - the sign string succeeds the quantity and the 
		 *	 $currency_symbol
		 * = 3 - the sign string precedes the $currency_symbol
		 * = 4 - the sign string succeeds the $currency_symbol
		 *
		 */

		tmpptr = dst;

		if (flags & PARENTH_POSN)
			sign_posn = 111;	/* invalid, so no signstr */

		if ((sign_posn == 0 && !(flags & SIGN_POSN_USED)) ||
		    (flags & (PARENTH_POSN|IS_NEGATIVE)) == 
					(PARENTH_POSN|IS_NEGATIVE)) {
			PRINT('(');
			cols_used++;
		}

		while (pad_size-- > 0) {
			PRINT(' ');
			cols_used++;
		}

		if (cs_precedes == 1) {
			if (sign_posn == 1 || sign_posn == 3 ||
			    (sign_posn == 0 && (flags & SIGN_POSN_USED))) {
				PRINTS(signstr);
				cols_used += ss_cols;
				if (sep_by_space == 2) {
					PRINT(' ');
					cols_used++;
				}
			}

			if (!(flags & SUPPRESS_CURR_SYMBOL)) {
				PRINTS(currency_symbol);
				cols_used += cs_cols;

				if (sign_posn == 4) {
					if (sep_by_space == 2) {
						PRINT(' ');
						cols_used++;
					}
					PRINTS(signstr);
					if (sep_by_space == 1) {
						PRINT(' ');
						cols_used++;
					}
				} else if (sep_by_space == 1) {
					cols_used++;	/* we know width==1 */
					PRINT_WIDE(space_char, ' ');
				}
			}
		} else if (sign_posn == 1 ||
			  (sign_posn == 0 && (flags & SIGN_POSN_USED))) {
			PRINTS(signstr);
			cols_used += ss_cols;
			if (sep_by_space == 2) {
				PRINT(' ');
				cols_used++;
			}
		}

		PRINTS(asciivalue);

		if (cs_precedes == 0) {
			if (sign_posn == 3) {
				if (sep_by_space == 1) {
					PRINT(' ');
					cols_used++;
				}
				PRINTS(signstr);
				cols_used += ss_cols;
			}

			if (!(flags & SUPPRESS_CURR_SYMBOL)) {
				if ((sign_posn == 3 && sep_by_space == 2)
				    || (sep_by_space == 1
				    && (sign_posn == 0 || sign_posn == 1 ||
					sign_posn == 2 || sign_posn == 4)  )) {
						cols_used++;
						PRINT_WIDE(space_char, ' ');
				}

				PRINTS(currency_symbol);
				cols_used += cs_cols;
				if (sign_posn == 4) {
					if (sep_by_space == 2) {
						PRINT(' ');
						cols_used++;
					}
					PRINTS(signstr);
					cols_used += ss_cols;
				}
			}
		}

		if (sign_posn == 2) {
			if (sep_by_space == 2) {
				PRINT(' ');
				cols_used++;
			}
			PRINTS(signstr);
			cols_used += ss_cols;
		}

		/* if aligning, pad the right side as well */
		if (left_prec >= 0) {
			pad_size = __calc_right_pad((flags ^ IS_NEGATIVE),
					currency_symbol, cs_cols, os_cols, lc)
				 - __calc_right_pad(flags,
					currency_symbol, cs_cols, ss_cols, lc);

			if (pad_size < 0)
				pad_size = 0;

			while (pad_size-- > 0) {
				PRINT(' ');
				cols_used++;
			}
		}

		if ((sign_posn == 0 && !(flags & SIGN_POSN_USED)) ||
		    (flags & (PARENTH_POSN|IS_NEGATIVE)) == 
					(PARENTH_POSN|IS_NEGATIVE)) {
			PRINT(')');
			cols_used++;
		}

		if (cols_used < width) {
			if (flags & LEFT_JUSTIFY) {
				while (cols_used++ < width)
					PRINT(' ');
			} else {
				size_t wps;

				wps = width - cols_used;
				memmove(tmpptr + wps, tmpptr,
				    (size_t)(dst - tmpptr));
				memset(tmpptr, ' ', wps);
				dst += wps;
			}
		}
	}

	PRINT('\0');
	free(asciivalue);
	free(currency_symbol);
	return (dst - s - 1);	/* return size of data except trailing '\0' */

 e2big_error:;
	errno = E2BIG;
	goto end_error;

 format_error:;
	errno = EINVAL;

 end_error:;
	sverrno = errno;
	if (asciivalue != NULL)
		free(asciivalue);
	if (currency_symbol != NULL)
		free(currency_symbol);
	errno = sverrno;
	return (-1);
}

static int
__add_wide_char(char *dst, char *limit, wchar_t wch)
{
	char sc[MAX_ENCODED_CHAR_LEN + 1];
	int clen;
	const int e = errno;

	(void)wctomb(NULL, L'\0');
	clen = wctomb(sc, wch);
	errno = e;
	if (clen > 0) {
		if (dst + clen >= limit)
			return -2;
		memcpy(dst, sc, (size_t)clen);
		return clen;
	}
	return -1;
}

static void
__setup_vars(int flags, char *cs_precedes, char *sep_by_space,
    char *sign_posn, const char **signstr, struct lconv *lc)
{

	if ((flags & IS_NEGATIVE) && (flags & USE_INTL_CURRENCY)) {
		*cs_precedes = lc->int_n_cs_precedes;
		*sep_by_space = lc->int_n_sep_by_space;
		*sign_posn = lc->int_n_sign_posn;
	} else if (flags & USE_INTL_CURRENCY) {
		*cs_precedes = lc->int_p_cs_precedes;
		*sep_by_space = lc->int_p_sep_by_space;
		*sign_posn = lc->int_p_sign_posn;
	} else if (flags & IS_NEGATIVE) {
		*cs_precedes = lc->n_cs_precedes;
		*sep_by_space = lc->n_sep_by_space;
		*sign_posn = lc->n_sign_posn;
	} else {
		*cs_precedes = lc->p_cs_precedes;
		*sep_by_space = lc->p_sep_by_space;
		*sign_posn = lc->p_sign_posn;
	}
	if ((flags & (PARENTH_POSN|IS_NEGATIVE)) == (PARENTH_POSN|IS_NEGATIVE))
		*sign_posn = 0;

	if (signstr != NULL) {
		if (flags & IS_NEGATIVE)
			*signstr = lc->negative_sign;
		else
			*signstr = lc->positive_sign;
	}

	/* Set default values for unspecified/invalid information. */
	if (*cs_precedes != 0)
		*cs_precedes = 1;
	if (*sep_by_space > 2 || (int)*sep_by_space < 0)
		*sep_by_space = 0;
	if (*sign_posn > 4 || (int)*sign_posn < 0)
		*sign_posn = 1;
}

/*
 * Func name is misleading (but historic), this calculates the number of
 * characters that are inserted before the value string for a value and
 * using options as given by flags (+/-, $ or not, ...)
 *
 * Taking the difference between what this returns for the sign of the
 * value we have, and what would be used for a value of the opposite sign,
 * will reveal how many pad chars need to be inserted.
 */
static int
__calc_left_pad(int flags, char *cur_symb, int cs_cols, int s_cols,
    struct lconv *lc)
{
	char cs_precedes, sep_by_space, sign_posn;
	int left_chars = 0;

	__setup_vars(flags, &cs_precedes, &sep_by_space, &sign_posn, NULL, lc);

	if ((flags & (PARENTH_POSN|IS_NEGATIVE)) ==
	    (PARENTH_POSN|IS_NEGATIVE) ||
	    (!(flags & SIGN_POSN_USED) && sign_posn == 0)) {
		left_chars++;	/* One char needed for '(' **)** */
		sign_posn = 0;
	}

	if (cs_precedes != 0) {
		/* space needed for currency symbol */
		left_chars += cs_cols;
		if (sep_by_space != 0)	/* either 1 or 2, space goes with $ */
			left_chars++;
	}

	switch (sign_posn) {
	case 0:
		/* paren marking, handled above */
		break;
	case 1:
		left_chars += s_cols;
		break;
	case 3:
	case 4:
		if (cs_precedes != 0)
			left_chars += s_cols;
		break;
	}
	return left_chars;
}

/*
 * The same for the right side
 */
static int
__calc_right_pad(int flags, char *cur_symb, int cs_cols, int s_cols,
    struct lconv *lc)
{
	char cs_precedes, sep_by_space, sign_posn;
	int right_chars = 0;

	__setup_vars(flags, &cs_precedes, &sep_by_space, &sign_posn, NULL, lc);

	if ((flags & (PARENTH_POSN|IS_NEGATIVE)) ==
	    (PARENTH_POSN|IS_NEGATIVE) ||
	    (!(flags & SIGN_POSN_USED) && sign_posn == 0)) {
		right_chars++;	/* **(** One char needed for ')' */
		sign_posn = 0;
	}

	if (cs_precedes == 0) {
		right_chars += cs_cols;
		if (sep_by_space != 0)
			right_chars++;
	}

	switch (sign_posn) {
	case 0:
		break;
	case 2:
		right_chars += s_cols;
		break;
	case 3:
	case 4:
		if (cs_precedes == 0)
			right_chars += s_cols;
		break;
	}
	return right_chars;
}

/*
 * Determine how many thousands_separator sequences will be
 * inserted into a string of <size> digits, using the
 * grouping algorithm given by <grouping>
 */
static int
__count_grps(int size, char *grouping)
{
	int	seps = 0;

	if (*grouping == CHAR_MAX || (int)*grouping <= 0) /* no grouping ? */
		return 0;

	while (size > (int)*grouping) {
		seps++;
		size -= (int)(unsigned char)*grouping++;

		/* no more grouping ? */
		if (*grouping == CHAR_MAX || (int)*grouping < 0)
			break;

		/* remainder grouping with same value ? */
		if (*grouping == 0) {
			/* known that: grouping[-1] != 0 */
			seps += (size - 1) / *(grouping - 1);
			break;
		}
	}
	return seps;
}

/* convert double to locale-encoded string */
static char *
__format_grouped_double(double value, int *flags, int *usedp, int left_prec,
    int right_prec, char *pad_char, size_t pad_char_len,
    struct lconv *lc, locale_t loc)
{
	char		*rslt;
	char		*avalue;
	int		avalue_size;

	size_t		bufsize;
	char		*bufend;

	int		padded;

	char		*grouping;
	const char	*decimal_point;
	const char	*thousands_sep;
	size_t		decimal_point_size;
	size_t		thousands_sep_size;
	int		decimal_point_cols;
	int		thousands_sep_cols;

	int		groups = 0;
	int		used = 0;

	grouping = lc->mon_grouping;
	decimal_point = lc->mon_decimal_point;
	if (*decimal_point == '\0')		/* POSIX will say to do this */
		decimal_point = lc->decimal_point;
	thousands_sep = lc->mon_thousands_sep;
#if 0
	if (*thousands_sep == '\0')		/* but not to do this */
		thousands_sep = lc->thousands_sep;
#endif

	decimal_point_size = strlen(decimal_point);
	thousands_sep_size = strlen(thousands_sep);
	decimal_point_cols = __columns(decimal_point, loc);
	thousands_sep_cols = __columns(thousands_sep, loc);

	/* fill left_prec with default value */
	if (left_prec == -1)
		left_prec = 0;

	/* fill right_prec with default value */
	if (right_prec == -1) {
		if (*flags & USE_INTL_CURRENCY)
			right_prec = lc->int_frac_digits;
		else
			right_prec = lc->frac_digits;

		if (right_prec == CHAR_MAX)	/* POSIX locale ? */
			right_prec = 0;
	}

	if (decimal_point_size == 0)
		right_prec = 0;

	if (*flags & NEED_GROUPING)
		left_prec +=
		    __count_grps(left_prec, grouping) * thousands_sep_cols;

	/*
	 * convert to string
	 *
	 * The field width specified allows for the minimum number of
	 * digits before the decimal point (left_prec), the number after
	 * it, and if that was not 0, 1 more for the '.'.
	 * nb: this is using printf(3), the decimal point is always '.'
	 * (and so is always 1 byte).
	 */
	avalue_size = asprintf_l(&avalue, loc, "%*.*f",
	    left_prec + right_prec + (right_prec ? 1 : 0), right_prec, value);
	if (avalue_size < 0)
		return (NULL);

	/*
	 * make sure that we've enough space for result string
	 * (it is OK to be more than enough)
	 *
	 * avalue_size are the digits, with often a '.', and leading
	 * padding to allow for the left_precision
	 *
	 * In extreme cases every character in the result might need
	 * a grouping character (really 1 less, but being bigger is OK)
	 * where each grouping character is thousands_sep_size bytes.
	 *
	 * Thus: avalue_size + (avalue_size * thousands_sep_size)
	 * or:   avalue_size * (1 + thousands_sep_size)
	 *
	 * After that we need to insert the locale radix char, which
	 * is decimal_point_size bytes (we could -1 for the '.' present)
	 *
	 * And one more for the \0 to end it all.
	 */
	bufsize = avalue_size * (1 + thousands_sep_size) +
	    decimal_point_size + 1;

	/* skip spaces at beginning */
	padded = 0;
	while (avalue[padded] == ' ') {
		padded++;
		avalue_size--;
	}
	bufsize += padded * (pad_char_len - 1);

	rslt = calloc(1, bufsize);
	if (rslt == NULL) {
		free(avalue);
		return (NULL);
	}
	bufend = rslt + bufsize - 1;	/* reserve space for trailing '\0' */

	if (right_prec > 0) {
		bufend -= right_prec;
		memcpy(bufend, avalue + avalue_size + padded - right_prec,
		    (size_t) right_prec);
		bufend -= decimal_point_size;
		memcpy(bufend, decimal_point, decimal_point_size);
		avalue_size -= (right_prec + 1);
		used += right_prec + decimal_point_cols;
	}

	/*
	 * If grouping has not been suppressed, and the width of
	 * the grouping separator is > 0 columns, and there is
	 * a defined (non-null) grouping algorithm, then we
	 * perform that algorithm here.
	 */
	if ((*flags & NEED_GROUPING) &&
	    thousands_sep_cols > 0 &&
	    *grouping != CHAR_MAX && *grouping > 0) {
		/* Moving from right to left */
		while (avalue_size > (int)*grouping) {
			/* insert the digits for the group */
			GRPCPY(*grouping);
			/* then a separator */
			GRPSEP;
			used += thousands_sep_cols + *grouping;
			/* and advance to the next step */
			grouping++;

			/* no more grouping ? */
			if (*grouping == CHAR_MAX || (int)*grouping < 0)
				break;

			/* rest grouping with same value ? */
			if (*grouping == 0) {
				/* go back to the last group size */
				grouping--;
				/* and use it until out of digits */
				while (avalue_size > *grouping) {
					GRPCPY(*grouping);
					GRPSEP;
					used += thousands_sep_cols + *grouping;
				}
				break;
			}
		}
		/* If digits remain uncopied, do them now */
		if (avalue_size != 0) {
			used += avalue_size;	/* must be before GRPCPY */
			GRPCPY(avalue_size);
		}
		/*
		 * we need N less padding chars for each separator inserted
		 * where N is the width of the separator
		 */
		padded -= groups * thousands_sep_cols;
	} else {
		/*
		 * No grouping to do, the easy case, just
		 * copy the digits
		 */
		bufend -= avalue_size;
		memcpy(bufend, avalue + padded, (size_t) avalue_size);
		used += avalue_size;
	}

	/* do padding with pad_char */
	if (padded > 0) {
		used += padded;
		if (pad_char_len == 1) {
			/* the most common case */
			bufend -= padded;
			memset(bufend, pad_char[0], padded);
		} else while (padded-- > 0) {
			bufend -= pad_char_len;
			memcpy(bufend, pad_char, pad_char_len);
		}
	}

	/* copy the formatted digit string into the output buffer */
	bufsize = rslt + bufsize - bufend;
	memmove(rslt, bufend, bufsize);
	free(avalue);		/* and we're done with that now */
	*usedp += used;
	return (rslt);
}

/*
 * Given a limited length multi-byte character string s
 * return the number of columns that will be used to display
 * that string.
 *
 * The string ends at the first \0 encountered within MAX_CH chars.
 * The whole string must be consumed, but at most MAX_CH-1 wide chars
 * (plus a null) can be present.
 *
 * This is not intended as a general purpose column counting function,
 * just to determine the widths of the various symbols used in strfmon()
 * - the decimmal_point, thousands_sep, and currency symbol.
 *
 * Returns -1 if any error is encountered (too much input, invalid
 * byte sequence, ...) or the number of columns required (>= 0).
 */
#define MAX_CH 8
static int
__columns(const char *s, locale_t loc)
{
	wchar_t widech[MAX_CH];	/* strings of up to 7 (wide) chars + nul */
	size_t count;
	mbstate_t ps;

	(void)mbrtowc_l(NULL, NULL, 1, &ps, loc);	/* init state */
	count = mbsrtowcs_l(widech, &s, MAX_CH, &ps, loc);
	if (count == (size_t)-1 || count > MAX_CH - 1 || s != NULL)
		return -1;
	return wcswidth_l(widech, count, loc);
}

ssize_t
strfmon(char * __restrict s, size_t maxsize,
    const char * __restrict format, ...)
{
	ssize_t ret;
	va_list ap;

	va_start(ap, format);
	ret = vstrfmon_l(s, maxsize, _current_locale(), format, ap);
	va_end(ap);

	return ret;
}

ssize_t
strfmon_l(char * __restrict s, size_t maxsize, locale_t loc,
    const char * __restrict format, ...)
{
	ssize_t ret;
	va_list ap;

	va_start(ap, format);
	ret = vstrfmon_l(s, maxsize, loc, format, ap);
	va_end(ap);

	return ret;
}
