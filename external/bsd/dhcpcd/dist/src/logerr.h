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

#ifndef LOGERR_H
#define LOGERR_H

#include <sys/param.h>

#ifndef __printflike
#if __GNUC__ > 2 || defined(__INTEL_COMPILER)
#define	__printflike(a, b) __attribute__((format(printf, a, b)))
#else
#define	__printflike
#endif
#endif /* !__printflike */

__printflike(1, 2) typedef void logfunc_t(const char *, ...);

__printflike(1, 2) void logdebug(const char *, ...);
__printflike(1, 2) void logdebugx(const char *, ...);
__printflike(1, 2) void loginfo(const char *, ...);
__printflike(1, 2) void loginfox(const char *, ...);
__printflike(1, 2) void logwarn(const char *, ...);
__printflike(1, 2) void logwarnx(const char *, ...);
__printflike(1, 2) void logerr(const char *, ...);
#define	LOGERROR	logerr("%s: %d", __FILE__, __LINE__)
__printflike(1, 2) void logerrx(const char *, ...);

void logsetopts(unsigned int);
#define	LOGERR_DEBUG	(1U << 6)
#define	LOGERR_QUIET	(1U << 7)
#define	LOGERR_LOG	(1U << 11)
#define	LOGERR_LOG_DATE	(1U << 12)
#define	LOGERR_LOG_HOST	(1U << 13)
#define	LOGERR_LOG_TAG	(1U << 14)
#define	LOGERR_LOG_PID	(1U << 15)
#define	LOGERR_ERR	(1U << 21)
#define	LOGERR_ERR_DATE	(1U << 22)
#define	LOGERR_ERR_HOST	(1U << 23)
#define	LOGERR_ERR_TAG	(1U << 24)
#define	LOGERR_ERR_PID	(1U << 25)

/* To build tag support or not. */
//#define	LOGERR_TAG
#if defined(LOGERR_TAG)
void logsettag(const char *);
#endif

int logopen(const char *);
void logclose(void);
int logreopen(void);

#endif
