/*	$NetBSD: npx.c,v 1.70.8.12 2001/04/30 16:23:14 sommerfeld Exp $	*/

#if 0
#define IPRINTF(x)	printf x
#else
#define	IPRINTF(x)
#endif

/*-
 * Copyright (c) 1994, 1995, 1998 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 William Jolitz.
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

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <i386/isa/icu.h>
#include <i386/isa/npxvar.h>

/*
 * 387 and 287 Numeric Coprocessor Extension (NPX) Driver.
 *
 * We do lazy initialization and switching using the TS bit in cr0 and the
 * MDP_USEDFPU bit in mdproc.
 *
 * DNA exceptions are handled like this:
 *
 * 1) If there is no NPX, return and go to the emulator.
 * 2) If someone else has used the NPX, save its state into that process's PCB.
 * 3a) If MDP_USEDFPU is not set, set it and initialize the NPX.
 * 3b) Otherwise, reload the process's previous NPX state.
 *
 * When a process is created or exec()s, its saved cr0 image has the TS bit
 * set and the MDP_USEDFPU bit clear.  The MDP_USEDFPU bit is set when the
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
#define	read_eflags()		({register u_long ef; \
				  __asm("pushfl; popl %0" : "=r" (ef)); \
				  ef;})
#define	write_eflags(x)		({register u_long ef = (x); \
				  __asm("pushl %0; popfl" : : "r" (ef));})
#define	clts()			__asm("clts")
#define	stts()			lcr0(rcr0() | CR0_TS)

int npxdna(struct cpu_info *);

static	enum npx_type		npx_type;
volatile u_int			npx_intrs_while_probing;
volatile u_int			npx_traps_while_probing;

extern int i386_fpu_present;
extern int i386_fpu_exception;
extern int i386_fpu_fdivbug;

struct npx_softc		*npx_softc;

enum npx_type
npxprobe1(bus_space_tag_t iot, bus_space_handle_t ioh, int irq)
{
	struct gate_descriptor save_idt_npxintr;
	struct gate_descriptor save_idt_npxtrap;
	enum npx_type rv = NPX_NONE;
	u_long	save_eflags;
	unsigned save_imen;
	int control;
	int status;

	save_eflags = read_eflags();
	disable_intr();
	save_idt_npxintr = idt[NRSVIDT + irq].gd;
	save_idt_npxtrap = idt[16].gd;
	setgate(&idt[NRSVIDT + irq].gd, probeintr, 0, SDT_SYS386IGT, SEL_KPL);
	setgate(&idt[16].gd, probetrap, 0, SDT_SYS386TGT, SEL_KPL);
	save_imen = imen;
	imen = ~((1 << IRQ_SLAVE) | (1 << irq));
	SET_ICUS();

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

	imen = save_imen;
	SET_ICUS();
	idt[NRSVIDT + irq].gd = save_idt_npxintr;
	idt[16].gd = save_idt_npxtrap;
	write_eflags(save_eflags);

	return (rv);
}

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

	npxinit(&cpu_info_primary);
	i386_fpu_present = 1;
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
npxintr(void *arg)
{
	struct cpu_info *ci = curcpu();
	register struct proc *p = ci->ci_fpcurproc;
	register struct save87 *addr;
	struct intrframe *frame = arg;
	struct npx_softc *sc;
	int code;

	sc = npx_softc;

	uvmexp.traps++;
	IPRINTF(("%s: fp intr\n", ci->ci_dev->dv_xname));

	/*
	 * Clear the interrupt latch.
	 */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 0, 0);

	/*
	 * If we're saving, ignore the interrupt.  The FPU will generate
	 * another one when we restore the state later.
	 */
	if (ci->ci_fpsaving)
		return (1);

	if (p == 0 || npx_type == NPX_NONE) {
		printf("npxintr: p = %p, curproc = %p, npx_type = %d\n",
		    p, curproc, npx_type);
		printf("npxintr: came from nowhere");
		return 1;
	}

#ifdef DIAGNOSTIC
	/*
	 * At this point, fpcurproc should be curproc.  If it wasn't, the TS
	 * bit should be set, and we should have gotten a DNA exception.
	 */
	if (p != curproc)
		panic("npxintr: wrong process");
#endif

	/*
	 * Find the address of fpcurproc's saved FPU state.  (Given the invariant
	 * above, this is always the one in curpcb.)
	 */
	addr = &p->p_addr->u_pcb.pcb_savefpu;
	/*
	 * Save state.  This does an implied fninit.  It had better not halt
	 * the cpu or we'll hang.
	 */
	fnsave(addr);
	fwait();
	/*
	 * Restore control word (was clobbered by fnsave).
	 */
	fldcw(&addr->sv_env.en_cw);
	fwait();
	/*
	 * Remember the exception status word and tag word.  The current
	 * (almost fninit'ed) fpu state is in the fpu and the exception
	 * state just saved will soon be junk.  However, the implied fninit
	 * doesn't change the error pointers or register contents, and we
	 * preserved the control word and will copy the status and tag
	 * words, so the complete exception state can be recovered.
	 */
	addr->sv_ex_sw = addr->sv_env.en_sw;
	addr->sv_ex_tw = addr->sv_env.en_tw;

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
		p->p_md.md_regs = (struct trapframe *)&frame->if_es;
