/*	$NetBSD: process_machdep.c,v 1.6 1995/07/28 07:51:38 phil Exp $	*/

/*
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1993 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 */

/* Modified by Phil Nelson for the pc532 port.  1/12/94 */

/*
 * This file may seem a bit stylized, but that so that it's easier to port.
 * Functions to be implemented here are:
 *
 * process_read_regs(proc, regs)
 *	Get the current user-visible register set from the process
 *	and copy it into the regs structure (<machine/reg.h>).
 *	The process is stopped at the time read_regs is called.
 *
 * process_write_regs(proc, regs)
 *	Update the current register set from the passed in regs
 *	structure.  Take care to avoid clobbering special CPU
 *	registers or privileged bits in the PSL.
 *	The process is stopped at the time write_regs is called.
 *
 * process_sstep(proc)
 *	Arrange for the process to trap after executing a single instruction.
 *
 * process_set_pc(proc)
 *	Set the process's program counter.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/frame.h>

int
process_read_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct on_stack	*r;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	/*
	 * Figure the offset from the start of the user area for the
	 * desired process, which is probably not the current process.
	 * Note that p->p_md.md_regs is a pointer on the kernel stack,
	 * which is mapped at the same place in each process, but is
	 * only valid for the currently running process.  However,
	 * all of the user structures are present in kernel VM, and
	 * may be accessed via p_addr.  Thus, if we look in the right
	 * place just after the user structure, we will find the aliased
	 * kernel stack for the process.
	 */
	r = (struct on_stack *)
		((char *) p->p_addr + ((u_int) p->p_md.md_regs - USRSTACK));
	regs->r_r0  = r->pcb_reg[REG_R0];
	regs->r_r1  = r->pcb_reg[REG_R1];
	regs->r_r2  = r->pcb_reg[REG_R2];
	regs->r_r3  = r->pcb_reg[REG_R3];
	regs->r_r4  = r->pcb_reg[REG_R4];
	regs->r_r5  = r->pcb_reg[REG_R5];
	regs->r_r6  = r->pcb_reg[REG_R6];
	regs->r_r7  = r->pcb_reg[REG_R7];
	regs->r_sb  = r->pcb_sb;
	regs->r_sp  = r->pcb_usp;
	regs->r_fp  = r->pcb_fp;
	regs->r_pc  = r->pcb_pc;
	regs->r_psr = r->pcb_psr;

	return (0);
}

int
process_read_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct pcb	*pcb;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	/*
	 * The floating point registers are stored in the pcb contained
	 * within the user structure.  Nothing fancy to access them.
	 */
	pcb = &p->p_addr->u_pcb;
	regs->r_fpsr = pcb->pcb_fsr;
	regs->r_f0 = pcb->pcb_freg[0];
	regs->r_f1 = pcb->pcb_freg[1];
	regs->r_f2 = pcb->pcb_freg[2];
	regs->r_f3 = pcb->pcb_freg[3];
	regs->r_f4 = pcb->pcb_freg[4];
	regs->r_f5 = pcb->pcb_freg[5];
	regs->r_f6 = pcb->pcb_freg[6];
	regs->r_f7 = pcb->pcb_freg[7];

	return (0);
}

int
process_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct on_stack	*r;
	int psr;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	r = (struct on_stack *)
		((char *) p->p_addr + ((u_int) p->p_md.md_regs - USRSTACK));
	psr = regs->r_psr;
	if ((psr & PSL_USERSET) != PSL_USERSET)
		return (EPERM);

	r->pcb_reg[REG_R0] = regs->r_r0;
	r->pcb_reg[REG_R1] = regs->r_r1;
	r->pcb_reg[REG_R2] = regs->r_r2;
	r->pcb_reg[REG_R3] = regs->r_r3;
	r->pcb_reg[REG_R4] = regs->r_r4;
	r->pcb_reg[REG_R5] = regs->r_r5;
	r->pcb_reg[REG_R6] = regs->r_r6;
	r->pcb_reg[REG_R7] = regs->r_r7;
	r->pcb_sb = regs->r_sb;
	r->pcb_usp = regs->r_sp;
	r->pcb_fp = regs->r_fp;
	r->pcb_pc = regs->r_pc;
	r->pcb_psr = psr;

	return (0);
}

int
process_write_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct pcb	*pcb;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	pcb = &p->p_addr->u_pcb;
	pcb->pcb_fsr = regs->r_fpsr;
	pcb->pcb_freg[0] = regs->r_f0;
	pcb->pcb_freg[1] = regs->r_f1;
	pcb->pcb_freg[2] = regs->r_f2;
	pcb->pcb_freg[3] = regs->r_f3;
	pcb->pcb_freg[4] = regs->r_f4;
	pcb->pcb_freg[5] = regs->r_f5;
	pcb->pcb_freg[6] = regs->r_f6;
	pcb->pcb_freg[7] = regs->r_f7;

	return (0);
}

int
process_sstep(p, sstep)
	struct proc *p;
	int sstep;
{
	struct on_stack	*r;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	r = (struct on_stack *)
		((char *) p->p_addr + ((u_int) p->p_md.md_regs - USRSTACK));
	if (sstep)
		r->pcb_psr |= PSL_T;
	else
		r->pcb_psr &= ~PSL_T;

	return (0);
}

int
process_set_pc(p, addr)
	struct proc *p;
	caddr_t addr;
{
	struct on_stack	*r;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	r = (struct on_stack *)
		((char *) p->p_addr + ((u_int) p->p_md.md_regs - USRSTACK));
	r->pcb_pc = addr;

	return (0);
}
