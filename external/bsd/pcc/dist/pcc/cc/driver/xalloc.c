/*	Id: xalloc.c,v 1.2 2011/05/26 16:48:40 plunky Exp 	*/	
/*	$NetBSD: xalloc.c,v 1.1.1.1 2011/09/01 12:47:05 plunky Exp $	*/

/*-
 * Copyright (c) 2011 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xalloc.h"

static void
error(const char *str)
{
	fprintf(stderr, "%s\n", str);
	exit(1);
}

void *
xcalloc(size_t elem, size_t len)
{
	void *ptr;

	if ((ptr = calloc(elem, len)) == NULL)
		error("calloc failed");
	return ptr;
}

void *
xmalloc(size_t len)
{
	void *ptr;

	if ((ptr = malloc(len)) == NULL)
		error("malloc failed");
	return ptr;
}

void *
xrealloc(void *buf, size_t len)
{
	void *ptr;

	if ((ptr = realloc(buf, len)) == NULL)
		error("realloc failed");
	return ptr;
}

char *
xstrdup(const char *str)
{
	char *buf;

	if ((buf = strdup(str)) == NULL)
		error("strdup failed");
	return buf;
}
