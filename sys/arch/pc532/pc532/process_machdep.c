/*	$NetBSD: process_machdep.c,v 1.5 1995/04/27 07:16:44 phil Exp $	*/

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
	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	regs->r_r0  = p->p_md.md_regs[REG_R0];
	regs->r_r1  = p->p_md.md_regs[REG_R1];
	regs->r_r2  = p->p_md.md_regs[REG_R2];
	regs->r_r3  = p->p_md.md_regs[REG_R3];
	regs->r_r4  = p->p_md.md_regs[REG_R4];
	regs->r_r5  = p->p_md.md_regs[REG_R5];
	regs->r_r6  = p->p_md.md_regs[REG_R6];
	regs->r_r7  = p->p_md.md_regs[REG_R7];
	regs->r_sb  = p->p_md.md_regs[REG_SB];
	regs->r_sp  = p->p_md.md_regs[REG_SP];
	regs->r_fp  = p->p_md.md_regs[REG_FP];
	regs->r_pc  = p->p_md.md_regs[REG_PC];
	regs->r_psr = p->p_md.md_regs[REG_PSR] >> 16;

	return (0);
}

int
process_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	int psr;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	psr = regs->r_psr;
	if ((psr & PSL_USERSET) != PSL_USERSET)
		return (EPERM);

	p->p_md.md_regs[REG_R0]  = regs->r_r0;
	p->p_md.md_regs[REG_R1]  = regs->r_r1;
	p->p_md.md_regs[REG_R2]  = regs->r_r2;
	p->p_md.md_regs[REG_R3]  = regs->r_r3;
	p->p_md.md_regs[REG_R4]  = regs->r_r4;
	p->p_md.md_regs[REG_R5]  = regs->r_r5;
	p->p_md.md_regs[REG_R6]  = regs->r_r6;
	p->p_md.md_regs[REG_R7]  = regs->r_r7;
	p->p_md.md_regs[REG_SB]  = regs->r_sb;
	p->p_md.md_regs[REG_SP]  = regs->r_sp;
	p->p_md.md_regs[REG_FP]  = regs->r_fp;
	p->p_md.md_regs[REG_PC]  = regs->r_pc;
	p->p_md.md_regs[REG_PSR] = psr << 16;

	return (0);
}

int
process_sstep(p, sstep)
	struct proc *p;
	int sstep;
{
	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	if (sstep)
		p->p_md.md_regs[REG_PSR] |= (PSL_T << 16);
	else
		p->p_md.md_regs[REG_PSR] &= ~(PSL_T << 16);

	return (0);
}

int
process_set_pc(p, addr)
	struct proc *p;
	caddr_t addr;
{
	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	p->p_md.md_regs[REG_PC] = (int)addr;

	return (0);
}
