/*	$NetBSD: util.c,v 1.17 2003/08/07 11:25:17 agc Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *
 *	from: @(#)util.c	8.1 (Berkeley) 6/6/93
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include "defs.h"

static void nomem(void);
static void vxerror(const char *, int, const char *, va_list)
	     __attribute__((__format__(__printf__, 3, 0)));
static void vxwarn(const char *, int, const char *, va_list)
	     __attribute__((__format__(__printf__, 3, 0)));
static void vxmsg(const char *fname, int line, const char *class, 
		  const char *fmt, va_list)
     __attribute__((__format__(__printf__, 4, 0)));

/*
 * Malloc, with abort on error.
 */
void *
emalloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		nomem();
	return (p);
}

/*
 * Realloc, with abort on error.
 */
void *
erealloc(void *p, size_t size)
{

	if ((p = realloc(p, size)) == NULL)
		nomem();
	return (p);
}

/*
 * Strdup, with abort on error.
 */
char *
estrdup(const char *p)
{
	char *cp;

	if ((cp = strdup(p)) == NULL)
		nomem();
	return (cp);
}

static void
nomem(void)
{

	(void)fprintf(stderr, "config: out of memory\n");
	exit(1);
}

/*
 * Push a prefix onto the prefix stack.
 */
void
prefix_push(const char *path)
{
	struct prefix *pf;
	char *cp;

	pf = emalloc(sizeof(struct prefix));

	if (! SLIST_EMPTY(&prefixes) && *path != '/') {
		cp = emalloc(strlen(SLIST_FIRST(&prefixes)->pf_prefix) + 1 +
		    strlen(path) + 1);
		(void) sprintf(cp, "%s/%s",
		    SLIST_FIRST(&prefixes)->pf_prefix, path);
		pf->pf_prefix = intern(cp);
		free(cp);
	} else
		pf->pf_prefix = intern(path);

	SLIST_INSERT_HEAD(&prefixes, pf, pf_next);
}

/*
 * Pop a prefix off the prefix stack.
 */
void
prefix_pop(void)
{
	struct prefix *pf;

	if ((pf = SLIST_FIRST(&prefixes)) == NULL) {
		error("no prefixes on the stack to pop");
		return;
	}

	SLIST_REMOVE_HEAD(&prefixes, pf_next);
	/* Remember this prefix for emitting -I... directives later. */
	SLIST_INSERT_HEAD(&allprefixes, pf, pf_next);
}

/*
 * Prepend the source path to a file name.
 */
char *
sourcepath(const char *file)
{
	size_t len;
	char *cp;
	struct prefix *pf;

	pf = SLIST_EMPTY(&prefixes) ? NULL : SLIST_FIRST(&prefixes);
	if (pf != NULL && *pf->pf_prefix == '/')
		len = strlen(pf->pf_prefix) + 1 + strlen(file) + 1;
	else {
		len = strlen(srcdir) + 1 + strlen(file) + 1;
		if (pf != NULL)
			len += strlen(pf->pf_prefix) + 1;
	}

	cp = emalloc(len);

	if (pf != NULL) {
		if (*pf->pf_prefix == '/')
			(void) sprintf(cp, "%s/%s", pf->pf_prefix, file);
		else
			(void) sprintf(cp, "%s/%s/%s", srcdir,
			    pf->pf_prefix, file);
	} else
		(void) sprintf(cp, "%s/%s", srcdir, file);
	return (cp);
}

static struct nvlist *nvfreelist;

struct nvlist *
newnv(const char *name, const char *str, void *ptr, int i, struct nvlist *next)
{
	struct nvlist *nv;

	if ((nv = nvfreelist) == NULL)
		nv = emalloc(sizeof(*nv));
	else
		nvfreelist = nv->nv_next;
	nv->nv_next = next;
	nv->nv_name = name;
	if (ptr == NULL)
		nv->nv_str = str;
	else {
		if (str != NULL)
			panic("newnv");
		nv->nv_ptr = ptr;
	}
	nv->nv_int = i;
	return (nv);
}

/*
 * Free an nvlist structure (just one).
 */
void
nvfree(struct nvlist *nv)
{

	nv->nv_next = nvfreelist;
	nvfreelist = nv;
}

/*
 * Free an nvlist (the whole list).
 */
void
nvfreel(struct nvlist *nv)
{
	struct nvlist *next;

	for (; nv != NULL; nv = next) {
		next = nv->nv_next;
		nv->nv_next = nvfreelist;
		nvfreelist = nv;
	}
}

void
warn(const char *fmt, ...)
{
	va_list ap;
	extern const char *yyfile;

	va_start(ap, fmt);
	vxwarn(yyfile, currentline(), fmt, ap);
	va_end(ap);
}


static void
vxwarn(const char *file, int line, const char *fmt, va_list ap)
{
	vxmsg(file, line, "warning: ", fmt, ap);
}

/*
 * External (config file) error.  Complain, using current file
 * and line number.
 */
void
error(const char *fmt, ...)
{
	va_list ap;
	extern const char *yyfile;

	va_start(ap, fmt);
	vxerror(yyfile, currentline(), fmt, ap);
	va_end(ap);
}

/*
 * Delayed config file error (i.e., something was wrong but we could not
 * find out about it until later).
 */
void
xerror(const char *file, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vxerror(file, line, fmt, ap);
	va_end(ap);
}

/*
 * Internal form of error() and xerror().
 */
static void
vxerror(const char *file, int line, const char *fmt, va_list ap)
{
	vxmsg(file, line, "", fmt, ap);
	errors++;
}


/*
 * Internal error, abort.
 */
__dead void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)fprintf(stderr, "config: panic: ");
	(void)vfprintf(stderr, fmt, ap);
	(void)putc('\n', stderr);
	va_end(ap);
	exit(2);
}

/*
 * Internal form of error() and xerror().
 */
static void
vxmsg(const char *file, int line, const char *msgclass, const char *fmt,
      va_list ap)
{

	(void)fprintf(stderr, "%s:%d: %s", file, line, msgclass);
	(void)vfprintf(stderr, fmt, ap);
	(void)putc('\n', stderr);
}
