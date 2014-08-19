/*	$NetBSD: barrier.h,v 1.2.10.2 2014/08/20 00:04:21 tls Exp $	*/

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

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

#define	mb	membar_sync
#define	wmb	membar_producer
#define	rmb	membar_consumer

#ifdef __alpha__		/* XXX As if...  */
#  define	read_barrier_depends	membar_sync
#else
#  define	read_barrier_depends()	do {} while (0)
#endif

#ifdef MULTIPROCESSOR
#  define	smp_mb				mb
#  define	smp_wmb				wmb
#  define	smp_rmb				rmb
#  define	smp_read_barrier_depends	read_barrier_depends
#else
#  define	smp_mb()			do {} while (0)
#  define	smp_wmb()			do {} while (0)
#  define	smp_rmb()			do {} while (0)
#  define	smp_read_barrier_depends()	do {} while (0)
#endif

#endif  /* _ASM_BARRIER_H_ */
