/*-
 * Copyright (c) 1993 Charles Hannum.
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
 *	from: @(#)npx.c	7.2 (Berkeley) 5/12/91
 *	$Id: npx.c,v 1.7.4.13 1993/12/05 11:20:45 mycroft Exp $
 */

#ifdef DDB
#define STATIC
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/pio.h>
#include <machine/pcb.h>
#include <machine/specialreg.h>
#include <machine/trap.h>

#include <i386/isa/isavar.h>
#include <i386/isa/isa.h>
#include <i386/isa/icu.h>

/*
 * 387 and 287 Numeric Coprocessor Extension (NPX) Driver.
 */
#define	fldcw(addr)		__asm("fldcw %0" : : "m" (*addr))
#define	fnclex()		__asm("fnclex")
#define	fninit()		__asm("fninit")
#define	fnsave(addr)		__asm("fnsave %0" : "=m" (*addr) : "0" (*addr))
#define	fnstcw(addr)		__asm("fnstcw %0" : "=m" (*addr) : "0" (*addr))
#define	fnstsw(addr)		__asm("fnstsw %0" : "=m" (*addr) : "0" (*addr))
#define	fp_divide_by_0()	__asm("fldz; fld1; fdiv %st,%st(1); fwait")
#define	frstor(addr)		__asm("frstor %0" : : "m" (*addr))
#define	fwait()			__asm("fwait")
#define	read_eflags()		({u_long ef; \
				  __asm("pushf; popl %0" : "=a" (ef)); \
				  ef;})
#define	start_emulating()	__asm("smsw %%ax; orb %0,%%al; lmsw %%ax" \
				      : : "n" (CR0_TS) : "ax")
#define	stop_emulating()	__asm("clts")
#define	write_eflags(ef)	__asm("pushl %0; popf" : : "a" ((u_long) ef))

extern	struct gate_descriptor idt[];

int	npxdna		__P((void));
void	npxexit		__P((struct proc *p));
void	npxinit		__P((u_int control));
void	npxsave		__P((struct save87 *addr));

struct	npx_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;
};

STATIC int npxprobe __P((struct device *, struct cfdata *, void *));
STATIC void npxforceintr __P((void *));
STATIC void npxattach __P((struct device *, struct device *, void *));
int npxintr __P((void *));

struct	cfdriver npxcd =
{ NULL, "npx", npxprobe, npxattach, DV_DULL, sizeof(struct npx_softc) };

struct	proc *npxproc;
STATIC	int floating_point = 0;

enum	npxtype {
	NONE,
	INTERRUPT,
	TRAP,
};
STATIC	enum npxtype npxtype = NONE;
STATIC	u_short npxaddr;

STATIC	volatile int npx_traps_while_probing,
		     npx_intrs_while_probing;

/*
 * Special interrupt handlers during probe.  This code is disgusting.
 */
