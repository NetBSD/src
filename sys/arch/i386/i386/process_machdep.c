/*	$NetBSD: process_machdep.c,v 1.44 2002/05/10 05:45:50 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by Jason R. Thorpe of Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.44 2002/05/10 05:45:50 thorpej Exp $");

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

static __inline struct trapframe *
process_frame(struct proc *p)
{

	return (p->p_md.md_regs);
}

static __inline union savefpu *
process_fpframe(struct proc *p)
{

	return (&p->p_addr->u_pcb.pcb_savefpu);
}

static int
xmm_to_s87_tag(const uint8_t *fpac, int regno, uint8_t tw)
{
	static const uint8_t empty_significand[8] = { 0 };
	int tag;
	uint16_t exponent;

	if (tw & (1U << regno)) {
		exponent = fpac[8] | (fpac[9] << 8);
		switch (exponent) {
		case 0x7fff:
			tag = 2;
			break;

		case 0x0000:
			if (memcmp(empty_significand, fpac,
				   sizeof(empty_significand)) == 0)
				tag = 1;
			else
				tag = 2;
			break;

		default:
			if ((fpac[7] & 0x80) == 0)
				tag = 2;
			else
				tag = 0;
			break;
		}
	} else
		tag = 3;

	return (tag);
}

void
process_xmm_to_s87(const struct savexmm *sxmm, struct save87 *s87)
{
	int i;

	/* FPU control/status */
	s87->sv_env.en_cw = sxmm->sv_env.en_cw;
	s87->sv_env.en_sw = sxmm->sv_env.en_sw;
	/* tag word handled below */
	s87->sv_env.en_fip = sxmm->sv_env.en_fip;
	s87->sv_env.en_fcs = sxmm->sv_env.en_fcs;
	s87->sv_env.en_opcode = sxmm->sv_env.en_opcode;
	s87->sv_env.en_foo = sxmm->sv_env.en_foo;
	s87->sv_env.en_fos = sxmm->sv_env.en_fos;

	/* Tag word and registers. */
	s87->sv_env.en_tw = 0;
	s87->sv_ex_tw = 0;
	for (i = 0; i < 8; i++) {
		s87->sv_env.en_tw |=
		    (xmm_to_s87_tag(sxmm->sv_ac[i].fp_bytes, i,
		     sxmm->sv_env.en_tw) << (i * 2));

		s87->sv_ex_tw |=
		    (xmm_to_s87_tag(sxmm->sv_ac[i].fp_bytes, i,
		     sxmm->sv_ex_tw) << (i * 2));

		memcpy(&s87->sv_ac[i].fp_bytes, &sxmm->sv_ac[i].fp_bytes,
		    sizeof(s87->sv_ac[i].fp_bytes));
	}

	s87->sv_ex_sw = sxmm->sv_ex_sw;
}

void
process_s87_to_xmm(const struct save87 *s87, struct savexmm *sxmm)
{
	int i;

	/* FPU control/status */
	sxmm->sv_env.en_cw = s87->sv_env.en_cw;
	sxmm->sv_env.en_sw = s87->sv_env.en_sw;
	/* tag word handled below */
	sxmm->sv_env.en_fip = s87->sv_env.en_fip;
	sxmm->sv_env.en_fcs = s87->sv_env.en_fcs;
	sxmm->sv_env.en_opcode = s87->sv_env.en_opcode;
	sxmm->sv_env.en_foo = s87->sv_env.en_foo;
	sxmm->sv_env.en_fos = s87->sv_env.en_fos;

	/* Tag word and registers. */
	for (i = 0; i < 8; i++) {
		if (((s87->sv_env.en_tw >> (i * 2)) & 3) == 3)
			sxmm->sv_env.en_tw &= ~(1U << i);
		else
			sxmm->sv_env.en_tw |= (1U << i);

#if 0
		/*
		 * Software-only word not provided by the userland fpreg
		 * structure.
		 */
		if (((s87->sv_ex_tw >> (i * 2)) & 3) == 3)
			sxmm->sv_ex_tw &= ~(1U << i);
		else
			sxmm->sv_ex_tw |= (1U << i);
#endif

		memcpy(&sxmm->sv_ac[i].fp_bytes, &s87->sv_ac[i].fp_bytes,
		    sizeof(sxmm->sv_ac[i].fp_bytes));
	}
#if 0
	/*
	 * Software-only word not provided by the userland fpreg
	 * structure.
	 */
	sxmm->sv_ex_sw = s87->sv_ex_sw;
#endif
}

