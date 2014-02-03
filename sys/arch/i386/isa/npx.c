/*	$NetBSD: npx.c,v 1.151 2014/02/03 23:00:32 dsl Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: npx.c,v 1.151 2014/02/03 23:00:32 dsl Exp $");

#if 0
#define IPRINTF(x)	printf x
#else
#define	IPRINTF(x)
#endif

#include "opt_multiprocessor.h"
#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/cpu.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/pcb.h>
#include <machine/specialreg.h>


/*
 * 387 Numeric Coprocessor Extension (NPX) Driver.
 *
 * Note that we only support 486 and later cpus, these do not support
 * an external fpu, only the one that is part of the cpu fabric.
 * A 486 might not have an fpu, but the 487 is a full 486.
 *
 * Since we set CR0.NE a FP exception can only happen when a user process
 * executes a FP instruction. The DNA exception takes precedence, so
 * the execption can only happen when the lwp already owns the fpu.
 * In particular the exceptions won't happen in-kernel while saving state.
 *
 * We do lazy initialization and switching using the TS bit in cr0 and the
 * MDL_USEDFPU bit in mdlwp.
 *
 * DNA exceptions are handled like this:
 *
 * 1) If there is no NPX, return and go to the emulator.
 * 2) If someone else has used the NPX, save its state into that process's PCB.
 * 3a) If MDL_USEDFPU is not set, set it and initialize the NPX.
 * 3b) Otherwise, reload the process's previous NPX state.
 *
 * When a process is created or exec()s, its saved cr0 image has the TS bit
 * set and the MDL_USEDFPU bit clear.  The MDL_USEDFPU bit is set when the
 * process first gets a DNA and the NPX is initialized.  The TS bit is turned
 * off when the NPX is used, and turned on again later when the process's NPX
 * state is saved.
 */

static int	x86fpflags_to_ksiginfo(uint32_t flags);
/* Called directly from i386_trap.S */
int	fpudna(struct cpu_info *);

#ifdef XEN
#define	clts() HYPERVISOR_fpu_taskswitch(0)
#define	stts() HYPERVISOR_fpu_taskswitch(1)
#endif

volatile u_int			npx_intrs_while_probing;
volatile u_int			npx_traps_while_probing;

extern int i386_fpu_present;
extern int i386_fpu_fdivbug;

struct npx_softc		*npx_softc;

#ifndef XEN
/* Initialise fpu, might be boot cpu or a later cpu coming online */
void
fpuinit(struct cpu_info *ci)
{
	uint16_t control;

	/* The default cr0 has the fpu enabled */
	clts();
	fninit();

	/* Read the default control word */
	fnstcw(&control);

	if (control != __INITIAL_NPXCW__) {
		/* Must be a 486SX, trap FP instructions */
		lcr0((rcr0() & ~CR0_MP) | CR0_EM);
		aprint_normal_dev(ci->ci_dev, "no fpu (control %x)\n", control);
		i386_fpu_present = 0;
		return;
	}

	if (npx586bug1(4195835, 3145727) != 0) {
		/* NB 120+MHz cpus are not affected */
		i386_fpu_fdivbug = 1;
		aprint_normal_dev(ci->ci_dev,
		    "WARNING: Pentium FDIV bug detected!\n");
	}

	/* Set TS so first fp instruction faults */
	stts();
}
#endif

/*
 * Record the FPU state and reinitialize it all except for the control word.
 * Then generate a SIGFPE.
 *
 * Reinitializing the state allows naive SIGFPE handlers to longjmp without
 * doing any fixups.
 *
 * XXX there is currently no way to pass the full error state to signal
 * handlers, and if this is a nested interrupt there is no way to pass even
 * a status code!  So there is no way to have a non-naive SIGFPE handler.  At
 * best a handler could do an fninit followed by an fldcw of a static value.
 * fnclex would be of little use because it would leave junk on the FPU stack.
 *
 * Only called directly from i386_trap.S (with interrupts disabled)
 *
 * Since we have CR0.NE set this can only happen as a result of
 * executing an FP instruction (and after CR0.TS has been checked).
 * This means this must be from userspace on the current lwp.
 */