void probeintr(void);
__asm
("
	.text
_probeintr:
	ss
	incl	_npx_intrs_while_probing
	pushl	%eax
	pushl	%edx
	movb	$0x65,%al
	outb	%al,$0xa0
	movb	$0x62,%al
	outb	%al,$0x20
	movb	$0,%al
	movw	_npxaddr,%dx
	outb	%al,%dx
	popl	%edx
	popl	%eax
	iret
");

void probetrap(void);
__asm
("
	.text
_probetrap:
	ss
	incl	_npx_traps_while_probing
	fnclex
	iret
");

/*
 * Probe routine.  Initialize cr0 to give correct behaviour for [f]wait
 * whether the device exists or not (XXX should be elsewhere).  Set flags
 * to tell npxattach() what to do.  Modify device struct if npx doesn't
 * need to use interrupts.  Return 1 if device exists.
 */
STATIC int
npxprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;
	int	control, status;

#ifdef DIAGNOSTIC
	if (cf->cf_unit != 0)
		panic("npxprobe: unit != 0");
#endif

	if (iobase == IOBASEUNK)
		return 0;

	npxaddr = iobase;

	/*
	 * Partially reset the coprocessor, if any.  Some BIOS's don't reset
	 * it after a warm boot.
	 */
	outb(iobase + 1, 0);	/* full reset on some systems, NOP on others */
	outb(iobase + 0, 0);	/* clear BUSY# latch */

	/*
	 * Prepare to trap all ESC (i.e., NPX) instructions and all WAIT
	 * instructions.  We must set the CR0_MP bit and use the CR0_TS
	 * bit to control the trap, because setting the CR0_EM bit does
	 * not cause WAIT instructions to trap.  It's important to trap
	 * WAIT instructions - otherwise the "wait" variants of no-wait
	 * control instructions would degenerate to the "no-wait" variants
	 * after FP context switches but work correctly otherwise.  It's
	 * particularly important to trap WAITs when there is no NPX -
	 * otherwise the "wait" variants would always degenerate.
	 *
	 * Try setting CR0_NE to get correct error reporting on 486DX's.
	 * Setting it should fail or do nothing on lesser processors.
	 */
	lcr0(rcr0() | CR0_MP | CR0_NE);

	/*
	 * But don't trap while we're probing.
	 */
	stop_emulating();

	/*
	 * Finish resetting the coprocessor, if any.  If there is an error
	 * pending, then we may get a bogus IRQ13, but it will be ignored.
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
			if (ia->ia_irq == IRQUNK) {
				u_long	save_eflags;
				struct	gate_descriptor save_idt_npxtrap,
							save_idt_npxintr;

				save_eflags = read_eflags();
				printf("imask %x cpl %x eflags %x\n",
					imask, cpl, save_eflags);
				save_idt_npxtrap = idt[16];
				setidt(16, probetrap, SDT_SYS386TGT, SEL_KPL);
				save_idt_npxintr = idt[13 + ICU_OFFSET];
				setidt(13 + ICU_OFFSET, probeintr,
					SDT_SYS386TGT, SEL_KPL);
				npxforceintr(&control);
				idt[13 + ICU_OFFSET] = save_idt_npxintr;
				idt[16] = save_idt_npxtrap;
				write_eflags(save_eflags);

				if (npx_traps_while_probing)
					ia->ia_irq = IRQNONE;
				else if (npx_intrs_while_probing)
					ia->ia_irq = IRQ13;
				else
					return 0;
			}

			if (ia->ia_irq == IRQNONE) {
				/*
				 * Good, exception 16 works.
				 */
				npxtype = TRAP;
			} else {
				/*
				 * Bad, we are stuck with IRQ13.
				 */
				npxtype = INTERRUPT;
			}

			ia->ia_iosize = 16;
			return 1;
		}
	}

	/*
	 * Probe failed.
	 */
	return 0;
}

STATIC void
npxforceintr(aux)
	void *aux;
{
	int	*control = aux;

	/*
	 * We have an npx, now divide by 0 to see if exception
	 * 16 works.
	 */
	*control &= ~(1 << 2);	/* enable divide by 0 trap */
	fldcw(control);
	npx_traps_while_probing = 0;
	npx_intrs_while_probing = 0;
#ifdef DDB
	__asm(".globl bpnpxforceintr; bpnpxforceintr:");
#endif
	fp_divide_by_0();
}

/*
 * Attach routine - announce which it is, and wire into system
 */
STATIC void
npxattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct	npx_softc *sc = (struct npx_softc *)self;
	struct	isa_attach_args *ia = aux;
	extern	int floating_point;

	floating_point = 1;
	if (npxtype == TRAP)
		printf(": using exception 16\n");
	else
		printf("\n");
	isa_establish(&sc->sc_id, &sc->sc_dev);

	if (npxtype == INTERRUPT) {
		sc->sc_ih.ih_fun = npxintr;
		sc->sc_ih.ih_arg = NULL;
		intr_establish(ia->ia_irq, &sc->sc_ih, DV_DULL);
	}
}

/*
 * Initialize floating point unit.
 */
void
npxinit(control)
	u_int control;
{
	struct save87 dummy;

	if (!floating_point)
		return;
	/*
	 * fninit has the same h/w bugs as fnsave.  Use the detoxified
	 * fnsave to throw away any junk in the fpu.  fnsave initializes
	 * the fpu and sets npxproc = NULL as important side effects.
	 */
	npxsave(&dummy);
	stop_emulating();
	fldcw(&control);
	if (curpcb != NULL)
		fnsave(&curpcb->pcb_savefpu);
	start_emulating();
}

/*
 * Free coprocessor (if we have it).
 */
