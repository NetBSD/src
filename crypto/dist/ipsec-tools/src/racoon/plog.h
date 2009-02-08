/*	$NetBSD: plog.h,v 1.5.12.1 2009/02/08 18:42:18 snj Exp $	*/

/* Id: plog.h,v 1.7 2006/06/20 09:57:31 vanhu Exp */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PLOG_H
#define _PLOG_H

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <syslog.h>

/*
 * INFO: begin negotiation, SA establishment/deletion/expiration.
 * NOTIFY: just notifiable.
 * WARNING: not error strictly.
 * ERROR: system call error. also invalid parameter/format.
 * DEBUG1: debugging informatioin.
 * DEBUG2: too more verbose. e.g. parsing config.
 */
#define LLV_ERROR	1
#define LLV_WARNING	2
#define LLV_NOTIFY	3
#define LLV_INFO	4
#define LLV_DEBUG	5
#define LLV_DEBUG2	6
 
#define LLV_BASE	LLV_INFO /* by default log less than this value. */

extern char *pname;
extern u_int32_t loglevel;
extern int f_foreground;
extern int print_location;

struct sockaddr;
#define plog(pri, ...) \
	do { \
		if ((pri) <= loglevel) \
			_plog((pri), __VA_ARGS__); \
	} while (0)
extern void _plog __P((int, const char *, struct sockaddr *, const char *, ...))
	__attribute__ ((__format__ (__printf__, 4, 5)));
extern void plogv __P((int, const char *, struct sockaddr *,
	const char *, va_list));
extern void plogdump __P((int, void *, size_t));
extern void ploginit __P((void));
extern void plogset __P((char *));

extern char* binsanitize __P((char*, size_t));

#endif /* _PLOG_H */
