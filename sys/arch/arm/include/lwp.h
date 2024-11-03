/*	$NetBSD: lwp.h,v 1.1 2024/11/03 22:24:21 christos Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein and by Jason R. Thorpe of Wasabi Systems, Inc.
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

#ifndef _ARM_LWP_H_
#define _ARM_LWP_H_

#include <sys/stdint.h>
#include <sys/tls.h>

#if defined(__aarch64__)

__BEGIN_DECLS
static __inline void *
__lwp_getprivate_fast(void)
{
	void *__tpidr;
	__asm __volatile("mrs\t%0, tpidr_el0" : "=r"(__tpidr));
	return __tpidr;
}
__END_DECLS

#elif defined(__arm__)

#if defined(__thumb__) && !defined(_ARM_ARCH_T2)
#include <arm/eabi.h>
#endif

__BEGIN_DECLS
static __inline void *
__lwp_getprivate_fast(void)
{
#if !defined(__thumb__) || defined(_ARM_ARCH_T2)
	void *rv;
	__asm("mrc p15, 0, %0, c13, c0, 3" : "=r"(rv));
	if (__predict_true(rv))
		return rv;
	/*
	 * Some ARM cores are broken and don't raise an undefined fault when an
	 * unrecogized mrc instruction is encountered, but just return zero.
	 * To do deal with that, if we get a zero we (re-)fetch the value using
	 * syscall.
	 */
	return _lwp_getprivate();
#else
	return __aeabi_read_tp();
#endif /* !__thumb__ || _ARM_ARCH_T2 */
}
__END_DECLS
#endif

#endif	/* !_ARM_LWP_H_ */
