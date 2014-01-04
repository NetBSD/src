/*	$NetBSD: process_machdep.c,v 1.30 2014/01/04 00:10:02 dsl Exp $	*/

/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
 * process_set_pc(proc)
 *	Set the process's program counter.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.30 2014/01/04 00:10:02 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>

#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/psl.h>

static inline struct frame *
process_frame(struct lwp *l)
{
	void *ptr;

	ptr = l->l_md.md_regs;
	return ptr;
}

static inline struct fpframe *
process_fpframe(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);

	return &pcb->pcb_fpregs;
}

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct frame *frame = process_frame(l);

	memcpy(regs->r_regs, frame->f_regs, sizeof(frame->f_regs));
	regs->r_sr = frame->f_sr;
	regs->r_pc = frame->f_pc;

	return 0;
}

int
process_read_fpregs(struct lwp *l, struct fpreg *regs, size_t *sz)
{
	struct fpframe *frame = process_fpframe(l);

	memcpy(regs->r_regs, frame->fpf_regs, sizeof(frame->fpf_regs));
	regs->r_fpcr = frame->fpf_fpcr;
	regs->r_fpsr = frame->fpf_fpsr;
	regs->r_fpiar = frame->fpf_fpiar;

	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regs)
{
	struct frame *frame = process_frame(l);

	/*
	 * in the hp300 machdep.c _write_regs, PC alignment wasn't
	 * checked.  If an odd address is placed in the PC and the
	 * program is allowed to run, it will cause an Address Error
	 * which will be transmitted to the process by a SIGBUS.
	 * No reasonable debugger would let this happen, but
	 * it's not our problem.
	 */

	/*
	 * XXX
	 * in hp300 machdep.c, it just cleared/set these bits
	 * automatically.  here, we barf.  well-written programs
	 * shouldn't munge them.
	 */
	if ((regs->r_sr & PSL_USERCLR) != 0 ||
	    (regs->r_sr & PSL_USERSET) != PSL_USERSET)
		return EPERM;

	memcpy(frame->f_regs, regs->r_regs, sizeof(frame->f_regs));
	frame->f_sr = regs->r_sr;
	frame->f_pc = regs->r_pc;

	return 0;
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *regs, size_t sz)
{
	struct fpframe *frame = process_fpframe(l);

	memcpy(frame->fpf_regs, regs->r_regs, sizeof(frame->fpf_regs));
	frame->fpf_fpcr = regs->r_fpcr;
	frame->fpf_fpsr = regs->r_fpsr;
	frame->fpf_fpiar = regs->r_fpiar;

	return 0;
}

int
process_sstep(struct lwp *l, int sstep)
{
	struct frame *frame = process_frame(l);

	if (sstep)
		frame->f_sr |= PSL_T;
	else
		frame->f_sr &= ~PSL_T;

	return 0;
}

int
process_set_pc(struct lwp *l, void *addr)
{
	struct frame *frame = process_frame(l);

	/*
	 * in the hp300 machdep.c _set_pc, PC alignment is guaranteed
	 * by chopping off the low order bit of the new pc.
	 * If an odd address was placed in the PC and the program
	 * is allowed to run, it will cause an Address Error
	 * which will be transmitted to the process by a SIGBUS.
	 * No reasonable debugger would let this happen, but
	 * it's not our problem.
	 */
	frame->f_pc = (u_int)addr;

	return 0;
}
