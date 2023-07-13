/*	$NetBSD: msg.c,v 1.24 2023/07/13 08:40:38 rillig Exp $	*/

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
 *	This product includes software developed by Jochen Pohl for
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
__RCSID("$NetBSD: msg.c,v 1.24 2023/07/13 08:40:38 rillig Exp $");
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "lint2.h"

static	const	char *msgs[] = {
	"%s used( %s ), but not defined",			      /* 0 */
	"%s defined( %s ), but never used",			      /* 1 */
	"%s declared( %s ), but never used or defined",		      /* 2 */
	"%s multiply defined  \t%s  ::  %s",			      /* 3 */
	"%s value used inconsistently  \t%s  ::  %s",		      /* 4 */
	"%s value declared inconsistently (%s != %s) \t%s  ::  %s",   /* 5 */
	"%s, arg %d used inconsistently  \t%s[%s]  ::  %s[%s]",	      /* 6 */
	"%s: variable # of args  \t%s  ::  %s",			      /* 7 */
	"%s returns value which is always ignored",		      /* 8 */
	"%s returns value which is sometimes ignored",		      /* 9 */
	"%s value is used( %s ), but none returned",		      /* 10 */
	"%s, arg %d declared inconsistently (%s != %s)\t%s :: %s",    /* 11 */
	"%s: variable # of args declared  \t%s  ::  %s",	      /* 12 */
	"%s: malformed format string  \t%s",			      /* 13 */
	"%s, arg %d inconsistent with format  \t%s",		      /* 14 */
	"%s: too few args for format  \t%s",			      /* 15 */
	"%s: too many args for format  \t%s",			      /* 16 */
	"%s function value must be declared before use  \t%s  ::  %s",/* 17 */
	"%s renamed multiple times  \t%s  ::  %s",		      /* 18 */
};

void
msg(int n, ...)
{
	va_list ap;

	va_start(ap, n);

	(void)vprintf(msgs[n], ap);
	(void)printf("\n");

	va_end(ap);
}

/*
 * Return a pointer to the last component of a path.
 */
static const char *
lbasename(const char *path)
{

	if (Fflag)
		return path;

	const char *base = path;
	for (const char *p = path; *p != '\0'; p++)
		if (*p == '/')
			base = p + 1;
	return base;
}

/*
 * Create a string which describes a position in a source file.
 */
const char *
mkpos(const pos_t *posp)
{
	static struct buffer {
		char *buf;
		size_t cap;
	} buffers[2];
	static unsigned int buf_index;

	struct buffer *buf = buffers + buf_index;
	buf_index ^= 1;

	int filename;
	int lineno;
	if (Hflag && posp->p_src != posp->p_isrc) {
		filename = posp->p_isrc;
		lineno = posp->p_iline;
	} else {
		filename = posp->p_src;
		lineno = posp->p_line;
	}

	bool qm = !Hflag && posp->p_src != posp->p_isrc;
	const char *fn = lbasename(fnames[filename]);
	size_t len = strlen(fn) + 1 + 1 + 3 * sizeof(int) + 1 + 1;

	if (len > buf->cap)
		buf->buf = xrealloc(buf->buf, buf->cap = len);
	if (lineno != 0)
		(void)snprintf(buf->buf, buf->cap, "%s%s(%d)",
		    fn, qm ? "?" : "", lineno);
	else
		(void)snprintf(buf->buf, buf->cap, "%s", fn);

	return buf->buf;
}
