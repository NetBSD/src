/*	$NetBSD: process_machdep.c,v 1.3 2002/05/28 23:11:39 fvdl Exp $	*/

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

static __inline struct trapframe *process_frame __P((struct proc *));
static __inline struct fxsave64 *process_fpframe __P((struct proc *));
#if 0
static __inline int verr_gdt __P((struct pmap *, int sel));
static __inline int verr_ldt __P((struct pmap *, int sel));
#endif

static __inline struct trapframe *
process_frame(p)
	struct proc *p;
{

	return (p->p_md.md_regs);
}

static __inline struct fxsave64 *
process_fpframe(p)
	struct proc *p;
{

	return (&p->p_addr->u_pcb.pcb_savefpu);
}

int
process_read_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct trapframe *tf = process_frame(p);

	regs->r_rflags = tf->tf_rflags;
	regs->r_rdi = tf->tf_rdi;
	regs->r_rsi = tf->tf_rsi;
	regs->r_rbp = tf->tf_rbp;
	regs->r_rbx = tf->tf_rbx;
	regs->r_rdx = tf->tf_rdx;
	regs->r_rcx = tf->tf_rcx;
	regs->r_rax = tf->tf_rax;
	regs->r_r8  = tf->tf_r8;
	regs->r_r9  = tf->tf_r9;
	regs->r_r10  = tf->tf_r10;
	regs->r_r11  = tf->tf_r11;
	regs->r_r12  = tf->tf_r12;
	regs->r_r13  = tf->tf_r13;
	regs->r_r14  = tf->tf_r14;
	regs->r_r15  = tf->tf_r15;
	regs->r_rip = tf->tf_rip;
	regs->r_cs = tf->tf_cs;
	regs->r_rsp = tf->tf_rsp;
	regs->r_ss = tf->tf_ss;

	return (0);
}

int
process_read_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct fxsave64 *frame = process_fpframe(p);

	if (p->p_md.md_flags & MDP_USEDFPU) {
		if (fpuproc == p)
			fpusave();
	} else {
		u_int16_t cw;

		/*
		 * Fake a FNINIT.
		 * The initial control word was already set by setregs(), so
		 * save it temporarily.
		 */
		cw = frame->fx_fcw;
		memset(frame, 0, sizeof(*regs));
		frame->fx_fcw = cw;
		frame->fx_fsw = 0x0000;
		frame->fx_ftw = 0xff;
		p->p_md.md_flags |= MDP_USEDFPU;
	}

	memcpy(&regs->fxstate, frame, sizeof(*regs));
	return (0);
}

#if 0
static __inline int
verr_ldt(pmap, sel)
	struct pmap *pmap;
	int sel;
{
	int off;
	struct mem_segment_descriptor *d;

	off = sel & 0xfff8;
	if (off > (pmap->pm_ldt_len - sizeof (struct mem_segment_descriptor)))
		return 0;
	d = (struct mem_segment_descriptor *)(ldtstore + off);
	return ((d->sd_type & SDT_MEMRO) != 0 && d->sd_dpl == SEL_UPL &&
		d->sd_p == 1);
}

static __inline int
verr_gdt(pmap, sel)
	struct pmap *pmap;
	int sel;
{
	int off;
	struct mem_segment_descriptor *d;

	off = sel & 0xfff8;
	if (off > (NGDT_MEM - 1) * sizeof (struct mem_segment_descriptor))
		return 0;
	d = (struct mem_segment_descriptor *)(gdtstore + off);
	return ((d->type & SDT_MEMRO) != 0 && d->sd_p == 1 &&
		d->dpl == SEL_UPL);
}

#define	verr(sel)	(ISLDT(sel) ? verr_ldt(IDXSEL(sel)) : \
				      verr_gdt(IDXSEL(sel)))
#define	valid_sel(sel)	(ISPL(sel) == SEL_UPL && verr(sel))
#define	null_sel(sel)	(!ISLDT(sel) && IDXSEL(sel) == 0)

#endif

int
process_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct trapframe *tf = process_frame(p);
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
#if 0
	union descriptor *gdt = (union descriptor *)gdtstore;
#endif

	/*
	 * Check for security violations.
	 */
	if (((regs->r_rflags ^ tf->tf_rflags) & PSL_USERSTATIC) != 0 ||
	    !USERMODE(regs->r_cs, regs->r_rflags))
		return (EINVAL);

	simple_lock(&pmap->pm_lock);

#if 0
	/*
	 * fs and gs contents ignored by long mode.
	 * must reenable this check for 32bit compat mode.
	 */
	if ((regs->r_gs != pcb->pcb_gs && \
	     !valid_sel(regs->r_gs) && !null_sel(regs->r_gs)) ||
	    (regs->r_fs != pcb->pcb_fs && \
	     !valid_sel(regs->r_fs) && !null_sel(regs->r_fs)))
		return (EINVAL);
#endif

	simple_unlock(&pmap->pm_lock);

	tf->tf_rflags = regs->r_rflags;
	tf->tf_r15 = regs->r_r15;
	tf->tf_r14 = regs->r_r14;
	tf->tf_r13 = regs->r_r13;
	tf->tf_r12 = regs->r_r12;
	tf->tf_r11 = regs->r_r11;
	tf->tf_r10 = regs->r_r10;
	tf->tf_r9 = regs->r_r9;
	tf->tf_r8 = regs->r_r8;
	tf->tf_rdi = regs->r_rdi;
	tf->tf_rsi = regs->r_rsi;
	tf->tf_rbp = regs->r_rbp;
	tf->tf_rbx = regs->r_rbx;
	tf->tf_rdx = regs->r_rdx;
	tf->tf_rcx = regs->r_rcx;
	tf->tf_rax = regs->r_rax;
	tf->tf_rip = regs->r_rip;
	tf->tf_cs = regs->r_cs;
	tf->tf_rsp = regs->r_rsp;
	tf->tf_ss = regs->r_ss;

	return (0);
}

int
process_write_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct fxsave64 *frame = process_fpframe(p);

	if (p->p_md.md_flags & MDP_USEDFPU) {
		if (fpuproc == p)
			fpudrop();
	} else {
		p->p_md.md_flags |= MDP_USEDFPU;
	}

	memcpy(frame, regs, sizeof(*regs));
	return (0);
}

int
process_sstep(p, sstep)
	struct proc *p;
{
	struct trapframe *tf = process_frame(p);

	if (sstep)
		tf->tf_rflags |= PSL_T;
	else
		tf->tf_rflags &= ~PSL_T;
	
	return (0);
}

int
process_set_pc(p, addr)
	struct proc *p;
	caddr_t addr;
{
	struct trapframe *tf = process_frame(p);

	tf->tf_rip = (u_int64_t)addr;

	return (0);
}
