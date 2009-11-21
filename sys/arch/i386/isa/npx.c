/*	$NetBSD: npx.c,v 1.135 2009/11/21 03:11:01 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: npx.c,v 1.135 2009/11/21 03:11:01 rmind Exp $");

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
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/vmmeter.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include <machine/specialreg.h>
#include <machine/pio.h>
#include <machine/i8259.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <i386/isa/npxvar.h>

/*
 * 387 and 287 Numeric Coprocessor Extension (NPX) Driver.
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
static int	npxdna(struct cpu_info *);

#ifdef XEN
#define	clts()
#define	stts()
#endif

static	enum npx_type		npx_type;
volatile u_int			npx_intrs_while_probing;
volatile u_int			npx_traps_while_probing;

extern int i386_fpu_present;
extern int i386_fpu_exception;
extern int i386_fpu_fdivbug;

struct npx_softc		*npx_softc;

static inline void
fpu_save(union savefpu *addr)
{
	if (i386_use_fxsave)
	{
                fxsave(&addr->sv_xmm);

		/* FXSAVE doesn't FNINIT like FNSAVE does -- so do it here. */
		fninit();
	} else
		fnsave(&addr->sv_87);
}

static int
npxdna_empty(struct cpu_info *ci)
{

#ifndef XEN
	panic("npxdna vector not initialized");
#endif
	return 0;
}


int    (*npxdna_func)(struct cpu_info *) = npxdna_empty;

#ifndef XEN
/*
 * This calls i8259_* directly, but currently we can count on systems
 * having a i8259 compatible setup all the time. Maybe have to change
 * that in the future.
 */
enum npx_type
npxprobe1(bus_space_tag_t iot, bus_space_handle_t ioh, int irq)
{
	struct gate_descriptor save_idt_npxintr;
	struct gate_descriptor save_idt_npxtrap;
	enum npx_type rv = NPX_NONE;
	u_long	save_eflags;
	int control;
	int status;
	unsigned irqmask;

	if (cpu_feature & CPUID_FPU) {
		i386_fpu_exception = 1;
		return NPX_CPUID;
	}
	save_eflags = x86_read_psl();
	x86_disable_intr();
	save_idt_npxintr = idt[NRSVIDT + irq];
	save_idt_npxtrap = idt[16];
	setgate(&idt[NRSVIDT + irq], probeintr, 0, SDT_SYS386IGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	setgate(&idt[16], probetrap, 0, SDT_SYS386TGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));

	irqmask = i8259_setmask(~((1 << IRQ_SLAVE) | (1 << irq)));

	/*
	 * Partially reset the coprocessor, if any.  Some BIOS's don't reset
	 * it after a warm boot.
	 */
	/* full reset on some systems, NOP on others */
	bus_space_write_1(iot, ioh, 1, 0);
	delay(1000);
	/* clear BUSY# latch */
	bus_space_write_1(iot, ioh, 0, 0);

	/*
	 * We set CR0 in locore to trap all ESC and WAIT instructions.
	 * We have to turn off the CR0_EM bit temporarily while probing.
	 */
	lcr0(rcr0() & ~(CR0_EM|CR0_TS));
	x86_enable_intr();

	/*
	 * Finish resetting the coprocessor, if any.  If there is an error
	 * pending, then we may get a bogus IRQ13, but probeintr() will handle
	 * it OK.  Bogus halts have never been observed, but we enabled
	 * IRQ13 and cleared the BUSY# latch early to handle them anyway.
	 */
	fninit();
	delay(1000);		/* wait for any IRQ13 (fwait might hang) */

	/*
	 * Check for a status of mostly zero.
	 */
	status = 0x5a5a;
	fnstsw(&status);
	if ((status & 0xb8ff) == 0) {
		/*
		 * Good, now check for a proper control word.
		 */
		control = 0x5a5a;
		fnstcw(&control);
		if ((control & 0x1f3f) == 0x033f) {
			/*
			 * We have an npx, now divide by 0 to see if exception
			 * 16 works.
			 */
			control &= ~(1 << 2);	/* enable divide by 0 trap */
			fldcw(&control);
			npx_traps_while_probing = npx_intrs_while_probing = 0;
			fp_divide_by_0();
			if (npx_traps_while_probing != 0) {
				/*
				 * Good, exception 16 works.
				 */
				rv = NPX_EXCEPTION;
				i386_fpu_exception = 1;
			} else if (npx_intrs_while_probing != 0) {
				/*
				 * Bad, we are stuck with IRQ13.
				 */
				rv = NPX_INTERRUPT;
			} else {
				/*
				 * Worse, even IRQ13 is broken.  Use emulator.
				 */
				rv = NPX_BROKEN;
			}
		}
	}

	x86_disable_intr();
	lcr0(rcr0() | (CR0_EM|CR0_TS));

	irqmask = i8259_setmask(irqmask);

	idt[NRSVIDT + irq] = save_idt_npxintr;

	idt[16] = save_idt_npxtrap;
	x86_write_psl(save_eflags);

	return (rv);
}

void npxinit(struct cpu_info *ci)
{
	lcr0(rcr0() & ~(CR0_EM|CR0_TS));
	fninit();
	if (npx586bug1(4195835, 3145727) != 0) {
		i386_fpu_fdivbug = 1;
		aprint_normal_dev(ci->ci_dev,
		    "WARNING: Pentium FDIV bug detected!\n");
	}
	lcr0(rcr0() | (CR0_TS));
}
#endif

/*
 * Common attach routine.
 */