int npxintr(void *, struct intrframe *);
int
npxintr(void *arg, struct intrframe *frame)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l = ci->ci_fpcurlwp;
	union savefpu *addr;
	struct pcb *pcb;
	uint32_t statbits;
	ksiginfo_t ksi;

	if (!USERMODE(frame->if_cs, frame->if_eflags))
		panic("fpu trap from kernel\n");

	kpreempt_disable();
#ifndef XEN
	KASSERT((x86_read_psl() & PSL_I) == 0);
	x86_enable_intr();
#endif

	IPRINTF(("%s: fp intr\n", device_xname(ci->ci_dev)));

	/*
	 * At this point, fpcurlwp should be curlwp.  If it wasn't, the TS
	 * bit should be set, and we should have gotten a DNA exception.
	 */
	if (l != curlwp || !i386_fpu_present) {
		printf("npxintr: l = %p, curproc = %p, fpu_present = %d\n",
		    l, curproc, i386_fpu_present);
		printf("npxintr: came from nowhere");
		kpreempt_enable();
		return 1;
	}

	/* Find the address of fpcurproc's saved FPU state. */
	pcb = lwp_getpcb(l);
	addr = &pcb->pcb_savefpu;

	/*
	 * XXX: (dsl) I think this is all borked.
	 * We need to save the status word (which contains the cause)
	 * of the fault, and clear the relevant error bits so that
	 * the fp instruction doesn't trap again when the signal handler
	 * returns (or if the signal is ignored).
	 * What the code actually does is to reset the FPU, this clears
	 * the FP stack pointer so I suspect the code then gets the
	 * wrong register values for an later operations.
	 * Any code that enabled the stack under/overflow traps is doomed.
	 *
	 * I think this code should just save the status word and clear
	 * the pending errors.
	 * If the signal is generated then the FP state can be saved in
	 * the context stashed on the user stack, and restrored from
	 * there later. So this code need not do a fsave or finit.
	 */
	if (i386_use_fxsave) {
		fxsave(&addr->sv_xmm);
		/* FXSAVE doesn't FNINIT like FNSAVE does -- so do it here. */
		fninit();
		fwait();
		fldcw(&addr->sv_xmm.fx_cw);
		/*
		 * FNINIT doesn't affect MXCSR or the XMM registers;
		 * no need to re-load MXCSR here.
		 */
		statbits = addr->sv_xmm.fx_sw;
	} else {
		fnsave(&addr->sv_87);
		/* fnsave has done an fninit() */
		fwait();
		fldcw(&addr->sv_87.s87_cw);
		statbits = addr->sv_87.s87_sw;
	}
	fwait();

	/*
	 * Pass exception to process.
	 */

	/*
	 * Interrupt is essentially a trap, so we can afford to call
	 * the SIGFPE handler (if any) as soon as the interrupt
	 * returns.
	 *
	 * XXX little or nothing is gained from this, and plenty is
	 * lost - the interrupt frame has to contain the trap frame
	 * (this is otherwise only necessary for the rescheduling trap
	 * in doreti, and the frame for that could easily be set up
	 * just before it is used).
	 */
	l->l_md.md_regs = (struct trapframe *)&frame->if_gs;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGFPE;
	ksi.ksi_addr = (void *)frame->if_eip;

	/*
	 * Encode the appropriate code for detailed information on
	 * this exception.
	 */

	ksi.ksi_code = x86fpflags_to_ksiginfo(statbits);
	ksi.ksi_trap = statbits;

	trapsignal(l, &ksi);

	kpreempt_enable();
	return (1);
}

/* map x86 fp flags to ksiginfo fp codes 		*/
/* see table 8-4 of the IA-32 Intel Architecture	*/
/* Software Developer's Manual, Volume 1		*/
/* XXX punting on the stack fault with FLTINV		*/
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
					     
	for(i=0;i < sizeof(x86fp_ksiginfo_table)/sizeof(int); i++) {
		if (flags & (1 << i))
			return(x86fp_ksiginfo_table[i]);
	}
	/* punt if flags not set */
	return(0);
}

/*
 * Implement device not available (DNA) exception
 *
 * If we were the last lwp to use the FPU, we can simply return.
 * Otherwise, we save the previous state, if necessary, and restore
 * our last saved state.
 */
