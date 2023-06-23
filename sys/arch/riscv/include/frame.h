/* $NetBSD: frame.h,v 1.6 2023/06/23 12:11:22 skrll Exp $ */

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

#ifndef _RISCV_FRAME_H_
#define _RISCV_FRAME_H_

#include <riscv/reg.h>

struct trapframe {
	struct reg tf_regs;
	register_t tf_tval;		// supervisor trap value
	register_t tf_cause;		// supervisor cause register
	register_t tf_sr;		// supervisor status register
	register_t tf_pad;		// For 16-byte alignment
#define tf_reg		tf_regs.r_reg
#define tf_pc		tf_regs.r_pc
#define tf_ra		tf_reg[_X_RA]
#define tf_sp		tf_reg[_X_SP]
#define tf_gp		tf_reg[_X_GP]
#define tf_tp		tf_reg[_X_TP]
#define tf_t0		tf_reg[_X_T0]
#define tf_t1		tf_reg[_X_T1]
#define tf_t2		tf_reg[_X_T2]
#define tf_s0		tf_reg[_X_S0]
#define tf_s1		tf_reg[_X_S1]
#define tf_a0		tf_reg[_X_A0]
#define tf_a1		tf_reg[_X_A1]
#define tf_a2		tf_reg[_X_A2]
#define tf_a3		tf_reg[_X_A3]
#define tf_a4		tf_reg[_X_A4]
#define tf_a5		tf_reg[_X_A5]
#define tf_a6		tf_reg[_X_A6]
#define tf_a7		tf_reg[_X_A7]
#define tf_s2		tf_reg[_X_S2]
#define tf_s3		tf_reg[_X_S3]
#define tf_s4		tf_reg[_X_S4]
#define tf_s5		tf_reg[_X_S5]
#define tf_s6		tf_reg[_X_S6]
#define tf_s7		tf_reg[_X_S7]
#define tf_s8		tf_reg[_X_S8]
#define tf_s9		tf_reg[_X_S9]
#define tf_s10		tf_reg[_X_S10]
#define tf_s11		tf_reg[_X_S11]
#define tf_t3		tf_reg[_X_T3]
#define tf_t4		tf_reg[_X_T4]
#define tf_t5		tf_reg[_X_T5]
#define tf_t6		tf_reg[_X_T6]
};

/*
 * Ensure the trapframe is a multiple of 16bytes so that stack
 * alignment is preserved.
 */
__CTASSERT((sizeof(struct trapframe) & (16 - 1)) == 0);

#ifdef _LP64
// For COMPAT_NETBSD32 coredumps
struct trapframe32 {
	struct reg32 tf_regs;
	register32_t tf_tval;
	register32_t tf_cause;
	register32_t tf_sr;
	register32_t tf_pad;
};
#endif


#define lwp_trapframe(l) ((l)->l_md.md_utf)


#endif /* _RISCV_FRAME_H_ */
