/*	$NetBSD: process_machdep.c,v 1.38 2017/12/01 21:22:45 maxv Exp $	*/

/*
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
 * process_read_fpregs(proc, regs, sz)
 *	Get the current user-visible register set from the process
 *	and copy it into the regs structure (<machine/reg.h>).
 *	The process is stopped at the time read_fpregs is called.
 *
 * process_write_fpregs(proc, regs, sz)
 *	Update the current register set from the passed in regs
 *	structure.  Take care to avoid clobbering special CPU
 *	registers or privileged bits in the PSL.
 *	The process is stopped at the time write_fpregs is called.
 *
 * process_read_dbregs(proc, regs, sz)
 *	Get the current user-visible register set from the process
 *	and copy it into the regs structure (<machine/reg.h>).
 *	The process is stopped at the time read_dbregs is called.
 *
 * process_write_dbregs(proc, regs, sz)
 *	Update the current register set from the passed in regs
 *	structure.  Take care to avoid clobbering special CPU
 *	registers or privileged bits in the PSL.
 *	The process is stopped at the time write_dbregs is called.
 *
 * process_sstep(proc)
 *	Arrange for the process to trap after executing a single instruction.
 *
 * process_set_pc(proc)
 *	Set the process's program counter.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.38 2017/12/01 21:22:45 maxv Exp $");

#include "opt_xen.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/ptrace.h>

#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/segments.h>
#include <x86/dbregs.h>
#include <x86/fpu.h>

static inline struct trapframe *process_frame(struct lwp *);

static inline struct trapframe *
process_frame(struct lwp *l)
{

	return l->l_md.md_regs;
}

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe *tf = process_frame(l);
	struct proc *p = l->l_proc;

	if (p->p_flag & PK_32) {
		return EINVAL;
	}

	regs->regs[_REG_RDI] = tf->tf_rdi;
	regs->regs[_REG_RSI] = tf->tf_rsi;
	regs->regs[_REG_RDX] = tf->tf_rdx;
	regs->regs[_REG_R10] = tf->tf_r10;
	regs->regs[_REG_R8]  = tf->tf_r8;
	regs->regs[_REG_R9]  = tf->tf_r9;
	/* argX not touched */
	regs->regs[_REG_RCX] = tf->tf_rcx;
	regs->regs[_REG_R11] = tf->tf_r11;
	regs->regs[_REG_R12] = tf->tf_r12;
	regs->regs[_REG_R13] = tf->tf_r13;
	regs->regs[_REG_R14] = tf->tf_r14;
	regs->regs[_REG_R15] = tf->tf_r15;
	regs->regs[_REG_RBP] = tf->tf_rbp;
	regs->regs[_REG_RBX] = tf->tf_rbx;
	regs->regs[_REG_RAX] = tf->tf_rax;
	regs->regs[_REG_GS]  = 0;
	regs->regs[_REG_FS]  = 0;
	regs->regs[_REG_ES]  = GSEL(GUDATA_SEL, SEL_UPL);
	regs->regs[_REG_DS]  = GSEL(GUDATA_SEL, SEL_UPL);
	regs->regs[_REG_TRAPNO] = tf->tf_trapno;
	regs->regs[_REG_ERR] = tf->tf_err;
	regs->regs[_REG_RIP] = tf->tf_rip;
	regs->regs[_REG_CS]  = LSEL(LUCODE_SEL, SEL_UPL);
	regs->regs[_REG_RFLAGS] = tf->tf_rflags;
	regs->regs[_REG_RSP] = tf->tf_rsp;
	regs->regs[_REG_SS]  = LSEL(LUDATA_SEL, SEL_UPL);

	return 0;
}

int
process_read_fpregs(struct lwp *l, struct fpreg *regs, size_t *sz)
{
	struct proc *p = l->l_proc;

	if (p->p_flag & PK_32) {
		return EINVAL;
	}

	process_read_fpregs_xmm(l, &regs->fxstate);

	return 0;
}

int
process_read_dbregs(struct lwp *l, struct dbreg *regs, size_t *sz)
{
	struct proc *p = l->l_proc;

	if (p->p_flag & PK_32) {
		return EINVAL;
	}

	x86_dbregs_read(l, regs);

	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regp)
{
	struct trapframe *tf = process_frame(l);
	struct proc *p = l->l_proc;
	int error;
	const long *regs = regp->regs;

	if (p->p_flag & PK_32) {
		return EINVAL;
	}

	/*
	 * Check for security violations. Note that struct regs is compatible
	 * with the __gregs array in mcontext_t.
	 */
	error = cpu_mcontext_validate(l, (const mcontext_t *)regs);
	if (error != 0)
		return error;

	tf->tf_rdi  = regs[_REG_RDI];
	tf->tf_rsi  = regs[_REG_RSI];
	tf->tf_rdx  = regs[_REG_RDX];
	tf->tf_r10  = regs[_REG_R10];
	tf->tf_r8   = regs[_REG_R8];
	tf->tf_r9   = regs[_REG_R9];
	/* argX not touched */
	tf->tf_rcx  = regs[_REG_RCX];
	tf->tf_r11  = regs[_REG_R11];
	tf->tf_r12  = regs[_REG_R12];
	tf->tf_r13  = regs[_REG_R13];
	tf->tf_r14  = regs[_REG_R14];
	tf->tf_r15  = regs[_REG_R15];
	tf->tf_rbp  = regs[_REG_RBP];
	tf->tf_rbx  = regs[_REG_RBX];
	tf->tf_rax  = regs[_REG_RAX];
	tf->tf_gs   = 0;
	tf->tf_fs   = 0;
	tf->tf_es   = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds   = GSEL(GUDATA_SEL, SEL_UPL);
	/* trapno, err not touched */
	tf->tf_rip  = regs[_REG_RIP];
	tf->tf_cs   = LSEL(LUCODE_SEL, SEL_UPL);
	tf->tf_rflags = regs[_REG_RFLAGS];
	tf->tf_rsp  = regs[_REG_RSP];
	tf->tf_ss   = LSEL(LUDATA_SEL, SEL_UPL);

#ifdef XEN
	/* see comment in cpu_setmcontext */
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
#endif

	return 0;
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *regs, size_t sz)
{
	struct proc *p = l->l_proc;

	if (p->p_flag & PK_32) {
		return EINVAL;
	}

	process_write_fpregs_xmm(l, &regs->fxstate);
	return 0;
}

int
process_write_dbregs(struct lwp *l, const struct dbreg *regs, size_t sz)
{
	struct proc *p = l->l_proc;
	int error;

	if (p->p_flag & PK_32) {
		return EINVAL;
	}

	/*
	 * Check for security violations.
	 */
	error = x86_dbregs_validate(regs);
	if (error != 0)
		return error;

	x86_dbregs_write(l, regs);

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
	
	return 0;
}

int
process_set_pc(struct lwp *l, void *addr)
{
	struct trapframe *tf = process_frame(l);
	struct proc *p = l->l_proc;

	if (p->p_flag & PK_32) {
		return EINVAL;
	}

	if ((uint64_t)addr >= VM_MAXUSER_ADDRESS)
		return EINVAL;
	tf->tf_rip = (uint64_t)addr;

	return 0;
}
