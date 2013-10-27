/*	$NetBSD: fpu.c,v 1.42 2013/10/27 16:25:01 rmind Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.42 2013/10/27 16:25:01 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <sys/bus.h>
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
 * MDL_USEDFPU bit in mdlwp.
 *
 * DNA exceptions are handled like this:
 *
 * 1) If there is no FPU, return and go to the emulator.
 * 2) If someone else has used the FPU, save its state into that lwp's PCB.
 * 3a) If MDL_USEDFPU is not set, set it and initialize the FPU.
 * 3b) Otherwise, reload the lwp's previous FPU state.
 *
 * When a lwp is created or exec()s, its saved cr0 image has the TS bit
 * set and the MDL_USEDFPU bit clear.  The MDL_USEDFPU bit is set when the
 * lwp first gets a DNA and the FPU is initialized.  The TS bit is turned
 * off when the FPU is used, and turned on again later when the lwp's FPU
 * state is saved.
 */

void		fpudna(struct cpu_info *);
static int	x86fpflags_to_ksiginfo(uint32_t);

/*
 * Init the FPU.
 */
void
fpuinit(struct cpu_info *ci)
{
	clts();
	fninit();
	stts();
}

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
	struct lwp *l = curlwp;
	struct pcb *pcb = lwp_getpcb(l);
	struct savefpu *sfp = &pcb->pcb_savefpu;
	uint32_t mxcsr, statbits;
	ksiginfo_t ksi;

	KPREEMPT_DISABLE(l);
	x86_enable_intr();

	fxsave(sfp);
	pcb->pcb_savefpu_i387.fp_ex_tw = sfp->fp_fxsave.fx_ftw;
	pcb->pcb_savefpu_i387.fp_ex_sw = sfp->fp_fxsave.fx_fsw;

	if (frame->tf_trapno == T_XMM) {
		mxcsr = sfp->fp_fxsave.fx_mxcsr;
		statbits = mxcsr;
		mxcsr &= ~0x3f;
		x86_ldmxcsr(&mxcsr);
	} else {
		uint16_t cw;

		fninit();
		fwait();
		cw = sfp->fp_fxsave.fx_fcw;
		fldcw(&cw);
		fwait();
		statbits = sfp->fp_fxsave.fx_fsw;
	}
	KPREEMPT_ENABLE(l);

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
	static int x86fp_ksiginfo_table[] = {
		FPE_FLTINV, /* bit 0 - invalid operation */
		FPE_FLTRES, /* bit 1 - denormal operand */
		FPE_FLTDIV, /* bit 2 - divide by zero	*/
		FPE_FLTOVF, /* bit 3 - fp overflow	*/
		FPE_FLTUND, /* bit 4 - fp underflow	*/
		FPE_FLTRES, /* bit 5 - fp precision	*/
		FPE_FLTINV, /* bit 6 - stack fault	*/
	};

	for (u_int i = 0; i < __arraycount(x86fp_ksiginfo_table); i++) {
		if (flags & (1U << i))
			return x86fp_ksiginfo_table[i];
	}

	/* Punt if flags not set. */
	return FPE_FLTINV;
}

/*
 * Implement device not available (DNA) exception
 *
 * If we were the last lwp to use the FPU, we can simply return.
 * Otherwise, we save the previous state, if necessary, and restore
 * our last saved state.
 */

extern const pcu_ops_t fpu_ops;

void
fpudna(struct cpu_info *ci)
{
	pcu_load(&fpu_ops);
}

static void
fpu_state_load(struct lwp *l, u_int flags)
{
	struct pcb *pcb = lwp_getpcb(l);

	clts();
	pcb->pcb_cr0 &= ~CR0_TS;
	if ((flags & PCU_RELOAD) == 0)
		return;

	if ((flags & PCU_LOADED) == 0) {
		uint32_t mxcsr;
		uint16_t cw;

		fninit();
		cw = pcb->pcb_savefpu.fp_fxsave.fx_fcw;
		fldcw(&cw);
		mxcsr = pcb->pcb_savefpu.fp_fxsave.fx_mxcsr;
		x86_ldmxcsr(&mxcsr);
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
}

static void
fpu_state_save(struct lwp *l, u_int flags)
{
	struct pcb *pcb = lwp_getpcb(l);

	clts();
	fxsave(&pcb->pcb_savefpu);
}

static void
fpu_state_release(struct lwp *l, u_int flags)
{
	struct pcb *pcb = lwp_getpcb(l);

	stts();
	pcb->pcb_cr0 |= CR0_TS;
}

const pcu_ops_t fpu_ops = {
	.pcu_id = PCU_FPU,
	.pcu_state_load = fpu_state_load,
	.pcu_state_save = fpu_state_save,
	.pcu_state_release = fpu_state_release,
};

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
	[PCU_FPU] = &fpu_ops,
};
