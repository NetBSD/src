/*	$NetBSD: mem.c,v 1.20 2022/05/20 21:18:54 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: mem.c,v 1.20 2022/05/20 21:18:54 rillig Exp $");
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lint.h"

#if defined(IS_LINT1) || defined(IS_LINT2)
size_t
mem_block_size(void)
{
	unsigned int pagesize;

	pagesize = (unsigned int)getpagesize();
	return (MBLKSIZ + pagesize - 1) / pagesize * pagesize;
}
#endif

static void *
not_null(void *ptr)
{

	if (ptr == NULL)
		errx(1, "virtual memory exhausted");
	return ptr;
}

void *
xmalloc(size_t s)
{

	return not_null(malloc(s));
}

void *
xcalloc(size_t n, size_t s)
{

	return not_null(calloc(n, s));
}

void *
xrealloc(void *p, size_t s)
{

	return not_null(realloc(p, s));
}

char *
xstrdup(const char *s)
{

	return not_null(strdup(s));
}

#if defined(IS_XLINT)
char *
xasprintf(const char *fmt, ...)
{
	char *str;
	int e;
	va_list ap;

	va_start(ap, fmt);
	e = vasprintf(&str, fmt, ap);
	va_end(ap);
	if (e < 0)
		(void)not_null(NULL);
	return str;
}
#endif
