/*	$NetBSD: syslog_ss.c,v 1.3.26.1 2024/10/08 11:16:17 martin Exp $	*/

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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: syslog_ss.c,v 1.3.26.1 2024/10/08 11:16:17 martin Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <stdio.h>
#include <sys/syslog.h>
#include "extern.h"
#include "syslog_private.h"


static size_t
timefun_ss(char *p, size_t tbuf_left)
{
	return snprintf_ss(p, tbuf_left, "-");
#if 0
	/*
	 * if gmtime_r() was signal-safe we could output
	 * the UTC-time:
	 */
	gmtime_r(&now, &tmnow);
	prlen = strftime(p, tbuf_left, "%FT%TZ", &tmnow);
	return prlen;
#endif
}

static int
lock_ss(const struct syslog_data *data __unused)
{
	return 0;
}

static int
unlock_ss(const struct syslog_data *data __unused)
{
	return 0;
}

struct syslog_fun _syslog_ss_fun = {
	timefun_ss,
	strerror_r_ss,
	vsnprintf_ss,
	lock_ss,
	unlock_ss,
};

void
syslog_ss(int pri, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vxsyslogp_r(pri, &_syslog_ss_fun, data, NULL, NULL, fmt, ap);
	va_end(ap);
}

void
syslogp_ss(int pri, struct syslog_data *data, const char *msgid,
	const char *sdfmt, const char *msgfmt, ...)
{
	va_list ap;

	va_start(ap, msgfmt);
	_vxsyslogp_r(pri, &_syslog_ss_fun, data, msgid, sdfmt, msgfmt, ap);
	va_end(ap);
}

void
vsyslog_ss(int pri, struct syslog_data *data, const char *fmt, va_list ap)
{
	_vxsyslogp_r(pri, &_syslog_ss_fun, data, NULL, NULL, fmt, ap);
}

void
vsyslogp_ss(int pri, struct syslog_data *data, const char *msgid,
	const char *sdfmt, const char *msgfmt, va_list ap)
{
	_vxsyslogp_r(pri, &_syslog_ss_fun, data, msgid, sdfmt, msgfmt, ap);
}
