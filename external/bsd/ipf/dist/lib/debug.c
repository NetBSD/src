/*	$NetBSD: debug.c,v 1.1.1.1.2.3 2012/10/30 18:55:04 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: debug.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
 */

#if defined(__STDC__)
# include <stdarg.h>
#else
# include <varargs.h>
#endif
#include <stdio.h>

#include "ipf.h"
#include "opts.h"

int	debuglevel = 0;


#ifdef	__STDC__
void	debug(int level, char *fmt, ...)
#else
void	debug(level, fmt, va_alist)
	int level;
	char *fmt;
	va_dcl
#endif
{
	va_list pvar;

	va_start(pvar, fmt);

	if ((debuglevel > 0) && (level <= debuglevel))
		vfprintf(stderr, fmt, pvar);
	va_end(pvar);
}


#ifdef	__STDC__
void	ipfkdebug(char *fmt, ...)
#else
void	ipfkdebug(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list pvar;

	va_start(pvar, fmt);

	if (opts & OPT_DEBUG)
		debug(0x1fffffff, fmt, pvar);
	va_end(pvar);
}
