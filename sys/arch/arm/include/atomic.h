/* $NetBSD: atomic.h,v 1.5.26.2 2007/03/12 05:47:04 rmind Exp $ */

/*
 * Copyright (C) 1994-1997 Mark Brinicombe
 * Copyright (C) 1994 Brini
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of Brini may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_ATOMIC_H_
#define	_ARM_ATOMIC_H_

#ifndef ATOMIC_SET_BIT_NONINLINE_REQUIRED

#ifdef ATOMIC_SET_BIT_NOINLINE
#define	ATOMIC_SET_BIT_NONINLINE_REQUIRED
#endif

#endif /* ATOMIC_SET_BIT_NONINLINE_REQUIRED */


#ifndef _LOCORE
#ifdef _KERNEL
#include <sys/types.h>
#include <arm/armreg.h>			/* I32_bit */

#ifdef ATOMIC_SET_BIT_NONINLINE_REQUIRED
void atomic_set_bit( u_int *, u_int );
void atomic_clear_bit( u_int *, u_int );
bool atomic_cas(volatile uintptr_t *, uintptr_t, uintptr_t);
#endif

#ifdef __PROG32
#define __with_interrupts_disabled(expr) \
	do {						\
		u_int cpsr_save, tmp;			\
							\
		__asm volatile(			\
			"mrs  %0, cpsr;"		\
			"orr  %1, %0, %2;"		\
			"msr  cpsr_all, %1;"		\
			: "=r" (cpsr_save), "=r" (tmp)	\
			: "I" (I32_bit)		\
		        : "cc" );		\
		(expr);				\
		 __asm volatile(		\
			"msr  cpsr_all, %0"	\
			: /* no output */	\
			: "r" (cpsr_save)	\
			: "cc" );		\
	} while(0)
#else /* __PROG26 */
#define __with_interrupts_disabled(expr)				\
	do {						\
		u_int r15_save, tmp;			\
							\
		__asm volatile(				\
			"mov  %0, r15;"			\
			"orr  %1, %0, %2;"		\
			"teqp %1, #0;"			\
			: "=r" (r15_save), "=r" (tmp)	\
			: "I" (R15_IRQ_DISABLE)		\
		        : "cc" );		\
		(expr);				\
		 __asm volatile(		\
			"teqp  %0, #0"		\
			: /* no output */	\
			: "r" (r15_save)	\
			: "cc" );		\
	} while(0)
#endif /* __PROG26 */


static __inline void
inline_atomic_set_bit( u_int *address, u_int setmask )
{
	__with_interrupts_disabled( *address |= setmask );
}

static __inline void
inline_atomic_clear_bit( u_int *address, u_int clearmask )
{
	__with_interrupts_disabled( *address &= ~clearmask );
}

static __inline bool
inline_atomic_cas(volatile uintptr_t *cell, uintptr_t old, uintptr_t new)
{
	bool rv;
	__with_interrupts_disabled(
	    rv = (*cell == old ? ((*cell = new), true) : false) );
	return rv;
}

#if !defined(ATOMIC_SET_BIT_NOINLINE)

#define atomic_set_bit(a,m)   inline_atomic_set_bit(a,m)
#define atomic_clear_bit(a,m) inline_atomic_clear_bit(a,m)
#define	atomic_cas(c,o,n)     inline_atomic_cas(c,o,n)

#endif

#undef __with_interrupts_disabled

#endif /* _KERNEL */
#endif /* _LOCORE */
#endif /* _ARM_ATOMIC_H_ */
