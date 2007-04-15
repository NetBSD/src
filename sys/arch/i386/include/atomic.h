/* $NetBSD: atomic.h,v 1.8.2.1 2007/04/15 16:02:48 yamt Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _I386_ATOMIC_H_
#define _I386_ATOMIC_H_

#ifndef _LOCORE

static __inline unsigned long x86_atomic_testset_ul(volatile uint32_t *,
    unsigned long);
static __inline unsigned long
x86_atomic_testset_ul(volatile uint32_t *__ptr, unsigned long __val) {
	__asm volatile ("xchgl %0,(%2)" :"=r" (__val):"0" (__val),"r" (__ptr));
	return __val;
}

static __inline int x86_atomic_testset_i(volatile int *, int);
static __inline int
x86_atomic_testset_i(volatile int *__ptr, int __val) {
	__asm volatile ("xchgl %0,(%2)" :"=r" (__val):"0" (__val),"r" (__ptr));
	return __val;
}

static __inline uint8_t x86_atomic_testset_b(volatile uint8_t *, uint8_t);
static __inline uint8_t
x86_atomic_testset_b(volatile uint8_t *__ptr, uint8_t __val) {
	__asm volatile ("xchgb %0,(%2)" :"=A" (__val):"0" (__val),"r" (__ptr));
	return __val;
}

static __inline void x86_atomic_setbits_l(volatile uint32_t *, unsigned long);
static __inline void
x86_atomic_setbits_l(volatile uint32_t *__ptr, unsigned long __bits) {
	__asm volatile("lock ; orl %1,%0" :  "=m" (*__ptr) : "ir" (__bits));
}

static __inline void x86_atomic_clearbits_l(volatile uint32_t *, unsigned long);
static __inline void
x86_atomic_clearbits_l(volatile uint32_t *__ptr, unsigned long __bits) {
	__asm volatile("lock ; andl %1,%0" :  "=m" (*__ptr) : "ir" (~__bits));
}

#endif
#endif
