/*	$KAME: plog.h,v 1.9 2002/04/26 00:00:10 itojun Exp $	*/

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

#define LC_DEFAULT_LOGF	"/var/log/racoon.log"

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
#define LLV_INFO	1
#define LLV_NOTIFY	2
#define LLV_WARNING	3
#define LLV_ERROR	4
#define LLV_DEBUG	5
#define LLV_DEBUG2	6
 
#define LLV_BASE	4	/* always logging less than this value. */

extern char *pname;
extern u_int32_t loglevel;
extern int f_foreground;

struct sockaddr;
extern void plog __P((int, const char *, struct sockaddr *, const char *, ...));
extern void plogv __P((int, const char *, struct sockaddr *,
	const char *, ...));
extern void plogdump __P((int, void *, size_t));
extern void ploginit __P((void));
extern void plogset __P((char *));
