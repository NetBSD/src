/* $NetBSD: process_machdep.c,v 1.21 2003/09/21 15:14:51 skd Exp $ */

/*
 * Copyright (c) 1994 Christopher G. Demetriou
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.21 2003/09/21 15:14:51 skd Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/alpha.h>

#define	lwp_frame(l)	((l)->l_md.md_tf)
#define	lwp_pcb(l)	(&(l)->l_addr->u_pcb)
#define	lwp_fpframe(l)	(&(lwp_pcb(l)->pcb_fp))

int
process_read_regs(struct lwp *l, struct reg *regs)
{

	frametoreg(lwp_frame(l), regs);
	regs->r_regs[R_ZERO] = lwp_frame(l)->tf_regs[FRAME_PC];
	regs->r_regs[R_SP] = lwp_pcb(l)->pcb_hw.apcb_usp;
	return (0);
}

int
process_write_regs(struct lwp *l, struct reg *regs)
{

	regtoframe(regs, lwp_frame(l));
	lwp_frame(l)->tf_regs[FRAME_PC] = regs->r_regs[R_ZERO];
	lwp_pcb(l)->pcb_hw.apcb_usp = regs->r_regs[R_SP];
	return (0);
}

int
process_sstep(struct lwp *l, int sstep)
{

	if (sstep)
		return (EINVAL);

	return (0);
}

int
process_set_pc(struct lwp *l, caddr_t addr)
{
	struct trapframe *frame = lwp_frame(l);

	frame->tf_regs[FRAME_PC] = (u_int64_t)addr;
	return (0);
}

int
process_read_fpregs(struct lwp *l, struct fpreg *regs)
{

	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_proc(l, 1);

	memcpy(regs, lwp_fpframe(l), sizeof(struct fpreg));
	return (0);
}

int
process_write_fpregs(struct lwp *l, struct fpreg *regs)
{

	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_proc(l, 0);

	memcpy(lwp_fpframe(l), regs, sizeof(struct fpreg));
	return (0);
}
