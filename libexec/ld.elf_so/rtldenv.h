/*	$NetBSD: rtldenv.h,v 1.2 1997/10/08 08:55:38 mrg Exp $	*/

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef _RTLDENV_H
#define	_RTLDENV_H

#include <stddef.h>
#include <stdarg.h>

extern void *xcalloc(size_t);
extern void *xmalloc(size_t);
extern char *xstrdup(const char *);

#ifdef RTLD_LOADER
extern void xprintf(const char *fmt, ...);
extern void xvprintf(const char *fmt, va_list ap);
extern void xsnprintf(char *buf, size_t buflen, const char *fmt, ...);
extern size_t xvsnprintf(char *buf, size_t buflen, const char *fmt, va_list ap);
extern void xwarn(const char *fmt, ...);
extern void xwarnx(const char *fmt, ...);
extern void xerr(int eval, const char *fmt, ...);
extern void xerrx(int eval, const char *fmt, ...);
extern void xassert(const char *file, int line, const char *failedexpr);
extern const char *xstrerror(int error);

#define assert(cond)	((cond) \
			 ? (void) 0 :\
			 (xassert(__FILE__, __LINE__, #cond "\n"), abort()))
#else
#include <assert.h>
#include <stdio.h>
#include <err.h>

#define	xprintf		printf
#define	xvprintf	vprintf
#define	xsnprintf	snprintf
#define	xvsnprintf	vsnprintf
#define	xwarn		warn
#define	xwarnx		warnx
#define	xerr		err
#define	xerrx		errx
#define	xassert		assert
#define	xstrerror	strerror
#endif

#endif /* _RTLDENV_H */