#ifdef notyet
		/*
		 * Encode the appropriate code for detailed information on
		 * this exception.
		 */
		code = XXX_ENCODE(addr->sv_ex_sw);
#else
		code = 0;	/* XXX */
#endif
		trapsignal(p, SIGFPE, code);
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
		psignal(p, SIGFPE);
	}

	return (1);
}

/*
 * Implement device not available (DNA) exception
 *
 * Save the previous state, if necessary, and restore our last
 * saved state.
 * XXX If we were the last process to use the FPU, we should be able
 * to simply return.
 */
int
npxdna(struct cpu_info *ci)
{
	struct proc *p;
	int s;
	
	if (npx_type == NPX_NONE) {
		IPRINTF(("%s: fp emul\n", ci->ci_dev->dv_xname));
		return (0);
	}

	if (ci->ci_fpsaving) {
		printf("recursive npx trap; cr0=%x\n", rcr0());
		return (0);
	}
	
	s = splipi();		/* lock out IPI's while we clean house.. */
#ifdef MULTIPROCESSOR
	p = ci->ci_curproc;
#else
	p = curproc;
#endif
	
	IPRINTF(("%s: dna for %p\n", ci->ci_dev->dv_xname, p));	
	/*
	 * If someone else was using our FPU, save their state (which does an
	 * implicit initialization); otherwise, initialize the FPU state to
	 * clear any exceptions.
	 */
	if (ci->ci_fpcurproc != NULL)
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
	KDASSERT(ci->ci_fpcurproc == NULL);
#ifndef MULTIPROCESSOR
	KDASSERT(p->p_addr->u_pcb.pcb_fpcpu == NULL);
#else
	if (p->p_addr->u_pcb.pcb_fpcpu != NULL)
		npxsave_proc(p, 1);
#endif
	p->p_addr->u_pcb.pcb_cr0 &= ~CR0_TS;
	clts();
	s = splipi();
	ci->ci_fpcurproc = p;
	p->p_addr->u_pcb.pcb_fpcpu = ci;
	splx(s);
	
	if ((p->p_md.md_flags & MDP_USEDFPU) == 0) {
		fldcw(&p->p_addr->u_pcb.pcb_savefpu.sv_env.en_cw);
		p->p_md.md_flags |= MDP_USEDFPU;
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
		frstor(&p->p_addr->u_pcb.pcb_savefpu);
	}

	return (1);
}

void
npxsave_cpu (struct cpu_info *ci, int save)
{
	struct proc *p;
	int s;

	KDASSERT(ci == curcpu());

	p = ci->ci_fpcurproc;
	if (p == NULL)
		return;

	IPRINTF(("%s: fp cpu %s %p\n", ci->ci_dev->dv_xname,
	    save? "save" : "flush", p));
	
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
		fnsave(&p->p_addr->u_pcb.pcb_savefpu);
		fwait();
		ci->ci_fpsaving = 0;
	}

	/*
	 * We set the TS bit in the saved CR0 for this process, so that it
	 * will get a DNA exception on any FPU instruction and force a reload.
	 */
	stts();
	p->p_addr->u_pcb.pcb_cr0 |= CR0_TS;

	s = splipi();
	p->p_addr->u_pcb.pcb_fpcpu = NULL;
	ci->ci_fpcurproc = NULL;
	splx(s);
}

/*
 * Save p's FPU state, which may be on this processor or another processor.
 *
 * The FNSAVE instruction clears the FPU state.  Rather than reloading the FPU
 * immediately, we clear fpcurproc and turn on CR0_TS to force a DNA and a
 * reload of the FPU state the next time we try to use it.  This routine
 * is only called when forking, core dumping, or debugging, or swapping,
 * so the lazy reload at worst forces us to trap once per fork(), and at best
 * saves us a reload once per fork().
 */
void
npxsave_proc(struct proc *p, int save)
{
	struct cpu_info *ci = curcpu();
	struct cpu_info *oci;
	
	KDASSERT(p->p_addr != NULL);
	KDASSERT(p->p_flag & P_INMEM);
	
	oci = p->p_addr->u_pcb.pcb_fpcpu;
	if (oci == NULL)
		return;
	
	IPRINTF(("%s: fp proc %s %p\n", ci->ci_dev->dv_xname,
	    save? "save" : "flush", p));

#if defined(MULTIPROCESSOR)
	if (oci == ci) {
		int s = splipi();
		npxsave_cpu(ci, save);
		splx(s);
	} else {
		int spincount;
		
		IPRINTF(("%s: fp ipi to %s %s %p\n",
		    ci->ci_dev->dv_xname,
		    oci->ci_dev->dv_xname,
		    save? "save" : "flush", p));

		i386_send_ipi(oci,
		    save ? I386_IPI_SYNCH_FPU : I386_IPI_FLUSH_FPU);
		
		spincount = 0;
		while (p->p_addr->u_pcb.pcb_fpcpu != NULL)
		{
			spincount++;
			delay(1000); /* XXX */
			if (spincount > 10000) {
				panic("fp_save ipi didn't");
			}
		}
	}
#else
	KASSERT(ci->ci_fpcurproc == p);
	npxsave_cpu(ci, save);
#endif
}
