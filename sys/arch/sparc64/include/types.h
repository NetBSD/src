/*	$NetBSD: types.h,v 1.14 2001/01/03 10:09:05 takemura Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)types.h	8.1 (Berkeley) 6/11/93
 */

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

#ifdef sun
#undef sun
#endif

#include <sys/cdefs.h>
#include <machine/int_types.h>

#if defined(_KERNEL)
typedef struct label_t {
	int val[2];
} label_t;
#endif

/*
 * Basic integral types.  Omit the typedef if
 * not possible for a machine/compiler combination.
 */
#define	__BIT_TYPES_DEFINED__
typedef	__int8_t	   int8_t;
typedef	__uint8_t	 u_int8_t;
typedef	__int16_t	  int16_t;
typedef	__uint16_t	u_int16_t;
typedef	__int32_t	  int32_t;
typedef	__uint32_t	u_int32_t;
typedef	__int64_t	  int64_t;
typedef	__uint64_t	u_int64_t;

/* The following are unsigned to prevent annoying sign extended pointers. */
typedef unsigned long		register_t;
typedef u_int32_t		register32_t;
typedef u_int64_t		register64_t;

#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
typedef unsigned long		vaddr_t;
typedef vaddr_t			vsize_t;
#ifdef SUN4U
typedef u_int64_t		paddr_t;
#else
typedef unsigned long		paddr_t;
#endif
typedef paddr_t			psize_t;
#endif

#define __HAVE_DEVICE_REGISTER

#endif	/* _MACHTYPES_H_ */
