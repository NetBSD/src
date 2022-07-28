/* $NetBSD: cpu_machdep.c,v 1.13 2022/07/28 09:14:12 riastradh Exp $ */

/*-
 * Copyright (c) 2014, 2019 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: cpu_machdep.c,v 1.13 2022/07/28 09:14:12 riastradh Exp $");

#include "opt_multiprocessor.h"

#define _INTR_PRIVATE

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/xcall.h>

#include <aarch64/armreg.h>
#include <aarch64/db_machdep.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/pcb.h>
#include <aarch64/userret.h>

#ifdef __HAVE_FAST_SOFTINTS
#if IPL_VM != IPL_SOFTSERIAL + 1
#error IPLs are screwed up
#elif IPL_SOFTSERIAL != IPL_SOFTNET + 1
#error IPLs are screwed up
#elif IPL_SOFTNET != IPL_SOFTBIO + 1
#error IPLs are screwed up
#elif IPL_SOFTBIO != IPL_SOFTCLOCK + 1
#error IPLs are screwed up
#elif !(IPL_SOFTCLOCK > IPL_NONE)
#error IPLs are screwed up
#elif (IPL_NONE != 0)
#error IPLs are screwed up
#endif

#ifndef __HAVE_PIC_FAST_SOFTINTS
#define SOFTINT2IPLMAP \
	(((IPL_SOFTSERIAL - IPL_SOFTCLOCK) << (SOFTINT_SERIAL * 4)) | \
	 ((IPL_SOFTNET    - IPL_SOFTCLOCK) << (SOFTINT_NET    * 4)) | \
	 ((IPL_SOFTBIO    - IPL_SOFTCLOCK) << (SOFTINT_BIO    * 4)) | \
	 ((IPL_SOFTCLOCK  - IPL_SOFTCLOCK) << (SOFTINT_CLOCK  * 4)))
#define SOFTINT2IPL(l)	((SOFTINT2IPLMAP >> ((l) * 4)) & 0x0f)

/*
 * This returns a mask of softint IPLs that be dispatch at <ipl>
 */
#define SOFTIPLMASK(ipl) ((0x0f << (ipl)) & 0x0f)
CTASSERT(SOFTIPLMASK(IPL_NONE)		== 0x0000000f);
CTASSERT(SOFTIPLMASK(IPL_SOFTCLOCK)	== 0x0000000e);
CTASSERT(SOFTIPLMASK(IPL_SOFTBIO)	== 0x0000000c);
CTASSERT(SOFTIPLMASK(IPL_SOFTNET)	== 0x00000008);
CTASSERT(SOFTIPLMASK(IPL_SOFTSERIAL)	== 0x00000000);

void
softint_trigger(uintptr_t mask)
{
	curcpu()->ci_softints |= mask;
}

