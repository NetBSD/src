/*	$NetBSD: fsutil.c,v 1.15 2006/06/05 16:52:05 christos Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
__RCSID("$NetBSD: fsutil.c,v 1.15 2006/06/05 16:52:05 christos Exp $");
#endif /* not lint */

#include <sys/param.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fstab.h>
#include <err.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "fsutil.h"

static const char *dev = NULL;
static int hot = 0;
static int preen = 0;
int quiet;
#define F_ERROR	0x80000000

void
setcdevname(const char *cd, int pr)
{

	dev = cd;
	preen = pr;
}

const char *
cdevname(void)
{

	return dev;
}

int
hotroot(void)
{

	return hot;
}

/*VARARGS*/
void
errexit(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(8);
}

void
vmsg(int fatal, const char *fmt, va_list ap)
{
	int serr = fatal & F_ERROR;
	int serrno = errno;
	fatal &= ~F_ERROR;

	if (!fatal && preen)
		(void)printf("%s: ", dev);
	if (quiet && !preen) {
		(void)printf("** %s (vmsg)\n", dev);
		quiet = 0;
	}

	(void) vprintf(fmt, ap);
	if (serr) 
		printf(" (%s)", strerror(serrno));

	if (fatal && preen)
		(void) printf("\n");

	if (fatal && preen) {
		(void) printf(
		    "%s: UNEXPECTED INCONSISTENCY; RUN %s MANUALLY.\n",
		    dev, getprogname());
		exit(8);
	}
}

/*VARARGS*/
void
pfatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vmsg(1, fmt, ap);
	va_end(ap);
}

/*VARARGS*/
void
pwarn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vmsg(0, fmt, ap);
	va_end(ap);
}

void
perr(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vmsg(1 | F_ERROR, fmt, ap);
	va_end(ap);
}

void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vmsg(1, fmt, ap);
	va_end(ap);
	exit(8);
}

const char *
unrawname(const char *name)
{
	static char unrawbuf[MAXPATHLEN];
	const char *dp;
	struct stat stb;

	if ((dp = strrchr(name, '/')) == 0)
		return (name);
	if (stat(name, &stb) < 0)
		return (name);
	if (!S_ISCHR(stb.st_mode))
		return (name);
	if (dp[1] != 'r')
		return (name);
	(void)snprintf(unrawbuf, sizeof(unrawbuf), "%.*s/%s",
	    (int)(dp - name), name, dp + 2);
	return (unrawbuf);
}

const char *
rawname(const char *name)
{
	static char rawbuf[MAXPATHLEN];
	const char *dp;

	if ((dp = strrchr(name, '/')) == 0)
		return (0);
	(void)snprintf(rawbuf, sizeof(rawbuf), "%.*s/r%s",
	    (int)(dp - name), name, dp + 1);
	return (rawbuf);
}

const char *
blockcheck(const char *origname)
{
	struct stat stslash, stblock, stchar;
	const char *newname, *raw;
	struct fstab *fsp;
	int retried = 0;

	hot = 0;
	if (stat("/", &stslash) < 0) {
		perr("Can't stat `/'");
		return (origname);
	}
	newname = origname;
retry:
	if (stat(newname, &stblock) < 0) {
		perr("Can't stat `%s'", newname);
		return (origname);
	}
	if (S_ISBLK(stblock.st_mode)) {
		if (stslash.st_dev == stblock.st_rdev)
			hot++;
		raw = rawname(newname);
		if (stat(raw, &stchar) < 0) {
			perr("Can't stat `%s'", raw);
			return (origname);
		}
		if (S_ISCHR(stchar.st_mode)) {
			return (raw);
		} else {
			printf("%s is not a character device\n", raw);
			return (origname);
		}
	} else if (S_ISCHR(stblock.st_mode) && !retried) {
		newname = unrawname(newname);
		retried++;
		goto retry;
	} else if ((fsp = getfsfile(newname)) != 0 && !retried) {
		newname = fsp->fs_spec;
		retried++;
		goto retry;
	}
	/*
	 * Not a block or character device, just return name and
	 * let the user decide whether to use it.
	 */
	return (origname);
}


void *
emalloc(size_t s)
{
	void *p;

	p = malloc(s);
	if (p == NULL)
		err(1, "malloc failed");
	return (p);
}


void *
erealloc(void *p, size_t s)
{
	void *q;

	q = realloc(p, s);
	if (q == NULL)
		err(1, "realloc failed");
	return (q);
}


char *
estrdup(const char *s)
{
	char *p;

	p = strdup(s);
	if (p == NULL)
		err(1, "strdup failed");
	return (p);
}
