/*	$NetBSD: string.c,v 1.3 2016/06/23 05:19:42 pgoyette Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/param.h>
#include <sys/string.h>

#define	IS_DIGIT(c)	((c) >= '0' && (c) <= '9')

#define	IS_ALPHA(c)	\
	(((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

#ifndef __NetBSD__
char *
strpbrk(const char *s, const char *b)
{
	const char *p;

	do {
		for (p = b; *p != '\0' && *p != *s; ++p)
			;
		if (*p != '\0')
			return ((char *)s);
	} while (*s++);

	return (NULL);
}
#endif

/*
 * Convert a string into a valid C identifier by replacing invalid
 * characters with '_'.  Also makes sure the string is nul-terminated
 * and takes up at most n bytes.
 */
void
strident_canon(char *s, size_t n)
{
	char c;
	char *end = s + n - 1;

	if ((c = *s) == 0)
		return;

	if (!IS_ALPHA(c) && c != '_')
		*s = '_';

	while (s < end && ((c = *(++s)) != 0)) {
		if (!IS_ALPHA(c) && !IS_DIGIT(c) && c != '_')
			*s = '_';
	}
	*s = 0;
}

/*
 * Simple-minded conversion of a long into a null-terminated character
 * string.  Caller must ensure there's enough space to hold the result.
 */
void
numtos(unsigned long num, char *s)
{
	char prbuf[40];

	char *cp = prbuf;

	do {
		*cp++ = "0123456789"[num % 10];
		num /= 10;
	} while (num);

	do {
		*s++ = *--cp;
	} while (cp > prbuf);
	*s = '\0';
}