void
npxattach(struct npx_softc *sc)
{

	npx_softc = sc;
	npx_type = sc->sc_type;

#ifndef XEN
	npxinit(&cpu_info_primary);
#endif
	i386_fpu_present = 1;
	npxdna_func = npxdna;

	if (!pmf_device_register(sc->sc_dev, NULL, NULL))
		aprint_error_dev(sc->sc_dev, "couldn't establish power handler\n");
}

int
npxdetach(device_t self, int flags)
{
	struct npx_softc *sc = device_private(self);

	if (sc->sc_type == NPX_INTERRUPT)
		return EBUSY;

	pmf_device_deregister(self);
	
	return 0;
}

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
 * Returning from the handler would be even less safe than usual because
 * IRQ13 exception handling makes exceptions even less precise than usual.
 */
int
npxintr(void *arg, struct intrframe *frame)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l = ci->ci_fpcurlwp;
	union savefpu *addr;
	struct npx_softc *sc;
	struct pcb *pcb;
	ksiginfo_t ksi;

	sc = npx_softc;

	kpreempt_disable();
#ifndef XEN
	KASSERT((x86_read_psl() & PSL_I) == 0);
	x86_enable_intr();
#endif

	uvmexp.traps++;
	IPRINTF(("%s: fp intr\n", device_xname(ci->ci_dev)));

#ifndef XEN
	/*
	 * Clear the interrupt latch.
	 */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 0, 0);
#endif

	/*
	 * If we're saving, ignore the interrupt.  The FPU will generate
	 * another one when we restore the state later.
	 */
	if (ci->ci_fpsaving) {
		kpreempt_enable();
		return (1);
	}

	if (l == NULL || npx_type == NPX_NONE) {
		printf("npxintr: l = %p, curproc = %p, npx_type = %d\n",
		    l, curproc, npx_type);
		printf("npxintr: came from nowhere");
		kpreempt_enable();
		return 1;
	}

	/*
	 * At this point, fpcurlwp should be curlwp.  If it wasn't, the TS
	 * bit should be set, and we should have gotten a DNA exception.
	 */
	KASSERT(l == curlwp);
	pcb = lwp_getpcb(l);

	/*
	 * Find the address of fpcurproc's saved FPU state.  (Given the
	 * invariant above, this is always the one in curpcb.)
	 */
	addr = &pcb->pcb_savefpu;

	/*
	 * Save state.  This does an implied fninit.  It had better not halt
	 * the CPU or we'll hang.
	 */
	fpu_save(addr);
	fwait();
        if (i386_use_fxsave) {
		fldcw(&addr->sv_xmm.sv_env.en_cw);
		/*
		 * FNINIT doesn't affect MXCSR or the XMM registers;
		 * no need to re-load MXCSR here.
		 */
        } else
                fldcw(&addr->sv_87.sv_env.en_cw);
	fwait();
	/*
	 * Remember the exception status word and tag word.  The current
	 * (almost fninit'ed) fpu state is in the fpu and the exception
	 * state just saved will soon be junk.  However, the implied fninit
	 * doesn't change the error pointers or register contents, and we
	 * preserved the control word and will copy the status and tag
	 * words, so the complete exception state can be recovered.
	 */
        if (i386_use_fxsave) {
		addr->sv_xmm.sv_ex_sw = addr->sv_xmm.sv_env.en_sw;
		addr->sv_xmm.sv_ex_tw = addr->sv_xmm.sv_env.en_tw;
	} else {
		addr->sv_87.sv_ex_sw = addr->sv_87.sv_env.en_sw;
		addr->sv_87.sv_ex_tw = addr->sv_87.sv_env.en_tw;
	}
	/*
	 * Pass exception to process.
	 */
	if (USERMODE(frame->if_cs, frame->if_eflags)) {
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

		if (i386_use_fxsave) {
			ksi.ksi_code =
				x86fpflags_to_ksiginfo(addr->sv_xmm.sv_ex_sw);
			ksi.ksi_trap = (int)addr->sv_xmm.sv_ex_sw;
		} else {
			ksi.ksi_code =
				x86fpflags_to_ksiginfo(addr->sv_87.sv_ex_sw);
			ksi.ksi_trap = (int)addr->sv_87.sv_ex_sw;
		}

		trapsignal(l, &ksi);
	} else {
		/*
		 * This is a nested interrupt.  This should only happen when
		 * an IRQ13 occurs at the same time as a higher-priority
		 * interrupt.
		 *
		 * XXX
		 * Currently, we treat this like an asynchronous interrupt, but
		 * this has disadvantages.
		 */
		psignal(l->l_proc, SIGFPE);
	}

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
static int
npxdna(struct cpu_info *ci)
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
		npxsave_cpu(true);
		KASSERT(ci->ci_fpcurlwp == NULL);
	}

	/* Save our state if on a remote CPU. */
	if (pcb->pcb_fpcpu != NULL) {
		/* Explicitly disable preemption before dropping spl. */
		KPREEMPT_DISABLE(l);
		splx(s);
		npxsave_lwp(l, true);
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
			fldcw(&pcb->pcb_savefpu.
			    sv_xmm.sv_env.en_cw);
		} else {
			fldcw(&pcb->pcb_savefpu.
			    sv_87.sv_env.en_cw);
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
npxsave_cpu(bool save)
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
npxsave_lwp(struct lwp *l, bool save)
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
			npxsave_cpu(save);
			splx(s);
			break;
		}
		splx(s);
		x86_send_ipi(oci, X86_IPI_SYNCH_FPU);
		while (pcb->pcb_fpcpu == oci &&
		    ticks == hardclock_ticks) {
			x86_pause();
			spins++;
		}
		if (spins > 100000000) {
			panic("npxsave_lwp: did not");
		}
	}

	if (!save) {
		/* Ensure we restart with a clean slate. */
	 	l->l_md.md_flags &= ~MDL_USEDFPU;
	}
}
