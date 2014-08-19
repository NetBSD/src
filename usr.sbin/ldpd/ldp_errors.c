/* $NetBSD: ldp_errors.c,v 1.2.2.2 2014/08/20 00:05:09 tls Exp $ */

/*
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

#include <arpa/inet.h>
#include <netmpls/mpls.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "ldp.h"
#include "ldp_errors.h"

int	debug_f = 0, warn_f = 0, syslog_f = 0;

static void do_syslog(int, const char*, va_list) __printflike(2, 0);
static char satos_str[INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN :
		INET_ADDRSTRLEN];
static char *mpls_ntoa(const struct sockaddr_mpls *smpls);

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

const char *
satos(const struct sockaddr *sa)
{
	switch (sa->sa_family) {
		case AF_INET:
		{
			const struct sockaddr_in *sin =
			    (const struct sockaddr_in *)sa;
			if (inet_ntop(AF_INET, &(sin->sin_addr), satos_str,
			    sizeof(satos_str)) == NULL)
				return "INET ERROR";
			break;
		}
		case AF_INET6:
		{
			const struct sockaddr_in6 *sin6 =
			    (const struct sockaddr_in6 *)sa;
			if (inet_ntop(AF_INET6, &(sin6->sin6_addr), satos_str,
			    sizeof(satos_str)) == NULL)
				return "INET6 ERROR";
			break;
		}
		case AF_LINK:
		{
			strlcpy(satos_str,
			    link_ntoa((const struct sockaddr_dl *)sa),
			    sizeof(satos_str));
			break;
		}
		case AF_MPLS:
		{
			strlcpy(satos_str,
			    mpls_ntoa((const struct sockaddr_mpls *)sa),
			    sizeof(satos_str));
			break;
		}
		default:
			return "UNKNOWN AF";
	}
	return satos_str;
}

static char *
mpls_ntoa(const struct sockaddr_mpls *smpls)
{
	static char ret[10];
	union mpls_shim ms2;

	ms2.s_addr = ntohl(smpls->smpls_addr.s_addr);
	snprintf(ret, sizeof(ret), "%d", ms2.shim.label);
	return ret;
}
