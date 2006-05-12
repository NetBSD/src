/*	$NetBSD: npx.c,v 1.3.14.1.2.2 2006/05/12 15:43:56 tron Exp $	*/
/*	NetBSD: npx.c,v 1.103 2004/03/21 10:56:24 simonb Exp 	*/

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
__KERNEL_RCSID(0, "$NetBSD: npx.c,v 1.3.14.1.2.2 2006/05/12 15:43:56 tron Exp $");

#if 0
#define IPRINTF(x)	printf x
#else
#define	IPRINTF(x)
#endif

#include "opt_cputype.h"
#include "opt_multiprocessor.h"
#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/vmmeter.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/cpufunc.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include <machine/specialreg.h>
#include <machine/pio.h>
#include <machine/i8259.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <i386/isa/npxvar.h>

#ifdef XEN
#include <machine/xen.h>
#include <machine/hypervisor.h>
#endif

/*
 * 387 and 287 Numeric Coprocessor Extension (NPX) Driver.
 *
 * We do lazy initialization and switching using the TS bit in cr0 and the
 * MDL_USEDFPU bit in mdproc.
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

#define	fldcw(addr)		__asm("fldcw %0" : : "m" (*addr))
#define	fnclex()		__asm("fnclex")
#define	fninit()		__asm("fninit")
#define	fnsave(addr)		__asm("fnsave %0" : "=m" (*addr))
#define	fnstcw(addr)		__asm("fnstcw %0" : "=m" (*addr))
#define	fnstsw(addr)		__asm("fnstsw %0" : "=m" (*addr))
#define	fp_divide_by_0()	__asm("fldz; fld1; fdiv %st,%st(1); fwait")
#define	frstor(addr)		__asm("frstor %0" : : "m" (*addr))
#define	fwait()			__asm("fwait")
#ifndef XEN
#define	clts()			__asm("clts")
#define	stts()			lcr0(rcr0() | CR0_TS)
#else
#define	clts()
#define	stts()
#endif

int npxdna(struct cpu_info *);
static int	npxdna_notset(struct cpu_info *);
static int	npxdna_s87(struct cpu_info *);
#ifdef I686_CPU
static int	npxdna_xmm(struct cpu_info  *);
#endif /* I686_CPU */
static int	x86fpflags_to_ksiginfo(u_int32_t flags);

#ifdef I686_CPU
#define	fxsave(addr)		__asm("fxsave %0" : "=m" (*addr))
#define	fxrstor(addr)		__asm("fxrstor %0" : : "m" (*addr))
#endif /* I686_CPU */

static	enum npx_type		npx_type;
volatile u_int			npx_intrs_while_probing;
volatile u_int			npx_traps_while_probing;

extern int i386_fpu_present;
extern int i386_fpu_exception;
extern int i386_fpu_fdivbug;

struct npx_softc		*npx_softc;

static __inline void
fpu_save(union savefpu *addr)
{
#ifdef I686_CPU
	if (i386_use_fxsave)
	{
                fxsave(&addr->sv_xmm);

		/* FXSAVE doesn't FNINIT like FNSAVE does -- so do it here. */
		fninit();
	} else
#endif /* I686_CPU */
		fnsave(&addr->sv_87);
}

static int
npxdna_notset(struct cpu_info *ci)
{
	panic("npxdna vector not initialized");
}

int    (*npxdna_func)(struct cpu_info *) = npxdna_notset;

#if 0
static int
npxdna_empty(struct cpu_info *ci)
{

	/* raise a DNA TRAP, math_emulate would take over eventually */
	IPRINTF(("Emul"));
	return 0;
}


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

	save_eflags = read_eflags();
	disable_intr();
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
	enable_intr();

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

	disable_intr();
	lcr0(rcr0() | (CR0_EM|CR0_TS));

	irqmask = i8259_setmask(irqmask);

	idt[NRSVIDT + irq] = save_idt_npxintr;
	idt_allocmap[NRSVIDT + irq] = 1;

	idt[16] = save_idt_npxtrap;
	write_eflags(save_eflags);

	if ((rv == NPX_NONE) || (rv == NPX_BROKEN)) {
		/* No FPU. Handle it here, npxattach won't be called */
		npxdna_func = npxdna_empty;
	}

	return (rv);
}
#endif

