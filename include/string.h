/*	$NetBSD: string.h,v 1.57 2024/11/02 02:43:48 riastradh Exp $	*/

/*-
 * Copyright (c) 1990, 1993
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
 *
 *	@(#)string.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _STRING_H_
#define	_STRING_H_
#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#include <sys/null.h>

#include <sys/cdefs.h>
#include <sys/featuretest.h>

#if (_POSIX_C_SOURCE - 0) >= 200809L || defined(_NETBSD_SOURCE)
#  ifndef __LOCALE_T_DECLARED
typedef struct _locale		*locale_t;
#  define __LOCALE_T_DECLARED
#  endif
#endif /* _POSIX_C_SOURCE || _NETBSD_SOURCE */

__BEGIN_DECLS
#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
void	*memccpy(void *, const void *, int, size_t);
#endif /* _XOPEN_SOURCE || _NETBSD_SOURCE */
void	*memchr(const void *, int, size_t);
int	 memcmp(const void *, const void *, size_t);
void	*memcpy(void * __restrict, const void * __restrict, size_t);
#if (_POSIX_C_SOURCE - 0 >= 202405L) || defined(_NETBSD_SOURCE)
void	*memmem(const void *, size_t, const void *, size_t);
#endif /* _POSIX_C_SOURCE || _NETBSD_SOURCE */
void	*memmove(void *, const void *, size_t);
void	*memset(void *, int, size_t);
#if (__STDC_VERSION__ - 0 >= 202311L) || defined(_ISOC23_SOURCE) || \
    defined(_NETBSD_SOURCE)
void	*memset_explicit(void *, int, size_t);
#endif
#if (_POSIX_C_SOURCE - 0 >= 200809L) || defined(_NETBSD_SOURCE)
char	*stpcpy(char * __restrict, const char * __restrict);
char	*stpncpy(char * __restrict, const char * __restrict, size_t);
#endif /* _POSIX_C_SOURCE || _NETBSD_SOURCE */
char	*strcat(char * __restrict, const char * __restrict);
char	*strchr(const char *, int);
int	 strcmp(const char *, const char *);
int	 strcoll(const char *, const char *);
#if (_POSIX_C_SOURCE - 0) >= 200809L || defined(_NETBSD_SOURCE)
int	 strcoll_l(const char *, const char *, locale_t);
#endif /* _POSIX_C_SOURCE || _NETBSD_SOURCE */
char	*strcpy(char * __restrict, const char * __restrict);
size_t	 strcspn(const char *, const char *);
#if (_POSIX_C_SOURCE - 0 >= 200809L) || defined(_XOPEN_SOURCE) || \
    defined(_NETBSD_SOURCE)
char	*strdup(const char *);
#endif /* _POSIX_C_SOURCE || _XOPEN_SOURCE || _NETBSD_SOURCE */
__aconst char *strerror(int);
#if (_POSIX_C_SOURCE - 0) >= 200809L || defined(_NETBSD_SOURCE)
__aconst char *strerror_l(int, locale_t);
#endif /* _POSIX_C_SOURCE || _NETBSD_SOURCE */
#if (_POSIX_C_SOURCE - 0 >= 200112L) || \
    defined(_REENTRANT) || defined(_NETBSD_SOURCE)
int	 strerror_r(int, char *, size_t);
#endif /* _POSIX_C_SOURCE || _REENTRANT || _NETBSD_SOURCE */
#if (_POSIX_C_SOURCE - 0 >= 202405L) || defined(_NETBSD_SOURCE)
size_t	 strlcat(char * __restrict, const char * __restrict, size_t);
size_t	 strlcpy(char * __restrict, const char * __restrict, size_t);
#endif /* _POSIX_C_SOURCE || _NETBSD_SOURCE */
size_t	 strlen(const char *);
char	*strncat(char * __restrict, const char * __restrict, size_t);
int	 strncmp(const char *, const char *, size_t);
char	*strncpy(char * __restrict, const char * __restrict, size_t);
#if (_POSIX_C_SOURCE - 0 >= 200809L) || defined(_NETBSD_SOURCE)
char	*strndup(const char *, size_t);
size_t	strnlen(const char *, size_t);
#endif /* _POSIX_C_SOURCE || _NETBSD_SOURCE */
char	*strpbrk(const char *, const char *);
char	*strrchr(const char *, int);
#if (_POSIX_C_SOURCE - 0 >= 200809L) || defined(_NETBSD_SOURCE)
#ifndef __STRSIGNAL_DECLARED
#define __STRSIGNAL_DECLARED
/* also in unistd.h */
__aconst char *strsignal(int);
#endif /* __STRSIGNAL_DECLARED */
#endif
size_t	 strspn(const char *, const char *);
char	*strstr(const char *, const char *);
char	*strtok(char * __restrict, const char * __restrict);
#if (_POSIX_C_SOURCE - 0 >= 200112L) || \
    defined(_REENTRANT) || defined(_NETBSD_SOURCE)
char	*strtok_r(char *, const char *, char **);
#endif /* _POSIX_C_SOURCE || _REENTRANT || _NETBSD_SOURCE */
size_t	 strxfrm(char * __restrict, const char * __restrict, size_t);
#if (_POSIX_C_SOURCE - 0) >= 200809L || defined(_NETBSD_SOURCE)
size_t	 strxfrm_l(char * __restrict, const char * __restrict, size_t,
	    locale_t);
#endif /* _POSIX_C_SOURCE || _NETBSD_SOURCE */
__END_DECLS

#if defined(_NETBSD_SOURCE)
#include <strings.h>		/* for backwards-compatibility */
__BEGIN_DECLS
char	*strcasestr(const char *, const char *);
char	*strchrnul(const char *, int);
char	*strsep(char **, const char *);
char	*stresep(char **, const char *, int);
char	*strnstr(const char *, const char *, size_t);
void	*memrchr(const void *, int, size_t);
void	*mempcpy(void * __restrict, const void * __restrict, size_t);
void	*explicit_memset(void *, int, size_t);
int	consttime_memequal(const void *, const void *, size_t);
__END_DECLS
#endif /* _NETBSD_SOURCE */

#if _FORTIFY_SOURCE > 0
#include <ssp/string.h>
#endif
#endif /* !defined(_STRING_H_) */
