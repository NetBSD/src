/*	$NetBSD: syslog.c,v 1.58 2017/01/12 18:16:52 christos Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)syslog.c	8.5 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: syslog.c,v 1.58 2017/01/12 18:16:52 christos Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netdb.h>

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <paths.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "syslog_private.h"
#include "reentrant.h"
#include "extern.h"

#ifdef __weak_alias
__weak_alias(syslog,_syslog)
__weak_alias(vsyslog,_vsyslog)
__weak_alias(syslogp,_syslogp)
__weak_alias(vsyslogp,_vsyslogp)
__weak_alias(closelog,_closelog)
__weak_alias(openlog,_openlog)
__weak_alias(setlogmask,_setlogmask)
#endif

static struct syslog_data _syslog_data = SYSLOG_DATA_INIT;

#ifdef _REENTRANT
static mutex_t	syslog_mutex = MUTEX_INITIALIZER;
#endif

static size_t
timefun(char *p, size_t tbuf_left)
{
	struct timeval tv;
	time_t now;
	struct tm tmnow;
	size_t prlen;
	char *op = p;

	if (gettimeofday(&tv, NULL) == -1)
		return snprintf_ss(p, tbuf_left, "-");
	
	/* strftime() implies tzset(), localtime_r() doesn't. */
	tzset();
	now = (time_t) tv.tv_sec;
	localtime_r(&now, &tmnow);

	prlen = strftime(p, tbuf_left, "%FT%T", &tmnow);
	DEC();
	prlen = snprintf(p, tbuf_left, ".%06ld", (long)tv.tv_usec);
	DEC();
	prlen = strftime(p, tbuf_left-1, "%z", &tmnow);
	/* strftime gives eg. "+0200", but we need "+02:00" */
	if (prlen == 5) {
		p[prlen+1] = p[prlen];
		p[prlen]   = p[prlen-1];
		p[prlen-1] = p[prlen-2];
		p[prlen-2] = ':';
		prlen += 1;
	}
	DEC();
	return (size_t)(p - op);
}

static int
lock(const struct syslog_data *data)
{
	int rv = data == &_syslog_data;
	if (rv)
		mutex_lock(&syslog_mutex);
	return rv;
}

static int
unlock(const struct syslog_data *data)
{
	int rv = data == &_syslog_data;
	if (rv)
		mutex_unlock(&syslog_mutex);
	return rv;
}

static struct syslog_fun _syslog_fun = {
	timefun,
	strerror_r,
	vsnprintf,
	lock,
	unlock,
};

void
openlog(const char *ident, int logstat, int logfac)
{
	openlog_r(ident, logstat, logfac, &_syslog_data);
}

void
closelog(void)
{
	closelog_r(&_syslog_data);
}

/* setlogmask -- set the log mask level */
int
setlogmask(int pmask)
{
	return setlogmask_r(pmask, &_syslog_data);
}

void
openlog_r(const char *ident, int logstat, int logfac, struct syslog_data *data)
{
	lock(data);
	_openlog_unlocked_r(ident, logstat, logfac, data);
	unlock(data);
}

void
closelog_r(struct syslog_data *data)
{
	lock(data);
	_closelog_unlocked_r(data);
	data->log_tag = NULL;
	unlock(data);
}

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */
void
syslog(int pri, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vxsyslogp_r(pri, &_syslog_fun, &_syslog_data, NULL, NULL, fmt, ap);
	va_end(ap);
}

void
vsyslog(int pri, const char *fmt, va_list ap)
{
	_vxsyslogp_r(pri, &_syslog_fun, &_syslog_data, NULL, NULL, fmt, ap);
}

/*
 * syslogp, vsyslogp --
 *	like syslog but take additional arguments for MSGID and SD
 */
void
syslogp(int pri, const char *msgid, const char *sdfmt, const char *msgfmt, ...)
{
	va_list ap;

	va_start(ap, msgfmt);
	_vxsyslogp_r(pri, &_syslog_fun, &_syslog_data,
	    msgid, sdfmt, msgfmt, ap);
	va_end(ap);
}

void
vsyslogp(int pri, const char *msgid, const char *sdfmt, const char *msgfmt,
    va_list ap)
{
	_vxsyslogp_r(pri, &_syslog_fun, &_syslog_data,
	    msgid, sdfmt, msgfmt, ap);
}

void
vsyslogp_r(int pri, struct syslog_data *data, const char *msgid,
    const char *sdfmt, const char *msgfmt, va_list ap)
{
	_vxsyslogp_r(pri, &_syslog_fun, data, msgid, sdfmt, msgfmt, ap);
}

void
syslog_r(int pri, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vxsyslogp_r(pri, &_syslog_fun, data, NULL, NULL, fmt, ap);
	va_end(ap);
}

void
syslogp_r(int pri, struct syslog_data *data, const char *msgid,
	const char *sdfmt, const char *msgfmt, ...)
{
	va_list ap;

	va_start(ap, msgfmt);
	_vxsyslogp_r(pri, &_syslog_fun, data, msgid, sdfmt, msgfmt, ap);
	va_end(ap);
}

void
vsyslog_r(int pri, struct syslog_data *data, const char *fmt, va_list ap)
{
	_vxsyslogp_r(pri, &_syslog_fun, data, NULL, NULL, fmt, ap);
}