void npxinit(ci)
	struct cpu_info *ci;
{
	lcr0(rcr0() & ~(CR0_EM|CR0_TS));
	fninit();
	if (npx586bug1(4195835, 3145727) != 0) {
		i386_fpu_fdivbug = 1;
		printf("%s: WARNING: Pentium FDIV bug detected!\n",
		    ci->ci_dev->dv_xname);
	}
	lcr0(rcr0() | (CR0_TS));
}

/*
 * Common attach routine.
 */
void
npxattach(struct npx_softc *sc)
{

	npx_softc = sc;
	npx_type = sc->sc_type;

	i386_fpu_present = 1;

#ifdef I686_CPU
	if (i386_use_fxsave)
		npxdna_func = npxdna_xmm;
	else
#endif /* I686_CPU */
		npxdna_func = npxdna_s87;
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
npxintr(void *arg, struct intrframe iframe)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l = ci->ci_fpcurlwp;
	union savefpu *addr;
	struct intrframe *frame = &iframe;
	struct npx_softc *sc;
	ksiginfo_t ksi;

	sc = npx_softc;

	uvmexp.traps++;
	IPRINTF(("%s: fp intr\n", ci->ci_dev->dv_xname));

	/*
	 * If we're saving, ignore the interrupt.  The FPU will generate
	 * another one when we restore the state later.
	 */
	if (ci->ci_fpsaving)
		return (1);

	if (l == NULL || npx_type == NPX_NONE) {
		printf("npxintr: l = %p, curproc = %p, npx_type = %d\n",
		    l, curproc, npx_type);
		printf("npxintr: came from nowhere");
		return 1;
	}

#ifdef DIAGNOSTIC
	/*
	 * At this point, fpcurlwp should be curlwp.  If it wasn't, the TS
	 * bit should be set, and we should have gotten a DNA exception.
	 */
	if (l != curlwp)
		panic("npxintr: wrong process");
#endif

	/*
	 * Find the address of fpcurproc's saved FPU state.  (Given the
	 * invariant above, this is always the one in curpcb.)
	 */
	addr = &l->l_addr->u_pcb.pcb_savefpu;
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

	return (1);
}

/* map x86 fp flags to ksiginfo fp codes 		*/
/* see table 8-4 of the IA-32 Intel Architecture	*/
/* Software Developer's Manual, Volume 1		*/
/* XXX punting on the stack fault with FLTINV		*/
static int
x86fpflags_to_ksiginfo(u_int32_t flags)
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
 * Save the previous state, if necessary, and restore our last
 * saved state.
 * XXX If we were the last process to use the FPU, we should be able
 * to simply return.
 */