void
npxexit(p)
	struct proc *p;
{
	if (p == npxproc) {
		start_emulating();
		npxproc = NULL;
	}
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
npxintr(aux)
	void *aux;
{

	if (npxproc == NULL || npxtype == NONE) {
		/* XXX no %p in kern/subr_prf.c.  Cast to quiet gcc -Wall. */
		printf("npxintr: npxproc = %lx, curproc = %lx, npxtype = %d\n",
		       (u_long) npxproc, (u_long) curproc, npxtype);
		return 0;
	}
	if (npxproc != curproc) {
		printf("npxintr: npxproc = %lx, curproc = %lx, npxtype = %d\n",
		       (u_long) npxproc, (u_long) curproc, npxtype);
		return 0;
	}
	/*
	 * Save state.  This does an implied fninit.  It had better not halt
	 * the cpu or we'll hang.
	 */
	outb(npxaddr + 0, 0);			/* reset #BUSY latch */
	fnsave(&curpcb->pcb_savefpu);
	fwait();
	/*
	 * Restore control word (was clobbered by fnsave).
	 */
	fldcw(&curpcb->pcb_savefpu.sv_env.en_cw);
	fwait();
	/*
	 * Remember the exception status word and tag word.  The current
	 * (almost fninit'ed) fpu state is in the fpu and the exception
	 * state just saved will soon be junk.  However, the implied fninit
	 * doesn't change the error pointers or register contents, and we
	 * preserved the control word and will copy the status and tag
	 * words, so the complete exception state can be recovered.
	 */
	curpcb->pcb_savefpu.sv_ex_sw = curpcb->pcb_savefpu.sv_env.en_sw;
	curpcb->pcb_savefpu.sv_ex_tw = curpcb->pcb_savefpu.sv_env.en_tw;

	/*
	 * Pass exception to process.  We don't really care where it was
	 * generated.
	 */
	psignal(npxproc, SIGFPE);
	return 1;
}

/*
 * Implement device not available (DNA) exception
 *
 * It would be better to switch FP context here (only).  This would require
 * saving the state in the proc table instead of in the pcb.
 */
int
npxdna()
{
	if (npxtype == NONE)
		return 0;
#ifdef DIAGNOSTIC
	if (npxproc != NULL) {
		printf("npxdna: npxproc = %lx, curproc = %lx\n",
		       (u_long) npxproc, (u_long) curproc);
		npxsave(&npxproc->p_addr->u_pcb.pcb_savefpu);
	}
#endif

	stop_emulating();
	/*
	 * Record new context early in case frstor causes an IRQ13.
	 */
	npxproc = curproc;
	/*
	 * The following frstor may cause an IRQ13 when the state being
	 * restored has a pending error.  The error will appear to have been
	 * triggered by the current (npx) user instruction even when that
	 * instruction is a no-wait instruction that should not trigger an
	 * error (e.g., fnclex).  On at least one 486 system all of the
	 * no-wait instructions are broken the same as frstor, so our
	 * treatment does not amplify the breakage.  On at least one
	 * 386/Cyrix 387 system, fnclex works correctly while frstor and
	 * fnsave are broken, so our treatment breaks fnclex if it is the
	 * first FPU instruction after a context switch.
	 */
	frstor(&curpcb->pcb_savefpu);

	return 1;
}

/*
 * Wrapper for fnsave instruction to handle h/w bugs.  If there is an error
 * pending, then fnsave generates a bogus IRQ13 on some systems.  Force
 * any IRQ13 to be handled immediately, and then ignore it.  This routine is
 * often called at splhigh so it must not use many system services.  In
 * particular, it's much easier to install a special handler than to
 * guarantee that it's safe to use npxintr() and its supporting code.
 */
void
npxsave(addr)
	struct save87 *addr;
{
#if 0 /* XXXX */
	u_char	icu1_mask;
	u_char	icu2_mask;
	u_char	old_icu1_mask;
	u_char	old_icu2_mask;
	struct gate_descriptor	save_idt_npxintr;

	disable_intr();
	old_icu1_mask = inb(IO_ICU1 + 1);
	old_icu2_mask = inb(IO_ICU2 + 1);
	save_idt_npxintr = idt[npx_intrno];
	outb(IO_ICU1 + 1, old_icu1_mask & ~(IRQ_SLAVE | npx0mask));
	outb(IO_ICU2 + 1, old_icu2_mask & ~(npx0mask >> 8));
	idt[npx_intrno] = npx_idt_probeintr;
	enable_intr();
#endif
	stop_emulating();
	fnsave(addr);
	fwait();
	start_emulating();
	npxproc = NULL;
#if 0 /* XXXX */
	disable_intr();
	icu1_mask = inb(IO_ICU1 + 1);	/* masks may have changed */
	icu2_mask = inb(IO_ICU2 + 1);
	outb(IO_ICU1 + 1,
	     (icu1_mask & ~npx0mask) | (old_icu1_mask & npx0mask));
	outb(IO_ICU2 + 1,
	     (icu2_mask & ~(npx0mask >> 8))
	     | (old_icu2_mask & (npx0mask >> 8)));
	idt[npx_intrno] = save_idt_npxintr;
	enable_intr();		/* back to usual state */
#endif
}
