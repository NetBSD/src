/*	$NetBSD: evutil.h,v 1.1.8.1 2009/08/04 18:32:09 snj Exp $	*/
/*
 * Copyright (c) 2007 Niels Provos <provos@citi.umich.edu>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVUTIL_H_
#define _EVUTIL_H_

/** @file evutil.h

  Common convenience functions for cross-platform portability and
  related socket manipulations.

 */

#ifdef __cplusplus
extern "C" {
#endif
#include <stdarg.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>

#define ev_uint64_t uint64_t
#define ev_int64_t int64_t
#define ev_uint32_t uint32_t
#define ev_uint16_t uint16_t
#define ev_uint8_t uint8_t

int evutil_socketpair(int d, int type, int protocol, int sv[2]);
int evutil_make_socket_nonblocking(int sock);
#define EVUTIL_CLOSESOCKET(s) close(s)

#define EVUTIL_SOCKET_ERROR() (errno)
#define EVUTIL_SET_SOCKET_ERROR(errcode)		\
		do { errno = (errcode); } while (0)

/*
 * Manipulation functions for struct timeval
 */
#define evutil_timeradd(tvp, uvp, vvp) timeradd((tvp), (uvp), (vvp))
#define evutil_timersub(tvp, uvp, vvp) timersub((tvp), (uvp), (vvp))

#define evutil_timerclear(tvp) timerclear(tvp)

#define	evutil_timercmp(tvp, uvp, cmp)							\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?							\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :						\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define evutil_timerisset(tvp) timerisset(tvp)


/* big-int related functions */
ev_int64_t evutil_strtoll(const char *s, char **endptr, int base);


#define evutil_gettimeofday(tv, tz) gettimeofday((tv), (tz))

int evutil_snprintf(char *buf, size_t buflen, const char *format, ...)
#ifdef __GNUC__
	__attribute__((format(printf, 3, 4)))
#endif
	;
int evutil_vsnprintf(char *buf, size_t buflen, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* _EVUTIL_H_ */
