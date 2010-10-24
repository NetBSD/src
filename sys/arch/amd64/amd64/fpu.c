/*	$NetBSD: fpu.c,v 1.30.4.3 2010/10/24 22:47:51 jym Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.  All
 * rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)npx.c	7.2 (Berkeley) 5/12/91
 */

/*-
 * Copyright (c) 1994, 1995, 1998 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)npx.c	7.2 (Berkeley) 5/12/91
 */

/*
 * XXXfvdl update copyright notice. this started out as a stripped isa/npx.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.30.4.3 2010/10/24 22:47:51 jym Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/vmmeter.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/cpufunc.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include <machine/specialreg.h>
#include <machine/fpu.h>

#ifndef XEN
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif

#ifdef XEN
#define clts() HYPERVISOR_fpu_taskswitch(0)
#define stts() HYPERVISOR_fpu_taskswitch(1)
#endif


/*
 * We do lazy initialization and switching using the TS bit in cr0 and the
 * MDP_USEDFPU bit in mdproc.
 *
 * DNA exceptions are handled like this:
 *
 * 1) If there is no FPU, return and go to the emulator.
 * 2) If someone else has used the FPU, save its state into that lwp's PCB.
 * 3a) If MDP_USEDFPU is not set, set it and initialize the FPU.
 * 3b) Otherwise, reload the lwp's previous FPU state.
 *
 * When a lwp is created or exec()s, its saved cr0 image has the TS bit
 * set and the MDP_USEDFPU bit clear.  The MDP_USEDFPU bit is set when the
 * lwp first gets a DNA and the FPU is initialized.  The TS bit is turned
 * off when the FPU is used, and turned on again later when the lwp's FPU
 * state is saved.
 */

void fpudna(struct cpu_info *);
static int x86fpflags_to_ksiginfo(uint32_t);

#ifndef XEN
/*
 * Init the FPU.
 */
void
fpuinit(struct cpu_info *ci)
{
	lcr0(rcr0() & ~(CR0_EM|CR0_TS));
	fninit();
	lcr0(rcr0() | (CR0_TS));
}
#endif

/*
 * Record the FPU state and reinitialize it all except for the control word.
 * Then generate a SIGFPE.
 *
 * Reinitializing the state allows naive SIGFPE handlers to longjmp without
 * doing any fixups.
 */

