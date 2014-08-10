/*	$NetBSD: log.c,v 1.2.38.1 2014/08/10 06:59:53 tls Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
#ifndef lint
__RCSID("$NetBSD: log.c,v 1.2.38.1 2014/08/10 06:59:53 tls Exp $");
#endif

#include <err.h>
#include <stdarg.h>
#include "os.h"
#include "mopdef.h"
#include "log.h"

int mopInteractive = 0;

void
mopLogErr(const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	int error;

	va_start(ap, fmt);
	if (mopInteractive)
		verr(1, fmt, ap);
	else {
		error = errno;
		vsnprintf(buf, sizeof(buf), fmt, ap);
		syslog(LOG_ERR, "%s: %s", buf, strerror(error));
	}
	va_end(ap);
	exit(1);
}

void
mopLogWarn(const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	int error;

	va_start(ap, fmt);
	if (mopInteractive)
		vwarn(fmt, ap);
	else {
		error = errno;
		vsnprintf(buf, sizeof(buf), fmt, ap);
		syslog(LOG_WARNING, "%s: %s", buf, strerror(error));
	}
	va_end(ap);
}

void
mopLogErrX(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (mopInteractive)
		verrx(1, fmt, ap);
	else
		vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

void
mopLogWarnX(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (mopInteractive)
		vwarnx(fmt, ap);
	else
		vsyslog(LOG_WARNING, fmt, ap);
	va_end(ap);
}
