/*	$NetBSD: process_machdep.c,v 1.8 2002/07/05 18:45:22 matt Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/ptrace.h>

#include <machine/fpu.h>
#include <machine/pcb.h>
#include <machine/reg.h>

int
process_read_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct trapframe *tf = trapframe(p);

	memcpy(regs->fixreg, tf->fixreg, sizeof(regs->fixreg));
	regs->lr = tf->lr;
	regs->cr = tf->cr;
	regs->xer = tf->xer;
	regs->ctr = tf->ctr;
	regs->pc = tf->srr0;

	return 0;
}

int
process_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct trapframe *tf = trapframe(p);

	memcpy(tf->fixreg, regs->fixreg, sizeof(regs->fixreg));
	tf->lr = regs->lr;
	tf->cr = regs->cr;
	tf->xer = regs->xer;
	tf->ctr = regs->ctr;
	tf->srr0 = regs->pc;

	return 0;
}

int
process_read_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct pcb *pcb = &p->p_addr->u_pcb;

	/* Is the process using the fpu? */
	if ((pcb->pcb_flags & PCB_FPU) == 0) {
		memset(regs, 0, sizeof (struct fpreg));
		return 0;
	}

#ifdef PPC_HAVE_FPU
	if (p == curcpu()->ci_fpuproc)
		save_fpu(p);
#endif
	memcpy(regs, &pcb->pcb_fpu, sizeof (struct fpreg));

	return 0;
}

int
process_write_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct pcb *pcb = &p->p_addr->u_pcb;

#ifdef PPC_HAVE_FPU
	if (p == curcpu()->ci_fpuproc)
		curcpu()->ci_fpuproc = NULL;
#endif

	memcpy(&pcb->pcb_fpu, regs, sizeof(struct fpreg));

	/* pcb_fpu is initialized now. */
	pcb->pcb_flags |= PCB_FPU;

	return 0;
}

/*
 * Set the process's program counter.
 */
int
process_set_pc(p, addr)
	struct proc *p;
	caddr_t addr;
{
	struct trapframe *tf = trapframe(p);
	
	tf->srr0 = (int)addr;
	return 0;
}

int
process_sstep(p, sstep)
	struct proc *p;
	int sstep;
{
	struct trapframe *tf = trapframe(p);
	
	if (sstep)
		tf->srr1 |= PSL_SE;
	else
		tf->srr1 &= ~PSL_SE;
	return 0;
}
