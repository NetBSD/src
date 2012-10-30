/*	$NetBSD: verbose.c,v 1.1.1.1.2.3 2012/10/30 18:55:12 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: verbose.c,v 1.1.1.2 2012/07/22 13:44:43 darrenr Exp $
 */

#if defined(__STDC__)
# include <stdarg.h>
#else
# include <varargs.h>
#endif
#include <stdio.h>

#include "ipf.h"
#include "opts.h"


#if defined(__STDC__)
void	verbose(int level, char *fmt, ...)
#else
void	verbose(level, fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list pvar;

	va_start(pvar, fmt);

	if (opts & OPT_VERBOSE)
		vprintf(fmt, pvar);
	va_end(pvar);
}


#if defined(__STDC__)
void	ipfkverbose(char *fmt, ...)
#else
void	ipfkverbose(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list pvar;

	va_start(pvar, fmt);

	if (opts & OPT_VERBOSE)
		verbose(0x1fffffff, fmt, pvar);
	va_end(pvar);
}
