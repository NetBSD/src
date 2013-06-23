/*	$NetBSD: localeconv.c,v 1.3.62.1 2013/06/23 06:26:20 tls Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#define _NETBSD_SOURCE
#include <sys/cdefs.h>
#include <sys/localedef.h>
#include <locale.h>
#include <limits.h>

/* 
 * The localeconv() function constructs a struct lconv from the current
 * monetary and numeric locales.
 */

/*
 * Return the current locale conversion.
 * Fixed in the "C" locale.
 */
struct lconv *
localeconv(void)
{
    static struct lconv ret = {
	/* char	*decimal_point */ ".",
	/* char	*thousands_sep */ "",
	/* char	*grouping */ "",
	/* char	*int_curr_symbol */ "",
	/* char	*currency_symbol */ "",
	/* char	*mon_decimal_point */ "",
	/* char	*mon_thousands_sep */ "",
	/* char	*mon_grouping */ "",
	/* char	*positive_sign */ "",
	/* char	*negative_sign */ "",
	/* char	int_frac_digits */ CHAR_MAX,
	/* char	frac_digits */ CHAR_MAX,
	/* char	p_cs_precedes */ CHAR_MAX,
	/* char	p_sep_by_space */ CHAR_MAX,
	/* char	n_cs_precedes */ CHAR_MAX,
	/* char	n_sep_by_space */ CHAR_MAX,
	/* char	p_sign_posn */ CHAR_MAX,
	/* char	n_sign_posn */ CHAR_MAX,
    };

    return (&ret);
}

/* ARGSUSED */
struct lconv *
localeconv_l(locale_t loc)
{
	return localeconv();
}