int
fpudna(struct cpu_info *ci)
{
	struct lwp *l, *fl;
	struct pcb *pcb;
	int s;

	if (ci->ci_fpsaving) {
		/* Recursive trap. */
		return 1;
	}

	/* Lock out IPIs and disable preemption. */
	s = splhigh();
#ifndef XEN
	x86_enable_intr();
#endif
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
			ci->ci_fpused = 1;
			clts();
			splx(s);
			return 1;
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
	ci->ci_fpused = 1;

	if ((l->l_md.md_flags & MDL_USEDFPU) == 0) {
		fninit();
		if (i386_use_fxsave) {
			fldcw(&pcb->pcb_savefpu.sv_xmm.fx_cw);
		} else {
			fldcw(&pcb->pcb_savefpu.sv_87.s87_cw);
		}
		l->l_md.md_flags |= MDL_USEDFPU;
	} else if (i386_use_fxsave) {
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
		fxrstor(&pcb->pcb_savefpu.sv_xmm);
	} else {
		frstor(&pcb->pcb_savefpu.sv_87);
	}

	KASSERT(ci == curcpu());
	splx(s);
	return 1;
}

/*
 * Save current CPU's FPU state.  Must be called at IPL_HIGH.
 */
void
fpusave_cpu(bool save)
{
	struct cpu_info *ci;
	struct lwp *l;
	struct pcb *pcb;

	KASSERT(curcpu()->ci_ilevel == IPL_HIGH);

	ci = curcpu();
	l = ci->ci_fpcurlwp;
	if (l == NULL)
		return;

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
		if (i386_use_fxsave) {
			fxsave(&pcb->pcb_savefpu.sv_xmm);
		} else {
			fnsave(&pcb->pcb_savefpu.sv_87);
		}
		ci->ci_fpsaving = 0;
	}

	stts();
	pcb->pcb_fpcpu = NULL;
	ci->ci_fpcurlwp = NULL;
	ci->ci_fpused = 1;
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
#ifdef XEN
		if (xen_send_ipi(oci, XEN_IPI_SYNCH_FPU) != 0) {
			panic("xen_send_ipi(%s, XEN_IPI_SYNCH_FPU) failed.",
			    cpu_name(oci));
		}
#else /* XEN */
		x86_send_ipi(oci, X86_IPI_SYNCH_FPU);
#endif
		while (pcb->pcb_fpcpu == oci &&
		    ticks == hardclock_ticks) {
			x86_pause();
			spins++;
		}
		if (spins > 100000000) {
			panic("fpusave_lwp: did not");
		}
	}

	if (!save) {
		/* Ensure we restart with a clean slate. */
	 	l->l_md.md_flags &= ~MDL_USEDFPU;
	}
}

/* 
 * The following mechanism is used to ensure that the FPE_... value
 * that is passed as a trapcode to the signal handler of the user
 * process does not have more than one bit set.
 * 
 * Multiple bits may be set if the user process modifies the control
 * word while a status word bit is already set.  While this is a sign
 * of bad coding, we have no choise than to narrow them down to one
 * bit, since we must not send a trapcode that is not exactly one of
 * the FPE_ macros.
 *
 * The mechanism has a static table with 127 entries.  Each combination
 * of the 7 FPU status word exception bits directly translates to a
 * position in this table, where a single FPE_... value is stored.
 * This FPE_... value stored there is considered the "most important"
 * of the exception bits and will be sent as the signal code.  The
 * precedence of the bits is based upon Intel Document "Numerical
 * Applications", Chapter "Special Computational Situations".
 *
 * The macro to choose one of these values does these steps: 1) Throw
 * away status word bits that cannot be masked.  2) Throw away the bits
 * currently masked in the control word, assuming the user isn't
 * interested in them anymore.  3) Reinsert status word bit 7 (stack
 * fault) if it is set, which cannot be masked but must be presered.
 * 4) Use the remaining bits to point into the trapcode table.
 *
 * The 6 maskable bits in order of their preference, as stated in the
 * above referenced Intel manual:
 * 1  Invalid operation (FP_X_INV)
 * 1a   Stack underflow
 * 1b   Stack overflow
 * 1c   Operand of unsupported format
 * 1d   SNaN operand.
 * 2  QNaN operand (not an exception, irrelavant here)
 * 3  Any other invalid-operation not mentioned above or zero divide
 *      (FP_X_INV, FP_X_DZ)
 * 4  Denormal operand (FP_X_DNML)
 * 5  Numeric over/underflow (FP_X_OFL, FP_X_UFL)
 * 6  Inexact result (FP_X_IMP) 
 */
