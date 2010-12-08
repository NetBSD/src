/* $NetBSD: ldp_errors.c,v 1.1 2010/12/08 07:20:14 kefren Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <unistd.h>

#include "ldp.h"
#include "ldp_errors.h"

int	debug_f = 0, warn_f = 0, syslog_f = 0;

static void do_syslog(int, const char*, va_list);

void 
debugp(const char *fmt, ...)
{
	va_list va;

	if (!debug_f)
		return;

	va_start(va, fmt);
	if (syslog_f)
		do_syslog(LOG_DEBUG, fmt, va);
	else
		vprintf(fmt, va);
	va_end(va);
}

void
warnp(const char *fmt, ...)
{
	va_list va;

	if (!debug_f && !warn_f)
		return;

	va_start(va, fmt);
	if (syslog_f)
		do_syslog(LOG_WARNING, fmt, va);
	else
		vprintf(fmt, va);
	va_end(va);
}

void
fatalp(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	if (syslog_f)
		do_syslog(LOG_ERR, fmt, va);
	else
		vprintf(fmt, va);
	va_end(va);
}

static void
do_syslog(int prio, const char *fmt, va_list va)
{
	vsyslog(prio, fmt, va);
}

void 
printtime()
{
	time_t          t;
	char            buf[26];
	int             i;

	time(&t);
	ctime_r(&t, buf);
	for (i = 0; (i < 26 && buf[i] != 0); i++)
		if (buf[i] == '\n')
			buf[i] = 0;
	printf("%s ", buf);
}
