/* $NetBSD: process_machdep.c,v 1.20 2003/09/18 05:26:41 skd Exp $ */

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
 * process_read_regs(lwp, regs)
 *	Get the current user-visible register set from the process
 *	and copy it into the regs structure (<machine/reg.h>).
 *	The process is stopped at the time read_regs is called.
 *
 * process_write_regs(lwp, regs)
 *	Update the current register set from the passed in regs
 *	structure.  Take care to avoid clobbering special CPU
 *	registers or privileged bits in the PSL.
 *	The process is stopped at the time write_regs is called.
 *
 * process_sstep(lwp,int)
 *	Arrange for the process to trap after executing a single instruction.
 *
 * process_set_pc(lwp)
 *	Set the process's program counter.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.20 2003/09/18 05:26:41 skd Exp $");

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
#include <alpha/alpha/db_instruction.h>

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

static int
process_read_bpt(struct lwp *l, vaddr_t addr, u_int32_t *v)
{
	struct iovec iov;
	struct uio uio;
	iov.iov_base = (caddr_t) v;
	iov.iov_len = sizeof(u_int32_t);
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)addr;
	uio.uio_resid = sizeof(u_int32_t);
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_rw = UIO_READ;
	uio.uio_procp = l->l_proc;
	return process_domem(l->l_proc,l->l_proc, &uio);
}

static int
process_write_bpt(struct lwp *l, vaddr_t addr, u_int32_t v)
{
	struct iovec iov;
	struct uio uio;
	iov.iov_base = (caddr_t) &v;
	iov.iov_len = sizeof(u_int32_t);
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)addr;
	uio.uio_resid = sizeof(u_int32_t);
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_rw = UIO_WRITE;
	uio.uio_procp = l->l_proc;
	return process_domem(l->l_proc,l->l_proc, &uio);
}

static int
process_clear_bpt(struct lwp *l, struct mdbpt *bpt)
{
	return process_write_bpt(l, bpt->addr, bpt->contents);
}

static int
process_set_bpt(struct lwp *l, struct mdbpt *bpt)
{
	int error;
	u_int32_t bpins = 0x00000080;
	error = process_read_bpt(l, bpt->addr, &bpt->contents);
	if (error)
		return error;
	return process_write_bpt(l, bpt->addr, bpins);
}

static u_int64_t
process_read_register(struct lwp *l, int regno)
{
	static int reg_to_frame[32] = {
		FRAME_V0,
		FRAME_T0,
		FRAME_T1,
		FRAME_T2,
		FRAME_T3,
		FRAME_T4,
		FRAME_T5,
		FRAME_T6,
		FRAME_T7,
		
		FRAME_S0,
		FRAME_S1,
		FRAME_S2,
		FRAME_S3,
		FRAME_S4,
		FRAME_S5,
		FRAME_S6,

		FRAME_A0,
		FRAME_A1,
		FRAME_A2,
		FRAME_A3,
		FRAME_A4,
		FRAME_A5,

		FRAME_T8,
		FRAME_T9,
		FRAME_T10,
		FRAME_T11,
		FRAME_RA,
		FRAME_T12,
		FRAME_AT,
		FRAME_GP,
		FRAME_SP,
		-1,             /* zero */
	};
	
	if (regno == R_ZERO)
		return 0;
	
	return l->l_md.md_tf->tf_regs[reg_to_frame[regno]];
}

static int
process_clear_sstep(struct lwp *l)
{
	if (l->l_md.md_flags & MDP_STEP2) {
		process_clear_bpt(l, &l->l_md.md_sstep[1]);
		process_clear_bpt(l, &l->l_md.md_sstep[0]);
		l->l_md.md_flags &= ~MDP_STEP2;
	} else if (l->l_md.md_flags & MDP_STEP1) {
		process_clear_bpt(l, &l->l_md.md_sstep[0]);
		l->l_md.md_flags &= ~MDP_STEP1;
	}
	return 0;
}

int
process_sstep(struct lwp *l,int action)
{
	int error;
	struct trapframe *tf = l->l_md.md_tf;
	vaddr_t pc = tf->tf_regs[FRAME_PC];
	alpha_instruction ins;
	vaddr_t addr[2];    /* places to set breakpoints */
	int count = 0;          /* count of breakpoints */
	
	if ( action == 0 )
		return(process_clear_sstep(l));
	
	if (l->l_md.md_flags & (MDP_STEP1|MDP_STEP2))
		panic("process_sstep: step breakpoints not removed");
	
	error = process_read_bpt(l, pc, &ins.bits);
	if (error)
		return error;
	
	switch (ins.branch_format.opcode) {

	case op_j:
		/* Jump: target is register value */
		addr[0] = process_read_register(l, ins.jump_format.rb) & ~3;
		count = 1;
		break;

	case op_br:
	case op_fbeq:
	case op_fblt:
	case op_fble:
	case op_bsr:
	case op_fbne:
	case op_fbge:
	case op_fbgt:
	case op_blbc:
	case op_beq:
	case op_blt:
	case op_ble:
	case op_blbs:
	case op_bne:
	case op_bge:
	case op_bgt:
		/* Branch: target is pc+4+4*displacement */
		addr[0] = pc + 4;
		addr[1] = pc + 4 + 4 * ins.branch_format.displacement;
		count = 2;
		break;
		
	default:
		addr[0] = pc + 4;
		count = 1;
	}

	l->l_md.md_sstep[0].addr = addr[0];
	error = process_set_bpt(l, &l->l_md.md_sstep[0]);
	if (error)
		return error;
	if (count == 2) {
		l->l_md.md_sstep[1].addr = addr[1];
		error = process_set_bpt(l, &l->l_md.md_sstep[1]);
		if (error) {
			process_clear_bpt(l, &l->l_md.md_sstep[0]);
			return error;
		}
		l->l_md.md_flags |= MDP_STEP2;
	} else
		l->l_md.md_flags |= MDP_STEP1;
	
	return 0;
}

