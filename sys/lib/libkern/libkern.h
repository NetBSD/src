/*	$NetBSD: libkern.h,v 1.71.6.1 2007/12/26 19:57:17 ad Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
 *	@(#)libkern.h	8.2 (Berkeley) 8/5/94
 */

#ifndef _LIB_LIBKERN_LIBKERN_H_
#define _LIB_LIBKERN_LIBKERN_H_

#include <sys/types.h>
#include <sys/inttypes.h>
#include <sys/null.h>

#ifndef LIBKERN_INLINE
#define LIBKERN_INLINE	static __inline
#define LIBKERN_BODY
#endif

LIBKERN_INLINE int imax __P((int, int)) __attribute__ ((unused));
LIBKERN_INLINE int imin __P((int, int)) __attribute__ ((unused));
LIBKERN_INLINE u_int max __P((u_int, u_int)) __attribute__ ((unused));
LIBKERN_INLINE u_int min __P((u_int, u_int)) __attribute__ ((unused));
LIBKERN_INLINE long lmax __P((long, long)) __attribute__ ((unused));
LIBKERN_INLINE long lmin __P((long, long)) __attribute__ ((unused));
LIBKERN_INLINE u_long ulmax __P((u_long, u_long)) __attribute__ ((unused));
LIBKERN_INLINE u_long ulmin __P((u_long, u_long)) __attribute__ ((unused));
LIBKERN_INLINE int abs __P((int)) __attribute__ ((unused));

LIBKERN_INLINE int isspace __P((int)) __unused;
LIBKERN_INLINE int isascii __P((int)) __unused;
LIBKERN_INLINE int isupper __P((int)) __unused;
LIBKERN_INLINE int islower __P((int)) __unused;
LIBKERN_INLINE int isalpha __P((int)) __unused;
LIBKERN_INLINE int isdigit __P((int)) __unused;
LIBKERN_INLINE int isxdigit __P((int)) __unused;
LIBKERN_INLINE int toupper __P((int)) __unused;
LIBKERN_INLINE int tolower __P((int)) __unused;

#ifdef LIBKERN_BODY
LIBKERN_INLINE int
imax(int a, int b)
{
	return (a > b ? a : b);
}
LIBKERN_INLINE int
imin(int a, int b)
{
	return (a < b ? a : b);
}
LIBKERN_INLINE long
lmax(long a, long b)
{
	return (a > b ? a : b);
}
LIBKERN_INLINE long
lmin(long a, long b)
{
	return (a < b ? a : b);
}
LIBKERN_INLINE u_int
max(u_int a, u_int b)
{
	return (a > b ? a : b);
}
LIBKERN_INLINE u_int
min(u_int a, u_int b)
{
	return (a < b ? a : b);
}
LIBKERN_INLINE u_long
ulmax(u_long a, u_long b)
{
	return (a > b ? a : b);
}
LIBKERN_INLINE u_long
ulmin(u_long a, u_long b)
{
	return (a < b ? a : b);
}

LIBKERN_INLINE int
abs(int j)
{
	return(j < 0 ? -j : j);
}

LIBKERN_INLINE int
isspace(int ch)
{
	return (ch == ' ' || (ch >= '\t' && ch <= '\r'));
}

LIBKERN_INLINE int
isascii(int ch)
{
	return ((ch & ~0x7f) == 0);
}

LIBKERN_INLINE int
isupper(int ch)
{
	return (ch >= 'A' && ch <= 'Z');
}

LIBKERN_INLINE int
islower(int ch)
{
	return (ch >= 'a' && ch <= 'z');
}

LIBKERN_INLINE int
isalpha(int ch)
{
	return (isupper(ch) || islower(ch));
}

LIBKERN_INLINE int
isdigit(int ch)
{
	return (ch >= '0' && ch <= '9');
}

LIBKERN_INLINE int
isxdigit(int ch)
{
	return (isdigit(ch) ||
	    (ch >= 'A' && ch <= 'F') ||
	    (ch >= 'a' && ch <= 'f'));
}

LIBKERN_INLINE int
toupper(int ch)
{
	if (islower(ch))
		return (ch - 0x20);
	return (ch);
}

LIBKERN_INLINE int
tolower(int ch)
{
	if (isupper(ch))
		return (ch + 0x20);
	return (ch);
}
#endif

#define	__NULL_STMT		do { } while (/* CONSTCOND */ 0)

#ifdef NDEBUG						/* tradition! */
#define	assert(e)	((void)0)
#else
#ifdef __STDC__
#define	assert(e)	(__predict_true((e)) ? (void)0 :		    \
			    __kernassert("", __FILE__, __LINE__, #e))
#else
#define	assert(e)	(__predict_true((e)) ? (void)0 :		    \
			    __kernassert("", __FILE__, __LINE__, "e"))
#endif
#endif

#ifdef __COVERITY__
#ifndef DIAGNOSTIC
#define DIAGNOSTIC
#endif
#endif

