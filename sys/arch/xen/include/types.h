/*	$NetBSD: types.h,v 1.4.14.1 2007/05/22 17:27:53 matt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
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
 *	@(#)types.h	7.5 (Berkeley) 3/9/91
 */

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <machine/int_types.h>

#if defined(_KERNEL)
typedef struct label_t {
	int val[6];
} label_t;
#endif

/* NB: This should probably be if defined(_KERNEL) */
#if defined(_NETBSD_SOURCE)
typedef unsigned long	paddr_t;
typedef unsigned long	psize_t;
typedef unsigned long	vaddr_t;
typedef unsigned long	vsize_t;
#endif

typedef int		pmc_evid_t;
typedef __uint64_t	pmc_ctr_t;
typedef int		register_t;

typedef	volatile unsigned char		__cpu_simple_lock_t;

/* __cpu_simple_lock_t used to be a full word. */
#define	__CPU_SIMPLE_LOCK_PAD

#define	__SIMPLELOCK_LOCKED	1
#define	__SIMPLELOCK_UNLOCKED	0

/* The x86 does not have strict alignment requirements. */
#define	__NO_STRICT_ALIGNMENT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_CPU_COUNTER
#define	__HAVE_SYSCALL_INTERN
#define	__HAVE_MINIMAL_EMUL
#define	__HAVE_OLD_DISKLABEL
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
#define	__HAVE_CPU_MAXPROC

#if defined(_KERNEL)
#define __HAVE_RAS
#endif

#define __HAVE_TIMECOUNTER

#endif	/* _MACHTYPES_H_ */