void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{
	lwp_t ** lp = &l->l_cpu->ci_softlwps[level];
	KASSERT(*lp == NULL || *lp == l);
	*lp = l;
	*machdep = 1 << SOFTINT2IPL(level);
	KASSERT(level != SOFTINT_CLOCK ||
	    *machdep == (1 << (IPL_SOFTCLOCK  - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_BIO ||
	    *machdep == (1 << (IPL_SOFTBIO    - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_NET ||
	    *machdep == (1 << (IPL_SOFTNET    - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_SERIAL ||
	    *machdep == (1 << (IPL_SOFTSERIAL - IPL_SOFTCLOCK)));
}

void
dosoftints(void)
{
	struct cpu_info * const ci = curcpu();
	const int opl = ci->ci_cpl;
	const uint32_t softiplmask = SOFTIPLMASK(opl);
	int s;

	s = splhigh();
	KASSERT(s == opl);
	for (;;) {
		u_int softints = ci->ci_softints & softiplmask;
		KASSERT((softints != 0) == ((ci->ci_softints >> opl) != 0));
		KASSERT(opl == IPL_NONE ||
		    (softints & (1 << (opl - IPL_SOFTCLOCK))) == 0);
		if (softints == 0) {
#ifdef __HAVE_PREEMPTION
			if (ci->ci_want_resched & RESCHED_KPREEMPT) {
				atomic_and_uint(&ci->ci_want_resched,
				    ~RESCHED_KPREEMPT);
				splsched();
				kpreempt(-2);
			}
#endif
			break;
		}
#define DOSOFTINT(n) \
		if (ci->ci_softints & (1 << (IPL_SOFT ## n - IPL_SOFTCLOCK))) {\
			ci->ci_softints &= \
			    ~(1 << (IPL_SOFT ## n - IPL_SOFTCLOCK)); \
			cpu_switchto_softint(ci->ci_softlwps[SOFTINT_ ## n], \
			    IPL_SOFT ## n); \
			continue; \
		}
		DOSOFTINT(SERIAL);
		DOSOFTINT(NET);
		DOSOFTINT(BIO);
		DOSOFTINT(CLOCK);
		panic("dosoftints wtf (softints=%u?, ipl=%d)", softints, opl);
	}
	splx(s);
}
#endif /* !__HAVE_PIC_FAST_SOFTINTS */
#endif /* __HAVE_FAST_SOFTINTS */

int
cpu_mcontext_validate(struct lwp *l, const mcontext_t *mcp)
{
	if ((mcp->__gregs[_REG_SPSR] & ~SPSR_NZCV)
	    || (mcp->__gregs[_REG_PC] & 3)
	    || (mcp->__gregs[_REG_SP] & 15))
		return EINVAL;

	return 0;
}

/*
 * Since the ucontext_t will be on the stack most of the time, make sure
 * it will keep the stack aligned.
 */
CTASSERT(sizeof(ucontext_t) % 16 == 0);

CTASSERT(sizeof(struct reg) == sizeof(__gregset_t));
CTASSERT(offsetof(struct reg, r_pc) == _REG_PC * sizeof(__greg_t));
CTASSERT(offsetof(struct reg, r_sp) == _REG_SP * sizeof(__greg_t));
CTASSERT(offsetof(struct reg, r_spsr) == _REG_SPSR * sizeof(__greg_t));
CTASSERT(offsetof(struct reg, r_tpidr) == _REG_TPIDR * sizeof(__greg_t));

CTASSERT(sizeof(struct fpreg) == sizeof(__fregset_t));
CTASSERT(offsetof(struct fpreg, fpcr) == offsetof(__fregset_t, __fpcr));
CTASSERT(offsetof(struct fpreg, fpsr) == offsetof(__fregset_t, __fpsr));

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flagsp)
{
	const struct trapframe * const tf = lwp_trapframe(l);

	memcpy(mcp->__gregs, &tf->tf_regs, sizeof(mcp->__gregs));
	mcp->__gregs[_REG_TPIDR] = (uintptr_t)l->l_private;
	mcp->__gregs[_REG_SPSR] &= ~SPSR_A64_BTYPE;

	if (fpu_used_p(l)) {
		const struct pcb * const pcb = lwp_getpcb(l);
		fpu_save(l);
		*flagsp |= _UC_FPU;
		mcp->__fregs = *(const __fregset_t *) &pcb->pcb_fpregs;
	}
	*flagsp |= _UC_CPU|_UC_TLSBASE;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct proc * const p = l->l_proc;

	if (flags & _UC_CPU) {
		struct trapframe * const tf = lwp_trapframe(l);
		int error = cpu_mcontext_validate(l, mcp);
		if (error)
			return error;

		memcpy(&tf->tf_regs, mcp->__gregs, sizeof(tf->tf_regs));
	}

	if (flags & _UC_TLSBASE)
		l->l_private = (void *)mcp->__gregs[_REG_TPIDR];

	if (flags & _UC_FPU) {
		struct pcb * const pcb = lwp_getpcb(l);
		fpu_discard(l, true);
		pcb->pcb_fpregs = *(const struct fpreg *)&mcp->__fregs;
	}

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return 0;
}

void
startlwp(void *arg)
{
	ucontext_t * const uc = arg;
	lwp_t * const l = curlwp;
	int error __diagused;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(*uc));
	userret(l);
}

void
cpu_need_resched(struct cpu_info *ci, struct lwp *l, int flags)
{
	KASSERT(kpreempt_disabled());

	if ((flags & RESCHED_KPREEMPT) != 0) {
#ifdef __HAVE_PREEMPTION
		if ((flags & RESCHED_REMOTE) != 0) {
			intr_ipi_send(ci->ci_kcpuset, IPI_KPREEMPT);
		}
#endif
		return;
	}
	if ((flags & RESCHED_REMOTE) != 0) {
#ifdef MULTIPROCESSOR
		intr_ipi_send(ci->ci_kcpuset, IPI_AST);
#endif
	} else {
		l->l_md.md_astpending = 1;
	}
}

void
cpu_need_proftick(struct lwp *l)
{
	KASSERT(kpreempt_disabled());
	KASSERT(l->l_cpu == curcpu());

	l->l_pflag |= LP_OWEUPC;
	l->l_md.md_astpending = 1;
}

void
cpu_signotify(struct lwp *l)
{

	KASSERT(kpreempt_disabled());

	if (l->l_cpu != curcpu()) {
#ifdef MULTIPROCESSOR
		intr_ipi_send(l->l_cpu->ci_kcpuset, IPI_AST);
#endif
	} else {
		l->l_md.md_astpending = 1;
	}
}

#ifdef __HAVE_PREEMPTION
bool
cpu_kpreempt_enter(uintptr_t where, int s)
{
	KASSERT(kpreempt_disabled());

#if 0
	if (where == (intptr_t)-2) {
		KASSERT(curcpu()->ci_mtx_count == 0);
		/*
		 * We must be called via kern_intr (which already checks for
		 * IPL_NONE so of course we call be preempted).
		 */
		return true;
	}
	/*
	 * We are called from KPREEMPT_ENABLE().  If we are at IPL_NONE,
	 * of course we can be preempted.  If we aren't, ask for a
	 * softint so that kern_intr can call kpreempt.
	 */
	if (s == IPL_NONE) {
		KASSERT(curcpu()->ci_mtx_count == 0);
		return true;
	}
	atomic_or_uint(curcpu()->ci_want_resched, RESCHED_KPREEMPT);
#endif
	return false;
}

void
cpu_kpreempt_exit(uintptr_t where)
{

	/* do nothing */
}

/*
 * Return true if preemption is disabled for MD reasons.  Must be called
 * with preemption disabled, and thus is only for diagnostic checks.
 */
bool
cpu_kpreempt_disabled(void)
{
	/*
	 * Any elevated IPL disables preemption.
	 */
	return curcpu()->ci_cpl > IPL_NONE;
}
#endif /* __HAVE_PREEMPTION */

#ifdef MULTIPROCESSOR
void
xc_send_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	intr_ipi_send(ci != NULL ? ci->ci_kcpuset : NULL, IPI_XCALL);
}

void
cpu_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	intr_ipi_send(ci != NULL ? ci->ci_kcpuset : NULL, IPI_GENERIC);
}

int
pic_ipi_shootdown(void *arg)
{
	/* may be populated in pmap.c */
	return 1;
}
#endif /* MULTIPROCESSOR */
