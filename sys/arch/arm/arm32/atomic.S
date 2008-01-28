/*	atomic.S,v 1.1.90.1 2007/09/11 18:41:22 matt Exp */

/*
 * Copyright (C) 1994-1997 Mark Brinicombe
 * Copyright (C) 1994 Brini
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

#include "assym.h"
#include <machine/asm.h>
#include <machine/atomic.h>
#include <machine/cpu.h>
	
#ifdef	ATOMIC_SET_BIT_NONINLINE_REQUIRED
/*
 * Atomic bit set and clear functions
 */

#undef atomic_set_bit
ENTRY(atomic_set_bit)
	mrs	r2, cpsr
	orr	r3, r2, #(I32_bit)
	msr	cpsr_all, r3

	ldr	r3, [r0]
	orr	r3, r3, r1
	str	r3, [r0]

	msr	cpsr_all, r2
	mov	pc, lr


#undef atomic_clear_bit
ENTRY(atomic_clear_bit)
	mrs	r2, cpsr
	orr	r3, r2, #(I32_bit)
	msr	cpsr_all, r3

	ldr	r3, [r0]
	bic	r3, r3, r1
	str	r3, [r0]

	msr	cpsr_all, r2
	mov	pc, lr

#endif	/* ATOMIC_SET_BIT_NONINLINE_REQUIRED */

#if 0 && defined(_ARM_ARCH_6)

#define	ATOMIC_OP(NAME, OP, ARG)	\
ENTRY_NP(atomic_##NAME##_32)		;\
	mov	ip, r0			;\
1:      ldrex	r0, [ip]		;\
	OP	r2, r0, ARG		;\
	strex	r3, r2, [ip]		;\
	cmp	r3, #0			;\
	bne	1b			;\
	RET				;\
	END(atomic_##NAME##_32)

ATOMIC_OP(and, and, r1)
ATOMIC_OP(nand, bic, r1)
ATOMIC_OP(or, orr, r1)
ATOMIC_OP(xor, eor, r1)
ATOMIC_OP(add, add, r1)
ATOMIC_OP(inc, add, #1)
ATOMIC_OP(sub, sub, r1)
ATOMIC_OP(dec, sub, #1)

#define	ATOMIC_OP_NV(NAME, OP, ARG)	\
ENTRY_NP(atomic_##NAME##_32_nv)		;\
	mov	ip, r0			;\
1:      ldrex	r0, [ip]		;\
	OP	r0, r0, ARG		;\
	strex	r3, r0, [ip]		;\
	cmp	r3, #0			;\
	bne	1b			;\
	RET				;\
	END(atomic_##NAME##_32_nv)

ATOMIC_OP_NV(and, and, r1)
ATOMIC_OP_NV(nand, bic, r1)
ATOMIC_OP_NV(or, orr, r1)
ATOMIC_OP_NV(xor, eor, r1)
ATOMIC_OP_NV(add, add, r1)
ATOMIC_OP_NV(inc, add, #1)
ATOMIC_OP_NV(sub, sub, r1)
ATOMIC_OP_NV(dec, sub, #1)

#endif /* _ARM_ARCH_6 */
