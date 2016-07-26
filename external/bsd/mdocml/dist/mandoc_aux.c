/*	Id: mandoc_aux.c,v 1.9 2015/11/07 14:22:29 schwarze Exp  */
/*
 * Copyright (c) 2009, 2011 Kristaps Dzonsons <kristaps@bsd.lv>
 * Copyright (c) 2014 Ingo Schwarze <schwarze@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "config.h"

#include <sys/types.h>

#if HAVE_ERR
#include <err.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mandoc.h"
#include "mandoc_aux.h"


int
mandoc_asprintf(char **dest, const char *fmt, ...)
{
	va_list	 ap;
	int	 ret;

	va_start(ap, fmt);
	ret = vasprintf(dest, fmt, ap);
	va_end(ap);

	if (ret == -1)
		err((int)MANDOCLEVEL_SYSERR, NULL);
	return ret;
}

void *
mandoc_calloc(size_t num, size_t size)
{
	void	*ptr;

	ptr = calloc(num, size);
	if (ptr == NULL)
		err((int)MANDOCLEVEL_SYSERR, NULL);
	return ptr;
}

void *
mandoc_malloc(size_t size)
{
	void	*ptr;

	ptr = malloc(size);
	if (ptr == NULL)
		err((int)MANDOCLEVEL_SYSERR, NULL);
	return ptr;
}

void *
mandoc_realloc(void *ptr, size_t size)
{

	ptr = realloc(ptr, size);
	if (ptr == NULL)
		err((int)MANDOCLEVEL_SYSERR, NULL);
	return ptr;
}

void *
mandoc_reallocarray(void *ptr, size_t num, size_t size)
{

	ptr = reallocarray(ptr, num, size);
	if (ptr == NULL)
		err((int)MANDOCLEVEL_SYSERR, NULL);
	return ptr;
}

char *
mandoc_strdup(const char *ptr)
{
	char	*p;

	p = strdup(ptr);
	if (p == NULL)
		err((int)MANDOCLEVEL_SYSERR, NULL);
	return p;
}

char *
mandoc_strndup(const char *ptr, size_t sz)
{
	char	*p;

	p = mandoc_malloc(sz + 1);
	memcpy(p, ptr, sz);
	p[(int)sz] = '\0';
	return p;
}
