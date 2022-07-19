/*	$NetBSD: barrier.h,v 1.18 2022/07/19 16:40:26 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _ASM_BARRIER_H_
#define _ASM_BARRIER_H_

#include <sys/atomic.h>

#include <linux/compiler.h>

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#else
#define	MULTIPROCESSOR	1	/* safer to assume multiprocessor */
#endif

/*
 * I/O memory barriers.  drm uses these to order memory operations on
 * normal or write-combining/prefetchable, or uncacheable I/O
 * operations, for coordination between the CPU and I/O devices.
 *
 * In NetBSD, this is normally done with bus_space_barrier, but Linux
 * doesn't pass around the bus space tag and handle needed for that.
 */
#if defined(__aarch64__)
#include <arm/cpufunc.h>
#define	mb()	dsb(sy)
#define	wmb()	dsb(st)
#define	rmb()	dsb(ld)
#elif defined(__arm__)
#include <arm/cpufunc.h>
#define	mb()	dsb()
#define	wmb()	mb()
#define	rmb()	mb()
#elif defined(__i386__) || defined(__x86_64__)
#include <x86/cpufunc.h>
#define	mb()	x86_mfence()
#define	wmb()	x86_sfence()
#define	rmb()	x86_lfence()
#elif defined(__powerpc__)
#define	mb()	__asm __volatile ("eieio" ::: "memory")
#define	wmb()	mb()
#define	rmb()	mb()
#elif defined(__sparc__) || defined(__sparc64__)
#ifdef __sparc64__
#define	mb()	__asm __volatile ("membar #MemIssue" ::: "memory")
#else
#define	mb()	membar_sync()	/* ldstub */
#endif
#define	wmb()	mb()		/* XXX could maybe be __insn_barrier in TSO */
#define	rmb()	mb()		/* XXX could maybe be __insn_barrier in TSO */
#elif defined(__mips__)
#include <mips/locore.h>
#define	mb()	wbflush()
#define	wmb()	mb()
#define	rmb()	mb()
#else
#error Missing Linux memory-mapped I/O barriers for this architecture.
#endif

/*
 * MP memory barriers.  drm uses these to order memory operations on
 * normal memory for coordination between CPUs.  Aliases for NetBSD's
 * membar_*.
 */
#ifdef MULTIPROCESSOR
#  define	smp_mb				membar_sync
#  define	smp_wmb				membar_producer
#  define	smp_rmb				membar_consumer
#else
#  define	smp_mb()			__insn_barrier()
#  define	smp_wmb()			__insn_barrier()
#  define	smp_rmb()			__insn_barrier()
#endif

#if defined(MULTIPROCESSOR) && !defined(__HAVE_ATOMIC_AS_MEMBAR)
#  define	smp_mb__before_atomic()		membar_release()
#  define	smp_mb__after_atomic()		membar_acquire()
#else
#  define	smp_mb__before_atomic()		__insn_barrier()
#  define	smp_mb__after_atomic()		__insn_barrier()
#endif

#endif  /* _ASM_BARRIER_H_ */
