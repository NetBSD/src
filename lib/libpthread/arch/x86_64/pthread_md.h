/*	$NetBSD: pthread_md.h,v 1.11 2009/05/16 22:23:45 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
 *
 * Adapted for x86_64 by fvdl@NetBSD.org
 */

#ifndef _LIB_PTHREAD_X86_64_MD_H
#define _LIB_PTHREAD_X86_64_MD_H

#include <sys/ucontext.h>

static inline long
pthread__sp(void)
{
	long ret;
	__asm("movq %%rsp, %0" : "=g" (ret));

	return ret;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_URSP])

/*
 * Set initial, sane values for registers whose values aren't just
 * "don't care".
 * 0x23 is GSEL(GUDATA_SEL, SEL_UPL), and
 * 0x1b is GSEL(GUCODE_SEL, SEL_UPL).
 * 0x202 is PSL_USERSET.
 */
#define _INITCONTEXT_U_MD(ucp)						\
	(ucp)->uc_mcontext.__gregs[_REG_GS] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_FS] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_ES] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_DS] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_CS] = 0x1b,			\
	(ucp)->uc_mcontext.__gregs[_REG_SS] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_RFL] = 0x202;

#define	pthread__smt_pause()	__asm __volatile("rep; nop" ::: "memory")

/* Don't need additional memory barriers. */
#define	PTHREAD__ATOMIC_IS_MEMBAR

static inline void *
_atomic_cas_ptr(volatile void *ptr, void *old, void *new)
{
	volatile uintptr_t *cast = ptr;
	void *ret;

	__asm __volatile ("lock; cmpxchgq %2, %1"
		: "=a" (ret), "=m" (*cast)
		: "r" (new), "m" (*cast), "0" (old));

	return ret;
}

static inline void *
_atomic_cas_ptr_ni(volatile void *ptr, void *old, void *new)
{
	volatile uintptr_t *cast = ptr;
	void *ret;

	__asm __volatile ("cmpxchgq %2, %1"
		: "=a" (ret), "=m" (*cast)
		: "r" (new), "m" (*cast), "0" (old));

	return ret;
}

#endif /* _LIB_PTHREAD_X86_64_MD_H */
