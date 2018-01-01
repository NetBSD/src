/*
 * logerr: errx with logging
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "logerr.h"

#ifndef	LOGERR_SYSLOG_FACILITY
#define	LOGERR_SYSLOG_FACILITY	LOG_DAEMON
#endif

#ifdef SMALL
#undef LOGERR_TAG
#endif

#define UNUSED(a)		(void)(a)

struct logctx {
	unsigned int	 log_opts;
#ifndef SMALL
	FILE		*log_file;
#ifdef LOGERR_TAG
	const char	*log_tag;
#endif
#endif
};

static struct logctx _logctx = {
	/* syslog style, but without the hostname or tag. */
	.log_opts = LOGERR_LOG | LOGERR_LOG_DATE | LOGERR_LOG_PID,
};

#if defined(LOGERR_TAG) && defined(__linux__)
/* Poor man's getprogname(3). */
static char *_logprog;
static const char *
getprogname(void)
{
	const char *p;

	/* Use PATH_MAX + 1 to avoid truncation. */
	if (_logprog == NULL) {
		/* readlink(2) does not append a NULL byte,
		 * so zero the buffer. */
		if ((_logprog = calloc(1, PATH_MAX + 1)) == NULL)
			return NULL;
	}
	if (readlink("/proc/self/exe", _logprog, PATH_MAX + 1) == -1)
		return NULL;
	if (_logprog[0] == '[')
		return NULL;
	p = strrchr(_logprog, '/');
	if (p == NULL)
		return _logprog;
	return p + 1;
}
#endif

#ifndef SMALL
/* Write the time, syslog style. month day time - */
static void
logprintdate(FILE *stream)
{
	struct timeval tv;
	time_t now;
	struct tm tmnow;
	char buf[32];

	if (gettimeofday(&tv, NULL) == -1)
		return;

	now = tv.tv_sec;
	tzset();
	localtime_r(&now, &tmnow);
	strftime(buf, sizeof(buf), "%b %d %T ", &tmnow);
	fprintf(stream, "%s", buf);
}
#endif

__printflike(3, 0) static void
vlogprintf_r(struct logctx *ctx, FILE *stream, const char *fmt, va_list args)
{
	va_list a;
#ifndef SMALL
	bool log_pid;
#ifdef LOGERR_TAG
	bool log_tag;
#endif

	if ((stream == stderr && ctx->log_opts & LOGERR_ERR_DATE) ||
	    (stream != stderr && ctx->log_opts & LOGERR_LOG_DATE))
		logprintdate(stream);

#ifdef LOGERR_TAG
	log_tag = ((stream == stderr && ctx->log_opts & LOGERR_ERR_TAG) ||
	    (stream != stderr && ctx->log_opts & LOGERR_LOG_TAG));
	if (log_tag) {
		if (ctx->log_tag == NULL)
			ctx->log_tag = getprogname();
		fprintf(stream, "%s", ctx->log_tag);
	}
#endif

	log_pid = ((stream == stderr && ctx->log_opts & LOGERR_ERR_PID) ||
	    (stream != stderr && ctx->log_opts & LOGERR_LOG_PID));
	if (log_pid)
		fprintf(stream, "[%d]", getpid());

#ifdef LOGERR_TAG
	if (log_tag || log_pid)
#else
	if (log_pid)
#endif
		fprintf(stream, ": ");
#else
	UNUSED(ctx);
#endif

	va_copy(a, args);
	vfprintf(stream, fmt, a);
	fputc('\n', stream);
	va_end(a);
}