static const uint8_t fpetable[128] = {
	0,
	FPE_FLTINV,	/*  1 - INV */
	FPE_FLTUND,	/*  2 - DNML */
	FPE_FLTINV,	/*  3 - INV | DNML */
	FPE_FLTDIV,	/*  4 - DZ */
	FPE_FLTINV,	/*  5 - INV | DZ */
	FPE_FLTDIV,	/*  6 - DNML | DZ */
	FPE_FLTINV,	/*  7 - INV | DNML | DZ */
	FPE_FLTOVF,	/*  8 - OFL */
	FPE_FLTINV,	/*  9 - INV | OFL */
	FPE_FLTUND,	/*  A - DNML | OFL */
	FPE_FLTINV,	/*  B - INV | DNML | OFL */
	FPE_FLTDIV,	/*  C - DZ | OFL */
	FPE_FLTINV,	/*  D - INV | DZ | OFL */
	FPE_FLTDIV,	/*  E - DNML | DZ | OFL */
	FPE_FLTINV,	/*  F - INV | DNML | DZ | OFL */
	FPE_FLTUND,	/* 10 - UFL */
	FPE_FLTINV,	/* 11 - INV | UFL */
	FPE_FLTUND,	/* 12 - DNML | UFL */
	FPE_FLTINV,	/* 13 - INV | DNML | UFL */
	FPE_FLTDIV,	/* 14 - DZ | UFL */
	FPE_FLTINV,	/* 15 - INV | DZ | UFL */
	FPE_FLTDIV,	/* 16 - DNML | DZ | UFL */
	FPE_FLTINV,	/* 17 - INV | DNML | DZ | UFL */
	FPE_FLTOVF,	/* 18 - OFL | UFL */
	FPE_FLTINV,	/* 19 - INV | OFL | UFL */
	FPE_FLTUND,	/* 1A - DNML | OFL | UFL */
	FPE_FLTINV,	/* 1B - INV | DNML | OFL | UFL */
	FPE_FLTDIV,	/* 1C - DZ | OFL | UFL */
	FPE_FLTINV,	/* 1D - INV | DZ | OFL | UFL */
	FPE_FLTDIV,	/* 1E - DNML | DZ | OFL | UFL */
	FPE_FLTINV,	/* 1F - INV | DNML | DZ | OFL | UFL */
	FPE_FLTRES,	/* 20 - IMP */
	FPE_FLTINV,	/* 21 - INV | IMP */
	FPE_FLTUND,	/* 22 - DNML | IMP */
	FPE_FLTINV,	/* 23 - INV | DNML | IMP */
	FPE_FLTDIV,	/* 24 - DZ | IMP */
	FPE_FLTINV,	/* 25 - INV | DZ | IMP */
	FPE_FLTDIV,	/* 26 - DNML | DZ | IMP */
	FPE_FLTINV,	/* 27 - INV | DNML | DZ | IMP */
	FPE_FLTOVF,	/* 28 - OFL | IMP */
	FPE_FLTINV,	/* 29 - INV | OFL | IMP */
	FPE_FLTUND,	/* 2A - DNML | OFL | IMP */
	FPE_FLTINV,	/* 2B - INV | DNML | OFL | IMP */
	FPE_FLTDIV,	/* 2C - DZ | OFL | IMP */
	FPE_FLTINV,	/* 2D - INV | DZ | OFL | IMP */
	FPE_FLTDIV,	/* 2E - DNML | DZ | OFL | IMP */
	FPE_FLTINV,	/* 2F - INV | DNML | DZ | OFL | IMP */
	FPE_FLTUND,	/* 30 - UFL | IMP */
	FPE_FLTINV,	/* 31 - INV | UFL | IMP */
	FPE_FLTUND,	/* 32 - DNML | UFL | IMP */
	FPE_FLTINV,	/* 33 - INV | DNML | UFL | IMP */
	FPE_FLTDIV,	/* 34 - DZ | UFL | IMP */
	FPE_FLTINV,	/* 35 - INV | DZ | UFL | IMP */
	FPE_FLTDIV,	/* 36 - DNML | DZ | UFL | IMP */
	FPE_FLTINV,	/* 37 - INV | DNML | DZ | UFL | IMP */
	FPE_FLTOVF,	/* 38 - OFL | UFL | IMP */
	FPE_FLTINV,	/* 39 - INV | OFL | UFL | IMP */
	FPE_FLTUND,	/* 3A - DNML | OFL | UFL | IMP */
	FPE_FLTINV,	/* 3B - INV | DNML | OFL | UFL | IMP */
	FPE_FLTDIV,	/* 3C - DZ | OFL | UFL | IMP */
	FPE_FLTINV,	/* 3D - INV | DZ | OFL | UFL | IMP */
	FPE_FLTDIV,	/* 3E - DNML | DZ | OFL | UFL | IMP */
	FPE_FLTINV,	/* 3F - INV | DNML | DZ | OFL | UFL | IMP */
	FPE_FLTSUB,	/* 40 - STK */
	FPE_FLTSUB,	/* 41 - INV | STK */
	FPE_FLTUND,	/* 42 - DNML | STK */
	FPE_FLTSUB,	/* 43 - INV | DNML | STK */
	FPE_FLTDIV,	/* 44 - DZ | STK */
	FPE_FLTSUB,	/* 45 - INV | DZ | STK */
	FPE_FLTDIV,	/* 46 - DNML | DZ | STK */
	FPE_FLTSUB,	/* 47 - INV | DNML | DZ | STK */
	FPE_FLTOVF,	/* 48 - OFL | STK */
	FPE_FLTSUB,	/* 49 - INV | OFL | STK */
	FPE_FLTUND,	/* 4A - DNML | OFL | STK */
	FPE_FLTSUB,	/* 4B - INV | DNML | OFL | STK */
	FPE_FLTDIV,	/* 4C - DZ | OFL | STK */
	FPE_FLTSUB,	/* 4D - INV | DZ | OFL | STK */
	FPE_FLTDIV,	/* 4E - DNML | DZ | OFL | STK */
	FPE_FLTSUB,	/* 4F - INV | DNML | DZ | OFL | STK */
	FPE_FLTUND,	/* 50 - UFL | STK */
	FPE_FLTSUB,	/* 51 - INV | UFL | STK */
	FPE_FLTUND,	/* 52 - DNML | UFL | STK */
	FPE_FLTSUB,	/* 53 - INV | DNML | UFL | STK */
	FPE_FLTDIV,	/* 54 - DZ | UFL | STK */
	FPE_FLTSUB,	/* 55 - INV | DZ | UFL | STK */
	FPE_FLTDIV,	/* 56 - DNML | DZ | UFL | STK */
	FPE_FLTSUB,	/* 57 - INV | DNML | DZ | UFL | STK */
	FPE_FLTOVF,	/* 58 - OFL | UFL | STK */
	FPE_FLTSUB,	/* 59 - INV | OFL | UFL | STK */
	FPE_FLTUND,	/* 5A - DNML | OFL | UFL | STK */
	FPE_FLTSUB,	/* 5B - INV | DNML | OFL | UFL | STK */
	FPE_FLTDIV,	/* 5C - DZ | OFL | UFL | STK */
	FPE_FLTSUB,	/* 5D - INV | DZ | OFL | UFL | STK */
	FPE_FLTDIV,	/* 5E - DNML | DZ | OFL | UFL | STK */
	FPE_FLTSUB,	/* 5F - INV | DNML | DZ | OFL | UFL | STK */
	FPE_FLTRES,	/* 60 - IMP | STK */
	FPE_FLTSUB,	/* 61 - INV | IMP | STK */
	FPE_FLTUND,	/* 62 - DNML | IMP | STK */
	FPE_FLTSUB,	/* 63 - INV | DNML | IMP | STK */
	FPE_FLTDIV,	/* 64 - DZ | IMP | STK */
	FPE_FLTSUB,	/* 65 - INV | DZ | IMP | STK */
	FPE_FLTDIV,	/* 66 - DNML | DZ | IMP | STK */
	FPE_FLTSUB,	/* 67 - INV | DNML | DZ | IMP | STK */
	FPE_FLTOVF,	/* 68 - OFL | IMP | STK */
	FPE_FLTSUB,	/* 69 - INV | OFL | IMP | STK */
	FPE_FLTUND,	/* 6A - DNML | OFL | IMP | STK */
	FPE_FLTSUB,	/* 6B - INV | DNML | OFL | IMP | STK */
	FPE_FLTDIV,	/* 6C - DZ | OFL | IMP | STK */
	FPE_FLTSUB,	/* 6D - INV | DZ | OFL | IMP | STK */
	FPE_FLTDIV,	/* 6E - DNML | DZ | OFL | IMP | STK */
	FPE_FLTSUB,	/* 6F - INV | DNML | DZ | OFL | IMP | STK */
	FPE_FLTUND,	/* 70 - UFL | IMP | STK */
	FPE_FLTSUB,	/* 71 - INV | UFL | IMP | STK */
	FPE_FLTUND,	/* 72 - DNML | UFL | IMP | STK */
	FPE_FLTSUB,	/* 73 - INV | DNML | UFL | IMP | STK */
	FPE_FLTDIV,	/* 74 - DZ | UFL | IMP | STK */
	FPE_FLTSUB,	/* 75 - INV | DZ | UFL | IMP | STK */
	FPE_FLTDIV,	/* 76 - DNML | DZ | UFL | IMP | STK */
	FPE_FLTSUB,	/* 77 - INV | DNML | DZ | UFL | IMP | STK */
	FPE_FLTOVF,	/* 78 - OFL | UFL | IMP | STK */
	FPE_FLTSUB,	/* 79 - INV | OFL | UFL | IMP | STK */
	FPE_FLTUND,	/* 7A - DNML | OFL | UFL | IMP | STK */
	FPE_FLTSUB,	/* 7B - INV | DNML | OFL | UFL | IMP | STK */
	FPE_FLTDIV,	/* 7C - DZ | OFL | UFL | IMP | STK */
	FPE_FLTSUB,	/* 7D - INV | DZ | OFL | UFL | IMP | STK */
	FPE_FLTDIV,	/* 7E - DNML | DZ | OFL | UFL | IMP | STK */
	FPE_FLTSUB,	/* 7F - INV | DNML | DZ | OFL | UFL | IMP | STK */
};

