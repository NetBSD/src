/* $NetBSD: types.h,v 1.49.6.1 2017/12/03 11:35:46 jdolecek Exp $ */

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
 *	@(#)types.h	8.3 (Berkeley) 1/5/94
 */

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <machine/int_types.h>

#if defined(_KERNEL)
typedef struct label_t {
	long	val[10];
} label_t;
#endif

typedef	int		__cpu_simple_lock_nv_t;
typedef long int	__register_t;
typedef unsigned long	__paddr_t;	/* XXX: For bus_user.h */


#if defined(_KERNEL) || defined(_KMEMUSER) || defined(_KERNTYPES) || defined(_STANDALONE)
typedef __paddr_t	paddr_t;
typedef unsigned long	psize_t;
typedef unsigned long	vaddr_t;
typedef unsigned long	vsize_t;
#define	PRIxPADDR	"lx"
#define	PRIxPSIZE	"lx"
#define	PRIuPSIZE	"lu"
#define	PRIxVADDR	"lx"
#define	PRIxVSIZE	"lx"
#define	PRIuVSIZE	"lu"

typedef __register_t	register_t;
#define	PRIxREGISTER	"lx"
#endif

#define	__SIMPLELOCK_LOCKED	1
#define	__SIMPLELOCK_UNLOCKED	0

#define	__HAVE_NEW_STYLE_BUS_H
#define	__HAVE_ATOMIC_OPERATIONS
#define	__HAVE_MEMBAR_DATADEP_CONSUMER
#define	__HAVE_CPU_COUNTER
#define	__HAVE_SYSCALL_INTERN
#define	__HAVE_MINIMAL_EMUL
#define	__HAVE_AST_PERPROC
#define	__HAVE_ATOMIC64_OPS
#define	__HAVE_MM_MD_DIRECT_MAPPED_IO
#define	__HAVE_MM_MD_DIRECT_MAPPED_PHYS
#define	__HAVE_CPU_UAREA_ROUTINES
#define	__HAVE_CPU_LWP_SETPRIVATE
#define	__HAVE___LWP_GETPRIVATE_FAST
#define	__HAVE_COMMON___TLS_GET_ADDR
#define	__HAVE_TLS_VARIANT_I

#if defined(_KERNEL)
#define	__HAVE_RAS
#endif

#if defined(_KERNEL) || defined(_KMEMUSER)
#define	PCU_FPU		0	/* FPU */
#define	PCU_UNIT_COUNT	1
#endif

#endif	/* _MACHTYPES_H_ */