void
fputrap(struct trapframe *frame)
{
	register struct lwp *l = curcpu()->ci_fpcurlwp;
	struct pcb *pcb = lwp_getpcb(l);
	struct savefpu *sfp = &pcb->pcb_savefpu;
	uint32_t mxcsr, statbits;
	uint16_t cw;
	ksiginfo_t ksi;

	KPREEMPT_DISABLE(l);
	x86_enable_intr();

	/*
	 * At this point, fpcurlwp should be curlwp.  If it wasn't, the TS bit
	 * should be set, and we should have gotten a DNA exception.
	 */
	KASSERT(l == curlwp);
	fxsave(sfp);
	if (frame->tf_trapno == T_XMM) {
		mxcsr = sfp->fp_fxsave.fx_mxcsr;
		statbits = mxcsr;
		mxcsr &= ~0x3f;
		x86_ldmxcsr(&mxcsr);
	} else {
		fninit();
		fwait();
		cw = sfp->fp_fxsave.fx_fcw;
		fldcw(&cw);
		fwait();
		statbits = sfp->fp_fxsave.fx_fsw;
	}
	KPREEMPT_ENABLE(l);

	sfp->fp_ex_tw = sfp->fp_fxsave.fx_ftw;
	sfp->fp_ex_sw = sfp->fp_fxsave.fx_fsw;
	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGFPE;
	ksi.ksi_addr = (void *)frame->tf_rip;
	ksi.ksi_code = x86fpflags_to_ksiginfo(statbits);
	ksi.ksi_trap = statbits;
	(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
}

static int
x86fpflags_to_ksiginfo(uint32_t flags)
{
	int i;
	static int x86fp_ksiginfo_table[] = {
		FPE_FLTINV, /* bit 0 - invalid operation */
		FPE_FLTRES, /* bit 1 - denormal operand */
		FPE_FLTDIV, /* bit 2 - divide by zero	*/
		FPE_FLTOVF, /* bit 3 - fp overflow	*/
		FPE_FLTUND, /* bit 4 - fp underflow	*/
		FPE_FLTRES, /* bit 5 - fp precision	*/
		FPE_FLTINV, /* bit 6 - stack fault	*/
	};

	for (i=0;i < sizeof(x86fp_ksiginfo_table)/sizeof(int); i++) {
		if (flags & (1 << i))
			return (x86fp_ksiginfo_table[i]);
	}
	/* punt if flags not set */
	return (FPE_FLTINV);
}

/*
 * Implement device not available (DNA) exception
 *
 * If we were the last lwp to use the FPU, we can simply return.
 * Otherwise, we save the previous state, if necessary, and restore
 * our last saved state.
 */
void
fpudna(struct cpu_info *ci)
{
	uint16_t cw;
	uint32_t mxcsr;
	struct lwp *l, *fl;
	struct pcb *pcb;
	int s;

	if (ci->ci_fpsaving) {
		/* Recursive trap. */
		x86_enable_intr();
		return;
	}

	/* Lock out IPIs and disable preemption. */
	s = splhigh();
	x86_enable_intr();

	/* Save state on current CPU. */
	l = ci->ci_curlwp;
	pcb = lwp_getpcb(l);
	fl = ci->ci_fpcurlwp;
	if (fl != NULL) {
		/*
		 * It seems we can get here on Xen even if we didn't
		 * switch lwp.  In this case do nothing
		 */
		if (fl == l) {
			KASSERT(pcb->pcb_fpcpu == ci);
			clts();
			splx(s);
			return;
		}
		KASSERT(fl != l);
		fpusave_cpu(true);
		KASSERT(ci->ci_fpcurlwp == NULL);
	}

	/* Save our state if on a remote CPU. */
	if (pcb->pcb_fpcpu != NULL) {
		/* Explicitly disable preemption before dropping spl. */
		KPREEMPT_DISABLE(l);
		splx(s);
		fpusave_lwp(l, true);
		KASSERT(pcb->pcb_fpcpu == NULL);
		s = splhigh();
		KPREEMPT_ENABLE(l);
	}

	/*
	 * Restore state on this CPU, or initialize.  Ensure that
	 * the entire update is atomic with respect to FPU-sync IPIs.
	 */
	clts();
	ci->ci_fpcurlwp = l;
	pcb->pcb_fpcpu = ci;
	if ((l->l_md.md_flags & MDP_USEDFPU) == 0) {
		fninit();
		cw = pcb->pcb_savefpu.fp_fxsave.fx_fcw;
		fldcw(&cw);
		mxcsr = pcb->pcb_savefpu.fp_fxsave.fx_mxcsr;
		x86_ldmxcsr(&mxcsr);
		l->l_md.md_flags |= MDP_USEDFPU;
	} else {
		/*
		 * AMD FPU's do not restore FIP, FDP, and FOP on fxrstor,
		 * leaking other process's execution history. Clear them
		 * manually.
		 */
		static const double zero = 0.0;
		int status;
		/*
		 * Clear the ES bit in the x87 status word if it is currently
		 * set, in order to avoid causing a fault in the upcoming load.
		 */
		fnstsw(&status);
		if (status & 0x80)
			fnclex();
		/*
		 * Load the dummy variable into the x87 stack.  This mangles
		 * the x87 stack, but we don't care since we're about to call
		 * fxrstor() anyway.
		 */
		fldummy(&zero);
		fxrstor(&pcb->pcb_savefpu);
	}

	KASSERT(ci == curcpu());
	splx(s);
}

/*
 * Save current CPU's FPU state.  Must be called at IPL_HIGH.
 */
void
fpusave_cpu(bool save)
{
	struct cpu_info *ci;
	struct pcb *pcb;
	struct lwp *l;

	KASSERT(curcpu()->ci_ilevel == IPL_HIGH);

	ci = curcpu();
	l = ci->ci_fpcurlwp;
	if (l == NULL) {
		return;
	}
	pcb = lwp_getpcb(l);

	if (save) {
		 /*
		  * Set ci->ci_fpsaving, so that any pending exception will
		  * be thrown away.  It will be caught again if/when the
		  * FPU state is restored.
		  */
		KASSERT(ci->ci_fpsaving == 0);
		clts();
		ci->ci_fpsaving = 1;
		fxsave(&pcb->pcb_savefpu);
		ci->ci_fpsaving = 0;
	}

	stts();
	pcb->pcb_fpcpu = NULL;
	ci->ci_fpcurlwp = NULL;
}

/*
 * Save l's FPU state, which may be on this processor or another processor.
 * It may take some time, so we avoid disabling preemption where possible.
 * Caller must know that the target LWP is stopped, otherwise this routine
 * may race against it.
 */
void
fpusave_lwp(struct lwp *l, bool save)
{
	struct cpu_info *oci;
	struct pcb *pcb;
	int s, spins, ticks;

	spins = 0;
	ticks = hardclock_ticks;
	for (;;) {
		s = splhigh();
		pcb = lwp_getpcb(l);
		oci = pcb->pcb_fpcpu;
		if (oci == NULL) {
			splx(s);
			break;
		}
		if (oci == curcpu()) {
			KASSERT(oci->ci_fpcurlwp == l);
			fpusave_cpu(save);
			splx(s);
			break;
		}
		splx(s);
		x86_send_ipi(oci, X86_IPI_SYNCH_FPU);
		while (pcb->pcb_fpcpu == oci && ticks == hardclock_ticks) {
			x86_pause();
			spins++;
		}
		if (spins > 100000000) {
			panic("fpusave_lwp: did not");
		}
	}

	if (!save) {
		/* Ensure we restart with a clean slate. */
	 	l->l_md.md_flags &= ~MDP_USEDFPU;
	}
}