/*
 * Preserve the FP status word, clear FP exceptions, then generate a SIGFPE.
 *
 * Clearing exceptions is necessary mainly to avoid IRQ13 bugs.  We now
 * depend on longjmp() restoring a usable state.  Restoring the state
 * or examining it might fail if we didn't clear exceptions.
 *
 * The error code chosen will be one of the FPE_... macros. It will be
 * sent as the second argument to old BSD-style signal handlers and as
 * "siginfo_t->si_code" (second argument) to SA_SIGINFO signal handlers.
 *
 * XXX the FP state is not preserved across signal handlers.  So signal
 * handlers cannot afford to do FP unless they preserve the state or
 * longjmp() out.  Both preserving the state and longjmp()ing may be
 * destroyed by IRQ13 bugs.  Clearing FP exceptions is not an acceptable
 * solution for signals other than SIGFPE.
 */
int
npxtrap(struct lwp *l)
{
	u_short control, status;
	struct cpu_info *ci = curcpu();
	struct lwp *fl = ci->ci_fpcurlwp;

	if (!i386_fpu_present) {
		printf("%s: fpcurthread = %p, curthread = %p\n",
		    __func__, fl, l);
		panic("npxtrap from nowhere");
	}
	kpreempt_disable();

	/*
	 * Interrupt handling (for another interrupt) may have pushed the
	 * state to memory.  Fetch the relevant parts of the state from
	 * wherever they are.
	 */
	if (fl != l) {
		struct pcb *pcb = lwp_getpcb(l);
		if (i386_use_fxsave) {
			control = pcb->pcb_savefpu.sv_xmm.fx_cw;
			status = pcb->pcb_savefpu.sv_xmm.fx_sw;
		} else {
			control = pcb->pcb_savefpu.sv_87.s87_cw;
			status = pcb->pcb_savefpu.sv_87.s87_sw;
		}
	} else {
		fnstcw(&control);
		fnstsw(&status);
	}

	if (fl == l)
		fnclex();
	kpreempt_enable();
	return fpetable[status & ((~control & 0x3f) | 0x40)];
}
