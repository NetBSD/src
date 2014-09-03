/* $NetBSD: reg.h,v 1.1 2014/09/03 19:34:26 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _OR1K_REG_H_
#define _OR1K_REG_H_

// r0 = must contain 0
// r1 = SP (stack pointer)
// r2 = FP (frame pointer)
// r3-r8 = arg param 0-5
// r9 = LR (link register)
// r10 = TP (thread pointer)
// r11 = RV
// r12 = RVH (64-bit returns)
// r13, r15, r17, r19, r21, r23, r25, r27, r29, r31 - temporary
// r14, r16, r18, r20, r22, r24, r26, r28, r30 - callee saved
// r16 = GOT address (PIC)

struct reg {
#ifdef _LP64
	uint32_t r_reg[31]; /* r0 is always 0 */
#else
	uint64_t r_reg[31]; /* r0 is always 0 */
#endif
};

struct fpreg {
	uint32_t f_fpcsr;
};

#endif /* _OR1K_REG_H_ */
