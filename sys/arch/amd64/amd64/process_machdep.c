/*	$NetBSD: process_machdep.c,v 1.9.4.1 2007/03/12 05:46:21 rmind Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.9.4.1 2007/03/12 05:46:21 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>

#include <uvm/uvm_extern.h>

#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/segments.h>
#include <machine/fpu.h>

static inline struct trapframe *process_frame __P((struct lwp *));
static inline struct fxsave64 *process_fpframe __P((struct lwp *));
#if 0
static inline int verr_gdt __P((struct pmap *, int sel));
static inline int verr_ldt __P((struct pmap *, int sel));
#endif

static inline struct trapframe *
process_frame(l)
	struct lwp *l;
{

	return (l->l_md.md_regs);
}

static inline struct fxsave64 *
process_fpframe(l)
	struct lwp *l;
{

	return (&l->l_addr->u_pcb.pcb_savefpu.fp_fxsave);
}

int
process_read_regs(l, regs)
	struct lwp *l;
	struct reg *regs;
{
	struct trapframe *tf = process_frame(l);

	memcpy(regs, tf, sizeof (*regs));

	return (0);
}

int
process_read_fpregs(l, regs)
	struct lwp *l;
	struct fpreg *regs;
{
	struct fxsave64 *frame = process_fpframe(l);

	if (l->l_md.md_flags & MDP_USEDFPU) {
		fpusave_lwp(l, 1);
	} else {
		u_int16_t cw;
		u_int32_t mxcsr, mxcsr_mask;

		/*
		 * Fake a FNINIT.
		 * The initial control word was already set by setregs(), so
		 * save it temporarily.
		 */
		cw = frame->fx_fcw;
		mxcsr = frame->fx_mxcsr;
		mxcsr_mask = frame->fx_mxcsr_mask;
		memset(frame, 0, sizeof(*regs));
		frame->fx_fcw = cw;
		frame->fx_fsw = 0x0000;
		frame->fx_ftw = 0xff;
		frame->fx_mxcsr = mxcsr;
		frame->fx_mxcsr_mask = mxcsr_mask;
		l->l_md.md_flags |= MDP_USEDFPU;
	}

	memcpy(&regs->fxstate, frame, sizeof(*regs));
	return (0);
}

int
process_write_regs(l, regp)
	struct lwp *l;
	const struct reg *regp;
{
	struct trapframe *tf = process_frame(l);
	int error;
	const long *regs = regp->regs;

	/*
	 * Check for security violations.
	 * Note that struct regs is compatible with
	 * the __gregs array in mcontext_t.
	 */
	error = check_mcontext(l, (const mcontext_t *)regs, tf);
	if (error != 0)
		return error;

	memcpy(tf, regs, sizeof (*tf));

	return (0);
}

int
process_write_fpregs(l, regs)
	struct lwp *l;
	const struct fpreg *regs;
{
	struct fxsave64 *frame = process_fpframe(l);

	if (l->l_md.md_flags & MDP_USEDFPU) {
		fpusave_lwp(l, 0);
	} else {
		l->l_md.md_flags |= MDP_USEDFPU;
	}

	memcpy(frame, &regs->fxstate, sizeof(*regs));
	return (0);
}

int
process_sstep(l, sstep)
	struct lwp *l;
{
	struct trapframe *tf = process_frame(l);

	if (sstep)
		tf->tf_rflags |= PSL_T;
	else
		tf->tf_rflags &= ~PSL_T;
	
	return (0);
}

int
process_set_pc(l, addr)
	struct lwp *l;
	void *addr;
{
	struct trapframe *tf = process_frame(l);

	if ((u_int64_t)addr > VM_MAXUSER_ADDRESS)
		return EINVAL;
	tf->tf_rip = (u_int64_t)addr;

	return (0);
}
