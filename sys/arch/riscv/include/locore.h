/* $NetBSD: locore.h,v 1.1 2014/09/19 17:36:26 matt Exp $ */
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

#ifndef _RISCV_LOCORE_H_
#define _RISCV_LOCORE_H_

#include <sys/lwp.h>
#include <sys/userret.h>

#include <riscv/reg.h>
#include <riscv/sysreg.h>

struct trapframe {
	struct reg tf_regs;
	register_t tf_badvaddr;	
	uint32_t tf_cause;		// 32-bit register
	uint32_t tf_sr;			// 32-bit register
#define tf_reg		tf_regs.r_reg
#define tf_pc		tf_regs.r_pc
#define tf_ra		tf_reg[_X_RA]
#define tf_s0		tf_reg[_X_S0]
#define tf_s1		tf_reg[_X_S1]
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
#define tf_sp		tf_reg[_X_SP]
#define tf_tp		tf_reg[_X_TP]
#define tf_v0		tf_reg[_X_V0]
#define tf_v1		tf_reg[_X_V1]
#define tf_a0		tf_reg[_X_A0]
#define tf_a1		tf_reg[_X_A1]
#define tf_a2		tf_reg[_X_A2]
#define tf_a3		tf_reg[_X_A3]
#define tf_a4		tf_reg[_X_A4]
#define tf_a5		tf_reg[_X_A5]
#define tf_a6		tf_reg[_X_A6]
#define tf_a7		tf_reg[_X_A7]
#define tf_t0		tf_reg[_X_T0]
#define tf_t1		tf_reg[_X_T1]
#define tf_t2		tf_reg[_X_T2]
#define tf_t3		tf_reg[_X_T3]
#define tf_t4		tf_reg[_X_T4]
#define tf_gp		tf_reg[_X_GP]
};

// For COMPAT_NETBDS32 coredumps
struct trapframe32 {
	struct reg32 tf_regs;
	register32_t tf_badvaddr;	
	uint32_t tf_cause;		// 32-bit register
	uint32_t tf_sr;			// 32-bit register
};

struct faultbuf {
	register_t fb_reg[_X_SP+1];
	register_t fb_v0;
	uint32_t fb_sr;
};

CTASSERT(sizeof(label_t) == sizeof(struct faultbuf));

struct mainbus_attach_args {
	const char *maa_name;
	u_int maa_instance;
};

#ifdef _KERNEL
extern int cpu_printfataltraps;
extern const pcu_ops_t pcu_fpu_ops;

static inline vaddr_t
stack_align(vaddr_t sp)
{
	return sp & ~STACK_ALIGNBYTES;
}

static inline void
userret(struct lwp *l)
{
	mi_userret(l);
}

static inline void
fpu_load(void)
{
	pcu_load(&pcu_fpu_ops);
}

static inline void
fpu_save(void)
{
	pcu_save(&pcu_fpu_ops);
}

static inline void
fpu_discard(void)
{
	pcu_discard(&pcu_fpu_ops, false);
}

static inline void
fpu_replace(void)
{
	pcu_discard(&pcu_fpu_ops, true);
}

static inline bool
fpu_valid_p(void)
{
	return pcu_valid_p(&pcu_fpu_ops);
}

void	__syncicache(const void *, size_t);

int	cpu_set_onfault(struct faultbuf *, register_t) __returns_twice;
void	cpu_jump_onfault(struct trapframe *, struct faultbuf *);

static inline void
cpu_unset_onfault(void)
{
	curlwp->l_md.md_onfault = NULL;
}

static inline struct faultbuf *
cpu_disable_onfault(void)
{
	struct faultbuf * const fb = curlwp->l_md.md_onfault;
	curlwp->l_md.md_onfault = NULL;
	return fb;
}

static inline void
cpu_enable_onfault(struct faultbuf *fb)
{
	curlwp->l_md.md_onfault = fb;
}

void	cpu_intr(struct trapframe */*tf*/, register_t /*epc*/,
	    register_t /*status*/, register_t /*cause*/,
	    register_t /*badvaddr*/);
void	cpu_trap(struct trapframe */*tf*/, register_t /*epc*/,
	    register_t /*status*/, register_t /*cause*/,
	    register_t /*badvaddr*/);
void	cpu_ast(struct trapframe *);
void	cpu_fast_switchto(struct lwp *, int);

void	cpu_lwp_trampoline(void);

void *	cpu_sendsig_getframe(struct lwp *, int, bool *);

#endif

#endif /* _RISCV_LOCORE_H_ */
