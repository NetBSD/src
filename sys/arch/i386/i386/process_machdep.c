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
 *
 * From:
 *	Id: procfs_i386.c,v 4.1 1993/12/17 10:47:45 jsp Rel
 *
 *	$Id: process_machdep.c,v 1.1 1994/01/08 11:13:06 cgd Exp $
 */

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
 * process_fix_sstep(proc)
 *	Cleanup process state after executing a single-step instruction.
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

extern int kstack[];		/* XXX */

int
process_read_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	void *ptr;
	struct trapframe *tp;

	if ((p->p_flag & SLOAD) == 0)
		return (EIO);

	ptr = (char *) p->p_addr + ((char *) p->p_regs - (char *) kstack);

	tp = ptr;
	regs->r_es = tp->tf_es;
	regs->r_ds = tp->tf_ds;
	regs->r_edi = tp->tf_edi;
	regs->r_esi = tp->tf_esi;
	regs->r_ebp = tp->tf_ebp;
	regs->r_ebx = tp->tf_ebx;
	regs->r_edx = tp->tf_edx;
	regs->r_ecx = tp->tf_ecx;
	regs->r_eax = tp->tf_eax;
	regs->r_eip = tp->tf_eip;
	regs->r_cs = tp->tf_cs;
	regs->r_eflags = tp->tf_eflags;
	regs->r_esp = tp->tf_esp;
	regs->r_ss = tp->tf_ss;

	return (0);
}

int
process_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	void *ptr;
	struct trapframe *tp;
	u_int eflags;

	if ((p->p_flag & SLOAD) == 0)
		return (EIO);

	ptr = (char *)p->p_addr + ((char *) p->p_regs - (char *) kstack);

	eflags = regs->r_eflags;
#ifdef notdef	/* BSDI's got a per-proc bitmap possible, apparently */
	if ((eflags & PSL_USERCLR) != 0 ||
	    (eflags & PSL_USERSET) != PSL_USERSET ||
	    (eflags & PSL_IOPL &&
		(p->p_md.md_flags & MDP_IOPL) == 0))
			return (EPERM);
#else	/* XXX - MORE THAN THIS! */
	if ((eflags & PSL_USERCLR) != 0 ||
	    (eflags & PSL_USERSET) != PSL_USERSET)
		return (EPERM);
#endif

	tp = ptr;
	tp->tf_es = regs->r_es;
	tp->tf_ds = regs->r_ds;
	tp->tf_edi = regs->r_edi;
	tp->tf_esi = regs->r_esi;
	tp->tf_ebp = regs->r_ebp;
	tp->tf_ebx = regs->r_ebx;
	tp->tf_edx = regs->r_edx;
	tp->tf_ecx = regs->r_ecx;
	tp->tf_eax = regs->r_eax;
	tp->tf_eip = regs->r_eip;
	tp->tf_cs = regs->r_cs;
	tp->tf_eflags = eflags;
	tp->tf_esp = regs->r_esp;
	tp->tf_ss = regs->r_ss;

	return (0);
}

int
process_sstep(p)
	struct proc *p;
{
	int error;
	struct reg r;

	error = process_read_regs(p, &r);
	if (error == 0) {
		r.r_eflags |= PSL_T;
		error = process_write_regs(p, &r);
	}

	return (error);
}

int
process_fix_sstep(p)
	struct proc *p;
{
	return 0;
}

int
process_set_pc(p, addr)
	struct proc *p;
	u_int addr;
{
	int error;
	struct reg r;

	error = process_read_regs(p, &r);
	if (error == 0) {
		r.r_eip = addr;
		error = process_write_regs(p, &r);
	}

	return (error);
}
