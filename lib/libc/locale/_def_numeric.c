/*	$NetBSD: _def_numeric.c,v 1.4 1997/04/29 16:40:16 kleink Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/localedef.h>
#include <locale.h>

const _NumericLocale _DefaultNumericLocale = 
{
	".",
	"",
	""
};

const _NumericLocale *_CurrentNumericLocale = &_DefaultNumericLocale;
