/*	$NetBSD: fpu.c,v 1.2 2002/06/12 19:13:27 fvdl Exp $	*/

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

/*
 * XXXfvdl update copyright notice. this started out as a stripped isa/npx.c
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
#include <machine/fpu.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <i386/isa/icu.h>

/*
 * We do lazy initialization and switching using the TS bit in cr0 and the
 * MDP_USEDFPU bit in mdproc.
 *
 * DNA exceptions are handled like this:
 *
 * 1) If there is no FPU, return and go to the emulator.
 * 2) If someone else has used the FPU, save its state into that process's PCB.
 * 3a) If MDP_USEDFPU is not set, set it and initialize the FPU.
 * 3b) Otherwise, reload the process's previous FPU state.
 *
 * When a process is created or exec()s, its saved cr0 image has the TS bit
 * set and the MDP_USEDFPU bit clear.  The MDP_USEDFPU bit is set when the
 * process first gets a DNA and the FPU is initialized.  The TS bit is turned
 * off when the FPU is used, and turned on again later when the process's FPU
 * state is saved.
 */

#define	fninit()		__asm("fninit")
#define fwait()			__asm("fwait")
#define	fxsave(addr)		__asm("fxsave %0" : "=m" (*addr))
#define	fxrstor(addr)		__asm("fxrstor %0" : : "m" (*addr))
#define fldcw(addr)		__asm("fldcw %0" : : "m" (*addr))
#define	clts()			__asm("clts")
#define	stts()			lcr0(rcr0() | CR0_TS)

void fpudna(struct proc *);
void fpuexit(void);
static void fpusave1(void);

struct proc	*fpuproc;

/*
 * Init the FPU.
 */
void
fpuinit(void)
{
	lcr0(rcr0() & ~(CR0_EM|CR0_TS));
	fninit();
	lcr0(rcr0() | (CR0_TS));
}

/*
 * Record the FPU state and reinitialize it all except for the control word.
 * Then generate a SIGFPE.
 *
 * Reinitializing the state allows naive SIGFPE handlers to longjmp without
 * doing any fixups.
 */

void
fputrap(frame)
	struct trapframe *frame;
{
	register struct proc *p = fpuproc;
	struct savefpu *sfp = &p->p_addr->u_pcb.pcb_savefpu;
	u_int16_t cw;

#ifdef DIAGNOSTIC
	/*
	 * At this point, fpuproc should be curproc.  If it wasn't, the TS bit
	 * should be set, and we should have gotten a DNA exception.
	 */
	if (p != curproc)
		panic("fputrap: wrong process");
#endif

	fxsave(sfp);
	if (frame->tf_trapno == T_XMM) {
	} else {
		fninit();
		fwait();
		cw = sfp->fp_fxsave.fx_fcw;
		fldcw(&cw);
		fwait();
	}
	sfp->fp_ex_tw = sfp->fp_fxsave.fx_ftw;
	sfp->fp_ex_sw = sfp->fp_fxsave.fx_fsw;
	trapsignal(p, SIGFPE, frame->tf_err);
}

/*
 * Wrapper for the fnsave instruction.  We set the TS bit in the saved CR0 for
 * this process, so that it will get a DNA exception on the FPU instruction and
 * force a reload.
 */
static inline void
fpusave1(void)
{
	struct proc *p = fpuproc;

	fxsave(&p->p_addr->u_pcb.pcb_savefpu);
	p->p_addr->u_pcb.pcb_cr0 |= CR0_TS;
}

/*
 * Implement device not available (DNA) exception
 *
 * If we were the last process to use the FPU, we can simply return.
 * Otherwise, we save the previous state, if necessary, and restore our last
 * saved state.
 */
void
fpudna(struct proc *p)
{

#ifdef DIAGNOSTIC
	if (cpl != 0)
		panic("fpudna: masked");
#endif
	u_int16_t cw;

	p->p_addr->u_pcb.pcb_cr0 &= ~CR0_TS;
	clts();

	/*
	 * Initialize the FPU state to clear any exceptions.  If someone else
	 * was using the FPU, save their state.
	 */
	if (fpuproc != 0 && fpuproc != p)
		fpusave1();

	fninit();
	fwait();

	fpuproc = p;

	if ((p->p_md.md_flags & MDP_USEDFPU) == 0) {
		cw = p->p_addr->u_pcb.pcb_savefpu.fp_fxsave.fx_fcw;
		fldcw(&cw);
		p->p_md.md_flags |= MDP_USEDFPU;
	} else
		fxrstor(&p->p_addr->u_pcb.pcb_savefpu);
}

/*
 * Drop the current FPU state on the floor.
 */
void
fpudrop(void)
{
	struct proc *p = fpuproc;

	fpuproc = 0;
	stts();
	p->p_addr->u_pcb.pcb_cr0 |= CR0_TS;
}

/*
 * Save fpuproc's FPU state.
 *
 * The FNSAVE instruction clears the FPU state.  Rather than reloading the FPU
 * immediately, we clear fpuproc and turn on CR0_TS to force a DNA and a reload
 * of the FPU state the next time we try to use it.  This routine is only
 * called when forking or core dumping, so the lazy reload at worst forces us
 * to trap once per fork(), and at best saves us a reload once per fork().
 */
void
fpusave(void)
{

#ifdef DIAGNOSTIC
	if (cpl != 0)
		panic("fpusave: masked");
#endif
	clts();
	fpusave1();
	fpuproc = 0;
	stts();
}

void
fpudiscard(struct proc *p)
{
	if (p == fpuproc) {
		fpuproc = 0;
		stts();
	}
}
