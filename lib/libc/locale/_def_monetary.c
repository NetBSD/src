/*	$NetBSD: _def_monetary.c,v 1.7 2003/07/26 19:24:46 salo Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <sys/localedef.h>
#include <limits.h>
#include <locale.h>

const _MonetaryLocale _DefaultMonetaryLocale = 
{
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX,
	(char)CHAR_MAX
};

const _MonetaryLocale *_CurrentMonetaryLocale = &_DefaultMonetaryLocale;
