/*	$NetBSD: _def_numeric.c,v 1.5 2003/07/26 19:24:46 salo Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
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
