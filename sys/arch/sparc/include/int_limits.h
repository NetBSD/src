/*	$NetBSD: int_limits.h,v 1.1.2.2 2001/04/21 17:54:38 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SPARC_INT_LIMITS_H_
#define _SPARC_INT_LIMITS_H_

/*
 * 7.18.2 Limits of specified-width integer types
 */

/* 7.18.2.1 Limits of exact-width integer types */

/* minimum values of exact-width signed integer types */
#define	INT8_MIN	(-0x7f-1)			/* int8_t	  */
#define	INT16_MIN	(-0x7fff-1)			/* int16_t	  */
#define	INT32_MIN	(-0x7fffffff-1)			/* int32_t	  */
#ifdef __arch64__
#define	INT64_MIN	(-0x7fffffffffffffffL-1)	/* int64_t	  */
#else
#define	INT64_MIN	(-0x7fffffffffffffffLL-1)	/* int64_t	  */
#endif

/* maximum values of exact-width signed integer types */
#define	INT8_MAX	0x7f				/* int8_t	  */
#define	INT16_MAX	0x7fff				/* int16_t	  */
#define	INT32_MAX	0x7fffffff			/* int32_t	  */
#ifdef __arch64__
#define	INT64_MAX	0x7fffffffffffffffL		/* int64_t	  */
#else
#define	INT64_MAX	0x7fffffffffffffffLL		/* int64_t	  */
#endif

/* maximum values of exact-width unsigned integer types */
#define	UINT8_MAX	0xffU				/* uint8_t	  */
#define	UINT16_MAX	0xffffU				/* uint16_t	  */
#define	UINT32_MAX	0xffffffffU			/* uint32_t	  */
#ifdef __arch64__
#define	UINT64_MAX	0xffffffffffffffffUL		/* uint64_t	  */
#else
#define	UINT64_MAX	0xffffffffffffffffULL		/* uint64_t	  */
#endif

/* 7.18.2.2 Limits of minimum-width integer types */

/* minimum values of minimum-width signed integer types */
#define	INT_LEAST8_MIN	(-0x7f-1)			/* int_least8_t	  */
#define	INT_LEAST16_MIN	(-0x7fff-1)			/* int_least16_t  */
#define	INT_LEAST32_MIN	(-0x7fffffff-1)			/* int_least32_t  */
#ifdef __arch64__
#define	INT_LEAST64_MIN	(-0x7fffffffffffffffL-1)	/* int_least64_t  */
#else
#define	INT_LEAST64_MIN	(-0x7fffffffffffffffLL-1)	/* int_least64_t  */
#endif

/* maximum values of minimum-width signed integer types */
#define	INT_LEAST8_MAX	0x7f				/* int_least8_t	  */
#define	INT_LEAST16_MAX	0x7fff				/* int_least16_t  */
#define	INT_LEAST32_MAX	0x7fffffff			/* int_least32_t  */
#ifdef __arch64__
#define	INT_LEAST64_MAX	0x7fffffffffffffffL		/* int_least64_t  */
#else
#define	INT_LEAST64_MAX	0x7fffffffffffffffLL		/* int_least64_t  */
#endif

/* maximum values of minimum-width unsigned integer types */
#define	UINT_LEAST8_MAX	 0xffU				/* uint_least8_t  */
#define	UINT_LEAST16_MAX 0xffffU			/* uint_least16_t */
#define	UINT_LEAST32_MAX 0xffffffffU			/* uint_least32_t */
#ifdef __arch64__
#define	UINT_LEAST64_MAX 0xffffffffffffffffUL		/* uint_least64_t */
#else
#define	UINT_LEAST64_MAX 0xffffffffffffffffULL		/* uint_least64_t */
#endif


/* 7.18.2.4 Limits of integer types capable of holding object pointers */

#ifdef __arch64__
#define	INTPTR_MIN	(-0x7fffffffffffffffL-1)	/* intptr_t	  */
#define	INTPTR_MAX	0x7fffffffffffffffL		/* intptr_t	  */
#define	UINTPTR_MAX	0xffffffffffffffffUL		/* uintptr_t	  */
#else
#define	INTPTR_MIN	(-0x7fffffffL-1)		/* intptr_t	  */
#define	INTPTR_MAX	0x7fffffffL			/* intptr_t	  */
#define	UINTPTR_MAX	0xffffffffUL			/* uintptr_t	  */
#endif

/* 7.18.2.5 Limits of greatest-width integer types */

#ifdef __arch64__
#define	INTMAX_MIN	(-0x7fffffffffffffffL-1)	/* intmax_t	  */
#define	INTMAX_MAX	0x7fffffffffffffffL		/* intmax_t	  */
#define	UINTMAX_MAX	0xffffffffffffffffUL		/* uintmax_t	  */
#else
#define	INTMAX_MIN	(-0x7fffffffffffffffLL-1)	/* intmax_t	  */
#define	INTMAX_MAX	0x7fffffffffffffffLL		/* intmax_t	  */
#define	UINTMAX_MAX	0xffffffffffffffffULL		/* uintmax_t	  */
#endif


/*
 * 7.18.3 Limits of other integer types
 */

/* limits of ptrdiff_t */
#ifdef __arch64__
#define	PTRDIFF_MIN	(-0x7fffffffffffffffL-1)	/* ptrdiff_t	  */
#define	PTRDIFF_MAX	0x7fffffffffffffffL		/* ptrdiff_t	  */
#else
#define	PTRDIFF_MIN	(-0x7fffffffL-1)		/* ptrdiff_t	  */
#define	PTRDIFF_MAX	0x7fffffffL			/* ptrdiff_t	  */
#endif

/* limits of sig_atomic_t */
#define	SIG_ATOMIC_MIN	(-0x7fffffff-1)			/* sig_atomic_t	  */
#define	SIG_ATOMIC_MAX	0x7fffffff			/* sig_atomic_t	  */

/* limit of size_t */
#ifdef __arch64__
#define	SIZE_MAX	0xffffffffffffffffUL		/* size_t	  */
#else
#define	SIZE_MAX	0xffffffffUL			/* size_t	  */
#endif

#ifndef WCHAR_MIN /* also possibly defined in <wchar.h> */
/* limits of wchar_t */
#define	WCHAR_MIN	(-0x7fffffff-1)			/* wchar_t	  */
#define	WCHAR_MAX	0x7fffffff			/* wchar_t	  */

/* limits of wint_t */
#define	WINT_MIN	(-0x7fffffff-1)			/* wint_t	  */
#define	WINT_MAX	0x7fffffff			/* wint_t	  */
#endif

#endif /* !_SPARC_INT_LIMITS_H_ */
