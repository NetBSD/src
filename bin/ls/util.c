/*	$NetBSD: util.c,v 1.27 2003/09/22 02:43:20 jschauma Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Fischbein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)util.c	8.5 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: util.c,v 1.27 2003/09/22 02:43:20 jschauma Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <fts.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vis.h>

#include "ls.h"
#include "extern.h"

int
safe_print(const char *src)
{
	size_t len;
	char *name;
	int flags;

	flags = VIS_NL | VIS_OCTAL;
	if (f_octal_escape)
		flags |= VIS_CSTYLE;

	len = strlen(src);
	if (len != 0 && SIZE_T_MAX/len <= 4) {
		errx(EXIT_FAILURE, "%s: name too long", src);
		/* NOTREACHED */
	}

	name = (char *)malloc(4*len+1);
	if (name != NULL) {
		len = strvis(name, src, flags);
		printf("%s", name);
		free(name);
		return len;
	} else
		errx(EXIT_FAILURE, "out of memory!");
		/* NOTREACHED */
}

int
printescaped(const char *src)
{
	unsigned char c;
	int n;

	for (n = 0; (c = *src) != '\0'; ++src, ++n)
		if (isprint(c))
			(void)putchar(c);
		else
			(void)putchar('?');
	return n;
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: ls [-AaBbCcdFfgikLlmnopqRrSsTtuWwx1] [file ...]\n");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
