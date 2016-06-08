/*	$NetBSD: port.h,v 1.1 2016/06/08 01:13:14 christos Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif

#ifndef __RCSID
#define __RCSID(a)
#endif

#ifndef __USE
#define __USE(a) ((void)&(a))
#endif

#ifndef __printflike
#define __printflike(f, a)	__attribute__((__format__(__printf__, f, a)))
#endif

#ifdef __NetBSD__
# include <util.h>
#else
# define pidfile(a)
#endif

#ifdef __linux__
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

extern char *__progname;
static inline const char *
getprogname(void) {
	return __progname;
}

static inline char *
strlcpy(char *d, const char *s, size_t l)
{
	if (l == 0)
		return NULL;
	strncpy(d, s, l);
	d[l - 1] = '\0';
	return d;
}
#endif