int
process_read_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct trapframe *tf = process_frame(p);

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
		regs->r_gs = tf->tf_gs & 0xffff;
		regs->r_fs = tf->tf_fs & 0xffff;
		regs->r_es = tf->tf_es & 0xffff;
		regs->r_ds = tf->tf_ds & 0xffff;
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
	regs->r_cs = tf->tf_cs & 0xffff;
	regs->r_esp = tf->tf_esp;
	regs->r_ss = tf->tf_ss & 0xffff;

	return (0);
}

int
process_read_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	union savefpu *frame = process_fpframe(p);

	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		extern struct proc *npxproc;

		if (npxproc == p)
			npxsave();
#endif
	} else {
		/*
		 * Fake a FNINIT.
		 * The initial control word was already set by setregs(), so
		 * save it temporarily.
		 */
		if (i386_use_fxsave) {
			uint32_t mxcsr = frame->sv_xmm.sv_env.en_mxcsr;
			uint16_t cw = frame->sv_xmm.sv_env.en_cw;

			/* XXX Don't zero XMM regs? */
			memset(&frame->sv_xmm, 0, sizeof(frame->sv_xmm));
			frame->sv_xmm.sv_env.en_cw = cw;
			frame->sv_xmm.sv_env.en_mxcsr = mxcsr;
			frame->sv_xmm.sv_env.en_sw = 0x0000;
			frame->sv_xmm.sv_env.en_tw = 0x00;
		} else {
			uint16_t cw = frame->sv_87.sv_env.en_cw;

			memset(&frame->sv_87, 0, sizeof(frame->sv_87));
			frame->sv_87.sv_env.en_cw = cw;
			frame->sv_87.sv_env.en_sw = 0x0000;
			frame->sv_87.sv_env.en_tw = 0xffff;
		}
		p->p_md.md_flags |= MDP_USEDFPU;
	}

	if (i386_use_fxsave) {
		struct save87 s87;

		/* XXX Yuck */
		process_xmm_to_s87(&frame->sv_xmm, &s87);
		memcpy(regs, &s87, sizeof(*regs));
	} else
		memcpy(regs, &frame->sv_87, sizeof(*regs));
	return (0);
}

int
process_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct trapframe *tf = process_frame(p);

#ifdef VM86
	if (regs->r_eflags & PSL_VM) {
		void syscall_vm86 __P((struct trapframe));

		tf->tf_vm86_gs = regs->r_gs;
		tf->tf_vm86_fs = regs->r_fs;
		tf->tf_vm86_es = regs->r_es;
		tf->tf_vm86_ds = regs->r_ds;
		set_vflags(p, regs->r_eflags);
		p->p_md.md_syscall = syscall_vm86;
	} else
