/*	$NetBSD: types.h,v 1.41.14.1 2006/06/19 03:45:06 chap Exp $ */

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
 *	@(#)types.h	8.1 (Berkeley) 6/11/93
 */

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

#ifdef sun
#undef sun
#endif

#if defined(_KERNEL_OPT)
#include "opt_sparc_arch.h"
#endif

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <machine/int_types.h>

#if defined(_KERNEL)
typedef struct label_t {
	int val[2];
} label_t;
#endif

/* The following are unsigned to prevent annoying sign extended pointers. */
typedef unsigned long int	register_t;
typedef unsigned int		register32_t;
#ifdef __arch64__
typedef unsigned long int	register64_t;
#else
/* LONGLONG */
typedef unsigned long long int	register64_t;
#endif

#if defined(_NETBSD_SOURCE)
typedef unsigned long int	vaddr_t;
typedef vaddr_t			vsize_t;
#ifdef SUN4U
#ifdef __arch64__
typedef unsigned long int	paddr_t;
#else
/* LONGLONG */
typedef unsigned long long int	paddr_t;
#endif /* __arch64__ */
#else
typedef unsigned long int	paddr_t;
#endif /* SUN4U */
typedef paddr_t			psize_t;
#endif

/*
 * The value for __SIMPLELOCK_LOCKED is what ldstub() naturally stores
 * `lock_data' given its address (and the fact that SPARC is big-endian).
 */

typedef	volatile int		__cpu_simple_lock_t;

#define	__SIMPLELOCK_LOCKED	0xff000000
#define	__SIMPLELOCK_UNLOCKED	0

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
#define	__HAVE_SYSCALL_INTERN
#define	__GENERIC_SOFT_INTERRUPTS_ALL_LEVELS
#define	__HAVE_NWSCONS
#define	__HAVE_TIMECOUNTER

#ifdef SUN4U
#define __HAVE_CPU_COUNTER	/* sparc v9 CPUs have %tick */
#if defined(_KERNEL)
#define __HAVE_RAS
#endif
#endif


#endif	/* _MACHTYPES_H_ */