#ifndef DIAGNOSTIC
#define _DIAGASSERT(a)	(void)0
#ifdef lint
#define	KASSERT(e)	/* NOTHING */
#else /* !lint */
#define	KASSERT(e)	((void)0)
#endif /* !lint */
#else /* DIAGNOSTIC */
#define _DIAGASSERT(a)	assert(a)
#ifdef __STDC__
#define	KASSERT(e)	(__predict_true((e)) ? (void)0 :		    \
			    __kernassert("diagnostic ", __FILE__, __LINE__, #e))
#else
#define	KASSERT(e)	(__predict_true((e)) ? (void)0 :		    \
			    __kernassert("diagnostic ", __FILE__, __LINE__,"e"))
#endif
#endif

#ifndef DEBUG
#ifdef lint
#define	KDASSERT(e)	/* NOTHING */
#else /* lint */
#define	KDASSERT(e)	((void)0)
#endif /* lint */
#else
#ifdef __STDC__
#define	KDASSERT(e)	(__predict_true((e)) ? (void)0 :		    \
			    __kernassert("debugging ", __FILE__, __LINE__, #e))
#else
#define	KDASSERT(e)	(__predict_true((e)) ? (void)0 :		    \
			    __kernassert("debugging ", __FILE__, __LINE__, "e"))
#endif
#endif
/*
 * XXX: For compatibility we use SMALL_RANDOM by default.
 */
#define SMALL_RANDOM

#ifndef offsetof
#define	offsetof(type, member) \
    ((size_t)(unsigned long)(&(((type *)0)->member)))
#endif

/* Prototypes for non-quad routines. */
/* XXX notyet #ifdef _STANDALONE */
int	 bcmp __P((const void *, const void *, size_t));
void	 bzero __P((void *, size_t));
/* #endif */

/* Prototypes for which GCC built-ins exist. */
void	*memcpy __P((void *, const void *, size_t));
int	 memcmp __P((const void *, const void *, size_t));
void	*memset __P((void *, int, size_t));
#if __GNUC_PREREQ__(2, 95) && (__GNUC_PREREQ__(4, 0) || !defined(__vax__))
#define	memcpy(d, s, l)		__builtin_memcpy(d, s, l)
#define	memcmp(a, b, l)		__builtin_memcmp(a, b, l)
#endif
#if __GNUC_PREREQ__(2, 95) && !defined(__vax__)
#define	memset(d, v, l)		__builtin_memset(d, v, l)
#endif

char	*strcpy __P((char *, const char *));
int	 strcmp __P((const char *, const char *));
size_t	 strlen __P((const char *));
char	*strsep(char **, const char *);
#if __GNUC_PREREQ__(2, 95)
#define	strcpy(d, s)		__builtin_strcpy(d, s)
#define	strcmp(a, b)		__builtin_strcmp(a, b)
#define	strlen(a)		__builtin_strlen(a)
#endif

/* Functions for which we always use built-ins. */
#ifdef __GNUC__
#define	alloca(s)		__builtin_alloca(s)
#endif

/* These exist in GCC 3.x, but we don't bother. */
char	*strcat __P((char *, const char *));
char	*strncpy __P((char *, const char *, size_t));
int	 strncmp __P((const char *, const char *, size_t));
char	*strchr __P((const char *, int));
char	*strrchr __P((const char *, int));

char	*strstr __P((const char *, const char *));

/*
 * ffs is an instruction on vax.
 */
int	 ffs __P((int));
#if __GNUC_PREREQ__(2, 95) && (!defined(__vax__) || __GNUC_PREREQ__(4,1))
#define	ffs(x)		__builtin_ffs(x)
#endif

void	 __kernassert __P((const char *, const char *, int, const char *));
unsigned int
	bcdtobin __P((unsigned int));
unsigned int
	bintobcd __P((unsigned int));
u_int32_t
	inet_addr __P((const char *));
struct in_addr;
int	inet_aton __P((const char *, struct in_addr *));
char	*intoa __P((u_int32_t));
#define inet_ntoa(a) intoa((a).s_addr)
void	*memchr __P((const void *, int, size_t));
void	*memmove __P((void *, const void *, size_t));
int	 pmatch __P((const char *, const char *, const char **));
u_int32_t arc4random __P((void));
void	 arc4randbytes __P((void *, size_t));
#ifndef SMALL_RANDOM
void	 srandom __P((unsigned long));
char	*initstate __P((unsigned long, char *, size_t));
char	*setstate __P((char *));
#endif /* SMALL_RANDOM */
long	 random __P((void));
int	 scanc __P((u_int, const u_char *, const u_char *, int));
int	 skpc __P((int, size_t, u_char *));
int	 strcasecmp __P((const char *, const char *));
size_t	 strlcpy __P((char *, const char *, size_t));
size_t	 strlcat __P((char *, const char *, size_t));
int	 strncasecmp __P((const char *, const char *, size_t));
u_long	 strtoul __P((const char *, char **, int));
long long strtoll __P((const char *, char **, int));
unsigned long long strtoull __P((const char *, char **, int));
uintmax_t strtoumax __P((const char *, char **, int));
#endif /* !_LIB_LIBKERN_LIBKERN_H_ */
