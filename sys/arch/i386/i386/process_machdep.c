/*	$NetBSD: process_machdep.c,v 1.30.10.4 2001/01/07 22:12:44 sommerfeld Exp $	*/

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

#include "opt_vm86.h"
#include "npx.h"

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

#ifdef VM86
#include <machine/vm86.h>
#endif

static __inline struct trapframe *process_frame __P((struct proc *));
static __inline struct save87 *process_fpframe __P((struct proc *));

static __inline struct trapframe *
process_frame(p)
	struct proc *p;
{

	return (p->p_md.md_regs);
}

static __inline struct save87 *
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
	struct pcb *pcb = &p->p_addr->u_pcb;

#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		regs->r_gs = tf->tf_vm86_gs;
		regs->r_fs = tf->tf_vm86_fs;
		regs->r_es = tf->tf_vm86_es;
		regs->r_ds = tf->tf_vm86_ds;
		regs->r_eflags = get_vflags(p);
	} else
#endif
	{
		regs->r_gs = pcb->pcb_gs;
		regs->r_fs = pcb->pcb_fs;
		regs->r_es = tf->tf_es;
		regs->r_ds = tf->tf_ds;
		regs->r_eflags = tf->tf_eflags;
	}
	regs->r_edi = tf->tf_edi;
	regs->r_esi = tf->tf_esi;
	regs->r_ebp = tf->tf_ebp;
	regs->r_ebx = tf->tf_ebx;
	regs->r_edx = tf->tf_edx;
	regs->r_ecx = tf->tf_ecx;
	regs->r_eax = tf->tf_eax;
	regs->r_eip = tf->tf_eip;
	regs->r_cs = tf->tf_cs;
	regs->r_esp = tf->tf_esp;
	regs->r_ss = tf->tf_ss;

	return (0);
}

int
process_read_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct save87 *frame = process_fpframe(p);

	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		npxsave_proc(p, 1);
#endif
	} else {
		u_short cw;

		/*
		 * Fake a FNINIT.
		 * The initial control word was already set by setregs(), so
		 * save it temporarily.
		 */
		cw = frame->sv_env.en_cw;
		memset(frame, 0, sizeof(*regs));
		frame->sv_env.en_cw = cw;
		frame->sv_env.en_sw = 0x0000;
		frame->sv_env.en_tw = 0xffff;
		p->p_md.md_flags |= MDP_USEDFPU;
	}

	memcpy(regs, frame, sizeof(*regs));
	return (0);
}

int
process_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct trapframe *tf = process_frame(p);
	struct pcb *pcb = &p->p_addr->u_pcb;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;

#ifdef VM86
	if (regs->r_eflags & PSL_VM) {
		void syscall_vm86 __P((struct trapframe));

		tf->tf_vm86_gs = regs->r_gs;
		tf->tf_vm86_fs = regs->r_fs;
		tf->tf_vm86_es = regs->r_es;
		tf->tf_vm86_ds = regs->r_ds;
		set_vflags(p, regs->r_eflags);
		/*
		 * Make sure that attempts at system calls from vm86
		 * mode die horribly.
		 */
		p->p_md.md_syscall = syscall_vm86;
	} else
#endif
	{
#define	verr_ldt(slot)	(slot < pmap->pm_ldt_len && \
			 (pmap->pm_ldt[slot].sd.sd_type & SDT_MEMRO) != 0 && \
			 pmap->pm_ldt[slot].sd.sd_dpl == SEL_UPL && \
			 pmap->pm_ldt[slot].sd.sd_p == 1)
#define	verr_gdt(slot)	(slot < NGDT && \
			 (gdt[slot].sd.sd_type & SDT_MEMRO) != 0 && \
			 gdt[slot].sd.sd_dpl == SEL_UPL && \
			 gdt[slot].sd.sd_p == 1)
#define	verr(sel)	(ISLDT(sel) ? verr_ldt(IDXSEL(sel)) : \
				      verr_gdt(IDXSEL(sel)))
#define	valid_sel(sel)	(ISPL(sel) == SEL_UPL && verr(sel))
#define	null_sel(sel)	(!ISLDT(sel) && IDXSEL(sel) == 0)

		/*
		 * Check for security violations.
		 */
		if (((regs->r_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
		    !USERMODE(regs->r_cs, regs->r_eflags))
			return (EINVAL);

		simple_lock(&pmap->pm_lock);

		if ((regs->r_gs != pcb->pcb_gs && \
		     !valid_sel(regs->r_gs) && !null_sel(regs->r_gs)) ||
		    (regs->r_fs != pcb->pcb_fs && \
		     !valid_sel(regs->r_fs) && !null_sel(regs->r_fs)))
			return (EINVAL);

		simple_unlock(&pmap->pm_lock);

		pcb->pcb_gs = regs->r_gs;
		pcb->pcb_fs = regs->r_fs;
		tf->tf_es = regs->r_es;
		tf->tf_ds = regs->r_ds;
#ifdef VM86
		/* Restore normal syscall handler */
		if (tf->tf_eflags & PSL_VM)
			(*p->p_emul->e_syscall_intern)(p);
#endif
		tf->tf_eflags = regs->r_eflags;
	}
	tf->tf_edi = regs->r_edi;
	tf->tf_esi = regs->r_esi;
	tf->tf_ebp = regs->r_ebp;
	tf->tf_ebx = regs->r_ebx;
	tf->tf_edx = regs->r_edx;
	tf->tf_ecx = regs->r_ecx;
	tf->tf_eax = regs->r_eax;
	tf->tf_eip = regs->r_eip;
	tf->tf_cs = regs->r_cs;
	tf->tf_esp = regs->r_esp;
	tf->tf_ss = regs->r_ss;

	return (0);
}

int
process_write_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct save87 *frame = process_fpframe(p);

	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		npxsave_proc(p, 0);
#endif
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
		tf->tf_eflags |= PSL_T;
	else
		tf->tf_eflags &= ~PSL_T;
	
	return (0);
}

int
process_set_pc(p, addr)
	struct proc *p;
	caddr_t addr;
{
	struct trapframe *tf = process_frame(p);

	tf->tf_eip = (int)addr;

	return (0);
}
