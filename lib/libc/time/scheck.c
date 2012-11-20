/*	$NetBSD: scheck.c,v 1.8.12.1 2012/11/20 03:00:43 tls Exp $	*/

/*
** This file is in the public domain, so clarified as of
** 2006-07-17 by Arthur David Olson.
*/

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>

#ifndef lint
#if 0
static char	elsieid[] = "@(#)scheck.c	8.19";
#else
__RCSID("$NetBSD: scheck.c,v 1.8.12.1 2012/11/20 03:00:43 tls Exp $");
#endif
#endif /* !defined lint */

/*LINTLIBRARY*/

#include "private.h"

const char *
scheck(const char *const string, const char *const format)
{
	register char *		fbuf;
	register const char *	fp;
	register char *		tp;
	register int		c;
	register const char *	result;
	char			dummy;

	result = "";
	if (string == NULL || format == NULL)
		return result;
	fbuf = malloc(2 * strlen(format) + 4);
	if (fbuf == NULL)
		return result;
	fp = format;
	tp = fbuf;
	while ((*tp++ = c = *fp++) != '\0') {
		if (c != '%')
			continue;
		if (*fp == '%') {
			*tp++ = *fp++;
			continue;
		}
		*tp++ = '*';
		if (*fp == '*')
			++fp;
		while (is_digit(*fp))
			*tp++ = *fp++;
		if (*fp == 'l' || *fp == 'h')
			*tp++ = *fp++;
		else if (*fp == '[')
			do *tp++ = *fp++;
				while (*fp != '\0' && *fp != ']');
		if ((*tp++ = *fp++) == '\0')
			break;
	}
	*(tp - 1) = '%';
	*tp++ = 'c';
	*tp = '\0';
	if (sscanf(string, fbuf, &dummy) != 1)
		result = format;
	free(fbuf);
	return result;
}