#ifdef I686_CPU
static int
npxdna_xmm(struct cpu_info *ci)
{
	struct lwp *l;
	int s;
#ifdef XXXXENDEBUG_LOW
	struct trapframe *f = (struct trapframe *)((uint32_t *)(void *)&ci)[1];
#endif

	KDASSERT(i386_use_fxsave == 1);

	if (ci->ci_fpsaving) {
		printf("recursive npx trap; cr0=%x\n", rcr0());
		return (0);
	}

	s = splipi();		/* lock out IPI's while we clean house.. */
#ifdef MULTIPROCESSOR
	l = ci->ci_curlwp;
#else
	l = curlwp;
#endif
	IPRINTF(("%s: dna for lwp %p at %p, eax %p\n", ci->ci_dev->dv_xname,
		    l, (void *)f->tf_eip, (void *)f->tf_eax));
	/*
	 * XXX should have a fast-path here when no save/restore is necessary
	 */
	/*
	 * Initialize the FPU state to clear any exceptions.  If someone else
	 * was using the FPU, save their state (which does an implicit
	 * initialization).
	 */
	if (ci->ci_fpcurlwp != NULL) {
		IPRINTF(("Save"));
		npxsave_cpu(ci, 1);
	} else {
		IPRINTF(("Init"));
		fninit();
		fwait();
	}
	splx(s);

	IPRINTF(("%s: done saving\n", ci->ci_dev->dv_xname));
	KDASSERT(ci->ci_fpcurlwp == NULL);
#ifndef MULTIPROCESSOR
	KDASSERT(l->l_addr->u_pcb.pcb_fpcpu == NULL);
#else
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		npxsave_lwp(l, 1);
#endif
	l->l_addr->u_pcb.pcb_cr0 &= ~CR0_TS;
	clts();
	s = splipi();
	ci->ci_fpcurlwp = l;
	l->l_addr->u_pcb.pcb_fpcpu = ci;
	splx(s);

	if ((l->l_md.md_flags & MDL_USEDFPU) == 0) {
		fldcw(&l->l_addr->u_pcb.pcb_savefpu.sv_xmm.sv_env.en_cw);
		l->l_md.md_flags |= MDL_USEDFPU;
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
		__asm __volatile("ffree %%st(7)\n\tfld %0" : : "m" (zero));
		fxrstor(&l->l_addr->u_pcb.pcb_savefpu.sv_xmm);
	}

	return (1);
}
#endif /* I686_CPU */

static int
npxdna_s87(struct cpu_info *ci)
{
	struct lwp *l;
	int s;

	KDASSERT(i386_use_fxsave == 0);

	/*
	 * Because we don't have clts() we will trap on the fnsave in
	 * fpu_save, if we're saving the FPU state not from interrupt
	 * context (f.i. during fork()).  Just return in this case.
	 */
	if (ci->ci_fpsaving)
		return (1);

	s = splipi();		/* lock out IPI's while we clean house.. */
#ifdef MULTIPROCESSOR
	l = ci->ci_curlwp;
#else
	l = curlwp;
#endif

	IPRINTF(("%s: dna for lwp %p\n", ci->ci_dev->dv_xname, l));
	/*
	 * If someone else was using our FPU, save their state (which does an
	 * implicit initialization); otherwise, initialize the FPU state to
	 * clear any exceptions.
	 */
	if (ci->ci_fpcurlwp != NULL)
		npxsave_cpu(ci, 1);
	else {
		clts();
		IPRINTF(("%s: fp init\n", ci->ci_dev->dv_xname));
		fninit();
		fwait();
		stts();
	}
	splx(s);

	IPRINTF(("%s: done saving\n", ci->ci_dev->dv_xname));
	KDASSERT(ci->ci_fpcurlwp == NULL);
#ifndef MULTIPROCESSOR
	KDASSERT(l->l_addr->u_pcb.pcb_fpcpu == NULL);
#else
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		npxsave_lwp(l, 1);
#endif
	l->l_addr->u_pcb.pcb_cr0 &= ~CR0_TS;
	clts();
	s = splipi();
	ci->ci_fpcurlwp = l;
	l->l_addr->u_pcb.pcb_fpcpu = ci;
	ci->ci_fpused = 1;
	splx(s);


	if ((l->l_md.md_flags & MDL_USEDFPU) == 0) {
		fldcw(&l->l_addr->u_pcb.pcb_savefpu.sv_87.sv_env.en_cw);
		l->l_md.md_flags |= MDL_USEDFPU;
	} else {
		/*
		 * The following frstor may cause an IRQ13 when the state being
		 * restored has a pending error.  The error will appear to have
		 * been triggered by the current (npx) user instruction even
		 * when that instruction is a no-wait instruction that should
		 * not trigger an error (e.g., fnclex).  On at least one 486
		 * system all of the no-wait instructions are broken the same
		 * as frstor, so our treatment does not amplify the breakage.
		 * On at least one 386/Cyrix 387 system, fnclex works correctly
		 * while frstor and fnsave are broken, so our treatment breaks
		 * fnclex if it is the first FPU instruction after a context
		 * switch.
		 */
		frstor(&l->l_addr->u_pcb.pcb_savefpu.sv_87);
	}

	return (1);
}

