/*	$NetBSD: _def_monetary.c,v 1.6 2001/02/09 10:55:48 wiz Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
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
