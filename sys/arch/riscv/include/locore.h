/* $NetBSD: locore.h,v 1.12 2023/05/07 12:41:48 skrll Exp $ */

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

#define	FB_A0	0
#define	FB_RA	1
#define	FB_SP	2
#define	FB_GP	3
#define	FB_S0	4
#define	FB_S1	5
#define	FB_S2	6
#define	FB_S3	7
#define	FB_S4	8
#define	FB_S5	9
#define	FB_S6	10
#define	FB_S7	11
#define	FB_S8	12
#define	FB_S9	13
#define	FB_S10	14
#define	FB_S11	15
#define	FB_MAX	16

struct faultbuf {
	register_t fb_reg[FB_MAX];
	register_t fb_sr;
};

CTASSERT(sizeof(label_t) == sizeof(struct faultbuf));

#ifdef _KERNEL
extern int cpu_printfataltraps;

#ifdef FPE
extern const pcu_ops_t pcu_fpu_ops;
#endif

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
#ifdef FPE
	pcu_load(&pcu_fpu_ops);
#endif
}

static inline void
fpu_save(lwp_t *l)
{
#ifdef FPE
	pcu_save(&pcu_fpu_ops, l);
#endif
}

static inline void
fpu_discard(lwp_t *l)
{
#ifdef FPE
	pcu_discard(&pcu_fpu_ops, l, false);
#endif
}

static inline void
fpu_replace(lwp_t *l)
{
#ifdef FPE
	pcu_discard(&pcu_fpu_ops, l, true);
#endif
}

static inline bool
fpu_valid_p(lwp_t *l)
{
#ifdef FPE
	return pcu_valid_p(&pcu_fpu_ops, l);
#else
	return false;
#endif
}

void	__syncicache(const void *, size_t);

int	cpu_set_onfault(struct faultbuf *, register_t) __returns_twice;
void	cpu_jump_onfault(struct trapframe *, const struct faultbuf *);

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
	    register_t /*status*/, register_t /*cause*/);
void	cpu_trap(struct trapframe */*tf*/, register_t /*epc*/,
	    register_t /*status*/, register_t /*cause*/,
	    register_t /*badvaddr*/);
void	cpu_ast(struct trapframe *);
void	cpu_fast_switchto(struct lwp *, int);

void	lwp_trampoline(void);

void *	cpu_sendsig_getframe(struct lwp *, int, bool *);

#endif

#endif /* _RISCV_LOCORE_H_ */
