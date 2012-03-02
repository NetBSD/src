/*	$NetBSD: pthread_md.h,v 1.20 2012/03/02 23:19:47 joerg Exp $	*/

/*-
 * Copyright (c) 2001, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams, and by Andrew Doran.
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

#ifndef _LIB_PTHREAD_I386_MD_H
#define _LIB_PTHREAD_I386_MD_H

#include <sys/ucontext.h>
#include <ucontext.h>

static inline unsigned long
pthread__sp(void)
{
	unsigned long ret;
	__asm("movl %%esp, %0" : "=g" (ret));

	return ret;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_UESP])

static inline void
_initcontext_u_md(ucontext_t *ucp)
{
	__asm ("pushfl; popl %0" : "=a" (ucp->uc_mcontext.__gregs[_REG_EFL]));
	__asm ("pushl %%cs; popl %0" : "=a" (ucp->uc_mcontext.__gregs[_REG_CS]));
	__asm ("movl %%ds, %0" : "=a" (ucp->uc_mcontext.__gregs[_REG_DS]));
	__asm ("movl %%es, %0" : "=a" (ucp->uc_mcontext.__gregs[_REG_ES]));
	__asm ("movl %%fs, %0" : "=a" (ucp->uc_mcontext.__gregs[_REG_FS]));
	__asm ("movl %%gs, %0" : "=a" (ucp->uc_mcontext.__gregs[_REG_GS]));
	__asm ("movl %%ss, %0" : "=a" (ucp->uc_mcontext.__gregs[_REG_SS]));
}

#define	_INITCONTEXT_U_MD(ucp)	_initcontext_u_md(ucp);

#define	pthread__smt_pause()	__asm __volatile("rep; nop" ::: "memory")

/* Don't need additional memory barriers. */
#define	PTHREAD__ATOMIC_IS_MEMBAR

static inline void *
_atomic_cas_ptr(volatile void *ptr, void *old, void *new)
{
	volatile uintptr_t *cast = ptr;
	void *ret;

	__asm __volatile ("lock; cmpxchgl %2, %1"
		: "=a" (ret), "=m" (*cast)
		: "r" (new), "m" (*cast), "0" (old));

	return ret;
}

static inline void *
_atomic_cas_ptr_ni(volatile void *ptr, void *old, void *new)
{
	volatile uintptr_t *cast = ptr;
	void *ret;

	__asm __volatile ("cmpxchgl %2, %1"
		: "=a" (ret), "=m" (*cast)
		: "r" (new), "m" (*cast), "0" (old));

	return ret;
}

#endif /* _LIB_PTHREAD_I386_MD_H */
