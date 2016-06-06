/*	$NetBSD: opendisk.c,v 1.14 2016/06/06 17:50:19 christos Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: opendisk.c,v 1.14 2016/06/06 17:50:19 christos Exp $");
#endif

#include <sys/param.h>

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#ifndef HAVE_NBTOOL_CONFIG_H
#include <util.h>
#include <paths.h>
#else
#include "opendisk.h"
#endif
#include <stdio.h>
#include <string.h>

static int __printflike(5, 6)
opd(char *buf, size_t len, int (*ofn)(const char *, int, ...),
    int flags, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, len, fmt, ap);
	va_end(ap);

	return (*ofn)(buf, flags, 0);
}

static int
__opendisk(const char *path, int flags, char *buf, size_t buflen, int iscooked,
	int (*ofn)(const char *, int, ...))
{
	int f, part;

	if (buf == NULL) {
		errno = EFAULT;
		return -1;
	}

	if ((flags & O_CREAT) != 0) {
		errno = EINVAL;
		return -1;
	}

	part = getrawpartition();
	if (part < 0)
		return -1;	/* sysctl(3) in getrawpartition sets errno */
	part += 'a';

	/*
	 * If we are passed a plain name, first try /dev to avoid accidents
	 * with files in the same directory that happen to match disk names.
	 */
	if (strchr(path, '/') == NULL) {
		const char *r = iscooked ? "" : "r";
		const char *d = _PATH_DEV;

		f = opd(buf, buflen, ofn, flags, "%s%s%s", d, r, path);
		if (f != -1 || errno != ENOENT)
			return f;

		f = opd(buf, buflen, ofn, flags, "%s%s%s%c", d, r, path, part);
		if (f != -1 || errno != ENOENT)
			return f;
	}

	f = opd(buf, buflen, ofn, flags, "%s", path);
	if (f != -1 || errno != ENOENT)
		return f;

	return opd(buf, buflen, ofn, flags, "%s%c", path, part);
}

int
opendisk(const char *path, int flags, char *buf, size_t buflen, int iscooked)
{

	return __opendisk(path, flags, buf, buflen, iscooked, open);
}

int
opendisk1(const char *path, int flags, char *buf, size_t buflen, int iscooked,
	int (*ofn)(const char *, int, ...))
{

	return __opendisk(path, flags, buf, buflen, iscooked, ofn);
}