/*
 * NetBSD's gcc has been modified to check for the non standard %m in printf
 * like functions and warn noisily about it that they should be marked as
 * syslog like instead.
 * This is all well and good, but our logger also goes via vfprintf and
 * when marked as a sysloglike funcion, gcc will then warn us that the
 * function should be printflike instead!
 * This creates an infinte loop of gcc warnings.
 * Until NetBSD solves this issue, we have to disable a gcc diagnostic
 * for our fully standards compliant code in the logger function.
 */
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-format-attribute"
#endif
__printflike(2, 0) static void
vlogmessage(int pri, const char *fmt, va_list args)
{
	struct logctx *ctx = &_logctx;

	if (ctx->log_opts & LOGERR_ERR &&
	    (pri <= LOG_ERR ||
	    (!(ctx->log_opts & LOGERR_QUIET) && pri <= LOG_INFO) ||
	    (ctx->log_opts & LOGERR_DEBUG && pri <= LOG_DEBUG)))
		vlogprintf_r(ctx, stderr, fmt, args);

	if (!(ctx->log_opts & LOGERR_LOG))
		return;

#ifdef SMALL
	vsyslog(pri, fmt, args);
#else
	if (ctx->log_file == NULL) {
		vsyslog(pri, fmt, args);
		return;
	}
	if (pri == LOG_DEBUG && !(ctx->log_opts & LOGERR_DEBUG))
		return;
	vlogprintf_r(ctx, ctx->log_file, fmt, args);
#endif
}
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5))
#pragma GCC diagnostic pop
#endif

__printflike(2, 3) static void
logmessage(int pri, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogmessage(pri, fmt, args);
	va_end(args);
}

__printflike(2, 0) static void
vlogerrmessage(int pri, const char *fmt, va_list args)
{
	int _errno = errno;
	char buf[1024];

	vsnprintf(buf, sizeof(buf), fmt, args);
	logmessage(pri, "%s: %s", buf, strerror(_errno));
}

void
logdebug(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogerrmessage(LOG_DEBUG, fmt, args);
	va_end(args);
}

void
logdebugx(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogmessage(LOG_DEBUG, fmt, args);
	va_end(args);
}

void
loginfo(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogerrmessage(LOG_INFO, fmt, args);
	va_end(args);
}

void
loginfox(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogmessage(LOG_INFO, fmt, args);
	va_end(args);
}

void
logwarn(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogerrmessage(LOG_WARNING, fmt, args);
	va_end(args);
}

void
logwarnx(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogmessage(LOG_WARNING, fmt, args);
	va_end(args);
}

void
logerr(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogerrmessage(LOG_ERR, fmt, args);
	va_end(args);
}

void
logerrx(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogmessage(LOG_ERR, fmt, args);
	va_end(args);
}

void
logsetopts(unsigned int opts)
{
	struct logctx *ctx = &_logctx;

	ctx->log_opts = opts;
	setlogmask(LOG_UPTO(opts & LOGERR_DEBUG ? LOG_DEBUG : LOG_INFO));
}

#ifdef LOGERR_TAG
void
logsettag(const char *tag)
{
#if !defined(SMALL)
	struct logctx *ctx = &_logctx;

	ctx->log_tag = tag;
#else
	UNUSED(tag);
#endif
}
#endif

int
logopen(const char *path)
{
	struct logctx *ctx = &_logctx;

	if (path == NULL) {
		int opts = 0;

		if (ctx->log_opts & LOGERR_LOG_PID)
			opts |= LOG_PID;
		openlog(NULL, opts, LOGERR_SYSLOG_FACILITY);
		return 1;
	}

#ifndef SMALL
	if ((ctx->log_file = fopen(path, "a")) == NULL)
		return -1;
	setlinebuf(ctx->log_file);
	return fileno(ctx->log_file);
#else
	errno = ENOTSUP;
	return -1;
#endif
}

void
logclose(void)
{
#ifndef SMALL
	struct logctx *ctx = &_logctx;
#endif

	closelog();
#ifndef SMALL
	if (ctx->log_file == NULL)
		return;
	fclose(ctx->log_file);
	ctx->log_file = NULL;
#endif
#if defined(LOGERR_TAG) && defined(__linux__)
	free(_logprog);
#endif
}
