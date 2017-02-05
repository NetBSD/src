/*	$NetBSD: process_machdep.c,v 1.29.6.1 2017/02/05 13:40:01 skrll Exp $	*/

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
 *
 * process_count_watchpoints(proc, retval)
 *	Return the number of supported hardware watchpoints.
 *
 * process_read_watchpoint(proc, watchpoint)
 *	Read hardware watchpoint of the given index.
 *
 * process_write_watchpoint(proc, watchpoint)
 *	Write hardware watchpoint of the given index.
 *
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.29.6.1 2017/02/05 13:40:01 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>

#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/segments.h>
#include <x86/dbregs.h>
#include <x86/fpu.h>

static inline struct trapframe *process_frame(struct lwp *);
#if 0
static inline int verr_gdt(struct pmap *, int sel);
static inline int verr_ldt(struct pmap *, int sel);
#endif

static inline struct trapframe *
process_frame(struct lwp *l)
{

	return (l->l_md.md_regs);
}

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe *tf = process_frame(l);

#define copy_to_reg(reg, REG, idx) regs->regs[_REG_##REG] = tf->tf_##reg;
	_FRAME_GREG(copy_to_reg)
#undef copy_to_reg

	return (0);
}

int
process_read_fpregs(struct lwp *l, struct fpreg *regs, size_t *sz)
{

	process_read_fpregs_xmm(l, &regs->fxstate);

	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regp)
{
	struct trapframe *tf = process_frame(l);
	int error;
	const long *regs = regp->regs;

	/*
	 * Check for security violations.
	 * Note that struct regs is compatible with
	 * the __gregs array in mcontext_t.
	 */
	error = cpu_mcontext_validate(l, (const mcontext_t *)regs);
	if (error != 0)
		return error;

#define copy_to_frame(reg, REG, idx) tf->tf_##reg = regs[_REG_##REG];
	_FRAME_GREG(copy_to_frame)
#undef copy_to_frame

	return (0);
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *regs, size_t sz)
{

	process_write_fpregs_xmm(l, &regs->fxstate);
	return 0;
}

int
process_sstep(struct lwp *l, int sstep)
{
	struct trapframe *tf = process_frame(l);

	if (sstep)
		tf->tf_rflags |= PSL_T;
	else
		tf->tf_rflags &= ~PSL_T;
	
	return (0);
}

int
process_set_pc(struct lwp *l, void *addr)
{
	struct trapframe *tf = process_frame(l);

	if ((uint64_t)addr > VM_MAXUSER_ADDRESS)
		return EINVAL;
	tf->tf_rip = (uint64_t)addr;

	return (0);
}

int
process_count_watchpoints(struct lwp *l, register_t *retval)
{

	*retval = X86_HW_WATCHPOINTS;

	return (0);
}

int
process_read_watchpoint(struct lwp *l, struct ptrace_watchpoint *pw)
{

	pw->pw_type = PTRACE_PW_TYPE_DBREGS;
	pw->pw_md.md_address =
	    (void*)(intptr_t)l->l_md.md_watchpoint[pw->pw_index].address;
	pw->pw_md.md_condition = l->l_md.md_watchpoint[pw->pw_index].condition;
	pw->pw_md.md_length = l->l_md.md_watchpoint[pw->pw_index].length;

	return (0);
}

static void
update_mdl_x86_hw_watchpoints(struct lwp *l)
{
	size_t i;

	for (i = 0; i < X86_HW_WATCHPOINTS; i++) {
		if (l->l_md.md_watchpoint[0].address != 0) {
			return;
		}
	}
	l->l_md.md_flags &= ~MDL_X86_HW_WATCHPOINTS;
}

int
process_write_watchpoint(struct lwp *l, struct ptrace_watchpoint *pw)
{

	if (pw->pw_index > X86_HW_WATCHPOINTS)
		return (EINVAL);

	if (pw->pw_type != PTRACE_PW_TYPE_DBREGS)
		return (EINVAL);

	if (pw->pw_md.md_address == 0) {
		l->l_md.md_watchpoint[pw->pw_index].address = 0;
		update_mdl_x86_hw_watchpoints(l);
		return (0);
	}

	if ((vaddr_t)pw->pw_md.md_address > VM_MAXUSER_ADDRESS)
		return (EINVAL);

	switch (pw->pw_md.md_condition) {
	case X86_HW_WATCHPOINT_DR7_CONDITION_EXECUTION:
	case X86_HW_WATCHPOINT_DR7_CONDITION_DATA_WRITE:
	case X86_HW_WATCHPOINT_DR7_CONDITION_DATA_READWRITE:
		break;
	default:
		return (EINVAL);
	}

	switch (pw->pw_md.md_length) {
	case X86_HW_WATCHPOINT_DR7_LENGTH_BYTE:
	case X86_HW_WATCHPOINT_DR7_LENGTH_TWOBYTES:
	case X86_HW_WATCHPOINT_DR7_LENGTH_FOURBYTES:
		break;
	default:
		return (EINVAL);
	}

	l->l_md.md_watchpoint[pw->pw_index].address =
	    (vaddr_t)pw->pw_md.md_address;
	l->l_md.md_watchpoint[pw->pw_index].condition = pw->pw_md.md_condition;
	l->l_md.md_watchpoint[pw->pw_index].length = pw->pw_md.md_length;

	l->l_md.md_flags |= MDL_X86_HW_WATCHPOINTS;

	return (0);
}
