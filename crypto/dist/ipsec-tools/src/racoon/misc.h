/*	$NetBSD: misc.h,v 1.6 2008/07/15 00:47:09 mgrooms Exp $	*/

/* Id: misc.h,v 1.9 2006/04/06 14:00:06 manubsd Exp */

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

#ifndef _MISC_H
#define _MISC_H

#define BIT2STR(b) bit2str(b, sizeof(b)<<3)

#ifdef HAVE_FUNC_MACRO
#define LOCATION        debug_location(__FILE__, __LINE__, __func__)
#else
#define LOCATION        debug_location(__FILE__, __LINE__, NULL)
#endif

extern int racoon_hexdump __P((void *, size_t));
extern char *bit2str __P((int, int));
extern void *get_newbuf __P((void *, size_t));
extern const char *debug_location __P((const char *, int, const char *));
extern int getfsize __P((char *));
struct timeval;
extern double timedelta __P((struct timeval *, struct timeval *));
char *strdup __P((const char *));
extern void close_on_exec __P((int fd));

#if defined(__APPLE__) && defined(__MACH__)
#define RACOON_TAILQ_FOREACH_REVERSE(var, head, headname ,field)	\
  TAILQ_FOREACH_REVERSE(var, head, field, headname)
#else
#define RACOON_TAILQ_FOREACH_REVERSE(var, head, headname ,field)	\
    TAILQ_FOREACH_REVERSE(var, head, headname, field)
#endif

#ifndef HAVE_STRLCPY
#define strlcpy(d,s,l) (strncpy(d,s,l), (d)[(l)-1] = '\0')
#endif

#ifndef HAVE_STRLCAT
#define strlcat(d,s,l) strncat(d,s,(l)-strlen(d)-1)
#endif

#define STRDUP_FATAL(x) if (x == NULL) {			\
	plog(LLV_ERROR, LOCATION, NULL, "strdup failed\n");	\
	exit(1);						\
}

#include "libpfkey.h"

#endif /* _MISC_H */
