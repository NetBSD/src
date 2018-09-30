/*	$NetBSD: nv_compat.h,v 1.1.2.2 2018/09/30 01:45:55 pgoyette Exp $	*/

/*
 * Copyright (c) 1987, 1991, 1993
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
 *	@(#)endian.h	8.1 (Berkeley) 6/11/93
 */

#ifndef	_NV_COMPAT_H_
#define	_NV_COMPAT_H_

#if defined(__NetBSD__) && (defined(_KERNEL) || defined(_STANDALONE))
#ifdef _KERNEL
#include <sys/malloc.h>
#define	M_NVLIST		M_TEMP
#endif
#include <sys/stdarg.h>
#include <lib/libkern/libkern.h>

#endif

#ifndef	__unused
#define	__unused		__attribute__((__unused__))
#endif

#ifndef	__DECONST
#define	__DECONST(type, var)	((type)(uintptr_t)(const void *)(var))
#endif

#ifndef	__printflike
#define __printflike(fmtarg, firstvararg)	\
	    __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#endif

#ifdef __linux__
#include <endian.h>
#else
#include <sys/endian.h>
#endif

#ifdef __linux__
static inline uint32_t
be32dec(const void *buf)
{
	uint8_t const *p = (uint8_t const *)buf;
	return (((unsigned)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static inline uint32_t
le32dec(const void *buf)
{
	uint8_t const *p = (uint8_t const *)buf;
	return (((unsigned)p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

static inline uint64_t
be64dec(const void *buf)
{
	uint8_t const *p = (uint8_t const *)buf;
	return (((uint64_t)be32dec(p) << 32) | be32dec(p + 4));
}

static inline uint64_t
le64dec(const void *buf)
{
	uint8_t const *p = (uint8_t const *)buf;
	return (((uint64_t)le32dec(p + 4) << 32) | le32dec(p));
}
#endif

#endif