void
npxsave_cpu (struct cpu_info *ci, int save)
{
	struct lwp *l;
	int s;

	KDASSERT(ci == curcpu());

	l = ci->ci_fpcurlwp;
	if (l == NULL)
		return;

	IPRINTF(("%s: fp CPU %s lwp %p\n", ci->ci_dev->dv_xname,
	    save? "save" : "flush", l));

	if (save) {
#ifdef DIAGNOSTIC
		if (ci->ci_fpsaving != 0)
			panic("npxsave_cpu: recursive save!");
#endif
		 /*
		  * Set ci->ci_fpsaving, so that any pending exception will be
		  * thrown away.  (It will be caught again if/when the FPU
		  * state is restored.)
		  *
		  * XXX on i386 and earlier, this routine should always be
		  * called at spl0; if it might called with the NPX interrupt
		  * masked, it would be necessary to forcibly unmask the NPX
		  * interrupt so that it could succeed.
		  * XXX this is irrelevant on 486 and above (systems
		  * which report FP failures via traps rather than irq13).
		  * XXX punting for now..
		  */
		clts();
		ci->ci_fpsaving = 1;
		fpu_save(&l->l_addr->u_pcb.pcb_savefpu);
		ci->ci_fpsaving = 0;
	}

	/*
	 * We set the TS bit in the saved CR0 for this process, so that it
	 * will get a DNA exception on any FPU instruction and force a reload.
	 */
	l->l_addr->u_pcb.pcb_cr0 |= CR0_TS;

	s = splipi();
	l->l_addr->u_pcb.pcb_fpcpu = NULL;
	ci->ci_fpcurlwp = NULL;
	ci->ci_fpused = 1;
	splx(s);
}

/*
 * Save l's FPU state, which may be on this processor or another processor.
 *
 * The FNSAVE instruction clears the FPU state.  Rather than reloading the FPU
 * immediately, we clear fpcurproc and turn on CR0_TS to force a DNA and a
 * reload of the FPU state the next time we try to use it.  This routine
 * is only called when forking, core dumping, or debugging, or swapping,
 * so the lazy reload at worst forces us to trap once per fork(), and at best
 * saves us a reload once per fork().
 */
void
npxsave_lwp(struct lwp *l, int save)
{
	struct cpu_info *ci = curcpu();
	struct cpu_info *oci;

	KDASSERT(l->l_addr != NULL);

	oci = l->l_addr->u_pcb.pcb_fpcpu;
	if (oci == NULL)
		goto end;

	IPRINTF(("%s: fp %s lwp %p\n", ci->ci_dev->dv_xname,
	    save? "save" : "flush", l));

#if defined(MULTIPROCESSOR)
	if (oci == ci) {
		int s = splipi();
		npxsave_cpu(ci, save);
		splx(s);
	} else {
#ifdef DIAGNOSTIC
		int spincount;
#endif

		IPRINTF(("%s: fp ipi to %s %s lwp %p\n",
		    ci->ci_dev->dv_xname,
		    oci->ci_dev->dv_xname,
		    save? "save" : "flush", l));

		x86_send_ipi(oci,
		    save ? X86_IPI_SYNCH_FPU : X86_IPI_FLUSH_FPU);

#ifdef DIAGNOSTIC
		spincount = 0;
#endif
		while (l->l_addr->u_pcb.pcb_fpcpu != NULL)
#ifdef DIAGNOSTIC
		{
			spincount++;
			if (spincount > 10000000) {
				panic("fp_save ipi didn't");
			}
		}
#else
		__insn_barrier();
		;
#endif
	}
#else
	KASSERT(ci->ci_fpcurlwp == l);
	npxsave_cpu(ci, save);
#endif
end:
	HYPERVISOR_fpu_taskswitch();
}