#endif
	{
		/*
		 * Check for security violations.
		 */
		if (((regs->r_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
		    !USERMODE(regs->r_cs, regs->r_eflags))
			return (EINVAL);

		tf->tf_gs = regs->r_gs;
		tf->tf_fs = regs->r_fs;
		tf->tf_es = regs->r_es;
		tf->tf_ds = regs->r_ds;
#ifdef VM86
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
	union savefpu *frame = process_fpframe(p);

	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		extern struct proc *npxproc;

		if (npxproc == p)
			npxdrop();
#endif
	} else {
		p->p_md.md_flags |= MDP_USEDFPU;
	}

	if (i386_use_fxsave) {
		struct save87 s87;

		/* XXX Yuck. */
		memcpy(&s87, regs, sizeof(*regs));
		process_s87_to_xmm(&s87, &frame->sv_xmm);
	} else
		memcpy(&frame->sv_87, regs, sizeof(*regs));
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

#ifdef __HAVE_PTRACE_MACHDEP
static int
process_machdep_read_xmmregs(struct proc *p, struct xmmregs *regs)
{
	union savefpu *frame = process_fpframe(p);

	if (i386_use_fxsave == 0)
		return (EINVAL);

	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		extern struct proc *npxproc;

		if (npxproc == p)
			npxsave();
#endif
	} else {
		/*
		 * Fake a FNINIT.
		 * The initial control word was already set by setregs(),
		 * so save it temporarily.
		 */
		uint32_t mxcsr = frame->sv_xmm.sv_env.en_mxcsr;
		uint16_t cw = frame->sv_xmm.sv_env.en_cw;

		/* XXX Don't zero XMM regs? */
		memset(&frame->sv_xmm, 0, sizeof(frame->sv_xmm));
		frame->sv_xmm.sv_env.en_cw = cw;
		frame->sv_xmm.sv_env.en_mxcsr = mxcsr;
		frame->sv_xmm.sv_env.en_sw = 0x0000;
		frame->sv_xmm.sv_env.en_tw = 0x00;

		p->p_md.md_flags |= MDP_USEDFPU;  
	}

	memcpy(regs, &frame->sv_xmm, sizeof(*regs));
	return (0);
}

static int
process_machdep_write_xmmregs(struct proc *p, struct xmmregs *regs)
{
	union savefpu *frame = process_fpframe(p);

	if (i386_use_fxsave == 0)
		return (EINVAL);

	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		extern struct proc *npxproc;

		if (npxproc == p)
			npxdrop();
#endif
	} else {
		p->p_md.md_flags |= MDP_USEDFPU;
	}

	memcpy(&frame->sv_xmm, regs, sizeof(*regs));
	return (0);
}

int
ptrace_machdep_dorequest(p, t, req, addr, data)
	struct proc *p, *t;
	int req;
	caddr_t addr;
	int data;
{
	struct uio uio;
	struct iovec iov;
	int write = 0;

	switch (req) {
	case PT_SETXMMREGS:
		write = 1;

	case PT_GETXMMREGS:
		/* write = 0 done above. */
		if (!process_machdep_validxmmregs(t))
			return (EINVAL);
		else {
			iov.iov_base = addr;
			iov.iov_len = sizeof(struct xmmregs);
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_offset = 0;
			uio.uio_resid = sizeof(struct xmmregs);
			uio.uio_segflg = UIO_USERSPACE;
			uio.uio_rw = write ? UIO_WRITE : UIO_READ;
			uio.uio_procp = p;
			return (process_machdep_doxmmregs(p, t, &uio));
		}
	}

#ifdef DIAGNOSTIC
	panic("ptrace_machdep: impossible");
#endif

	return (0);
}

/*
 * The following functions are used by both ptrace(2) and procfs.
 */

int
process_machdep_doxmmregs(curp, p, uio)
	struct proc *curp;		/* tracer */
	struct proc *p;			/* traced */
	struct uio *uio;
{
	int error;
	struct xmmregs r;
	char *kv;
	int kl;

	if ((error = process_checkioperm(curp, p)) != 0)
		return (error);

	kl = sizeof(r);
	kv = (char *) &r;

	kv += uio->uio_offset;
	kl -= uio->uio_offset;
	if (kl > uio->uio_resid)
		kl = uio->uio_resid;

	PHOLD(p);

	if (kl < 0)
		error = EINVAL;
	else
		error = process_machdep_read_xmmregs(p, &r);
	if (error == 0)
		error = uiomove(kv, kl, uio);
	if (error == 0 && uio->uio_rw == UIO_WRITE) {
		if (p->p_stat != SSTOP)
			error = EBUSY;
		else
			error = process_machdep_write_xmmregs(p, &r);
	}

	PRELE(p);

	uio->uio_offset = 0;
	return (error);
}

int
process_machdep_validxmmregs(p)
	struct proc *p;
{

	if (p->p_flag & P_SYSTEM)
		return (0);

	return (i386_use_fxsave);
}
#endif /* __HAVE_PTRACE_MACHDEP */
