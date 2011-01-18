/*	$NetBSD: process_machdep.c,v 1.29 2011/01/18 01:02:55 matt Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.29 2011/01/18 01:02:55 matt Exp $");

#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/ptrace.h>

#include <machine/fpu.h>
#include <machine/pcb.h>
#include <machine/reg.h>

#include <uvm/uvm_extern.h>

#include <powerpc/altivec.h>

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe * const tf = trapframe(l);

	memcpy(regs->fixreg, tf->tf_fixreg, sizeof(regs->fixreg));
	regs->lr = tf->tf_lr;
	regs->cr = tf->tf_cr;
	regs->xer = tf->tf_xer;
	regs->ctr = tf->tf_ctr;
	regs->pc = tf->tf_srr0;

	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regs)
{
	struct trapframe * const tf = trapframe(l);

	memcpy(tf->tf_fixreg, regs->fixreg, sizeof(regs->fixreg));
	tf->tf_lr = regs->lr;
	tf->tf_cr = regs->cr;
	tf->tf_xer = regs->xer;
	tf->tf_ctr = regs->ctr;
	tf->tf_srr0 = regs->pc;

	return 0;
}

int
process_read_fpregs(struct lwp *l, struct fpreg *fpregs)
{
	struct pcb * const pcb = lwp_getpcb(l);

	/* Is the process using the fpu? */
	if ((l->l_md.md_flags & MDLWP_USEDFPU) == 0) {
		memset(fpregs, 0, sizeof (*fpregs));
		return 0;
	}

#ifdef PPC_HAVE_FPU
	fpu_save_lwp(l, FPU_SAVE_AND_RELEASE);
#endif
	*fpregs = pcb->pcb_fpu;

	return 0;
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *fpregs)
{
	struct pcb * const pcb = lwp_getpcb(l);

#ifdef PPC_HAVE_FPU
	fpu_save_lwp(l, FPU_DISCARD);
#endif

	pcb->pcb_fpu = *fpregs;

	/* pcb_fpu is initialized now. */
	l->l_md.md_flags |= MDLWP_USEDFPU;

	return 0;
}

/*
 * Set the process's program counter.
 */
int
process_set_pc(struct lwp *l, void *addr)
{
	struct trapframe * const tf = trapframe(l);
	
	tf->tf_srr0 = (register_t)addr;

	return 0;
}

int
process_sstep(struct lwp *l, int sstep)
{
	struct trapframe *tf = trapframe(l);
	
	if (sstep)
		tf->tf_srr1 |= PSL_SE;
	else
		tf->tf_srr1 &= ~PSL_SE;
	return 0;
}


#ifdef __HAVE_PTRACE_MACHDEP
static int
process_machdep_read_vecregs(struct lwp *l, struct vreg *vregs)
{
	struct pcb * const pcb = lwp_getpcb(l);

#ifdef ALTIVEC
	if (cpu_altivec == 0)
		return (EINVAL);
#endif

	/* Is the process using AltiVEC? */
	if ((l->l_md.md_flags & MDLWP_USEDVEC) == 0) {
		memset(vregs, 0, sizeof (*vregs));
		return 0;
	}
	vec_save_lwp(l, VEC_SAVE_AND_RELEASE);
	*vregs = pcb->pcb_vr;

	return (0);
}

static int
process_machdep_write_vecregs(struct lwp *l, struct vreg *vregs)
{
	struct pcb * const pcb = lwp_getpcb(l);

#ifdef ALTIVEC
	if (cpu_altivec == 0)
		return (EINVAL);
#endif

	vec_save_lwp(l, VEC_DISCARD);
	pcb->pcb_vr = *vregs;
	l->l_md.md_flags |= MDLWP_USEDVEC;	/* pcb_vr is initialized now. */

	return (0);
}

int
ptrace_machdep_dorequest(struct lwp *l, struct lwp *lt,
	int req, void *addr, int data)
{
	struct uio uio;
	struct iovec iov;
	int write = 0;

	switch (req) {
	case PT_SETVECREGS:
		write = 1;

	case PT_GETVECREGS:
		/* write = 0 done above. */
		if (!process_machdep_validvecregs(lt->l_proc))
			return (EINVAL);
		iov.iov_base = addr;
		iov.iov_len = sizeof(struct vreg);
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_resid = sizeof(struct vreg);
		uio.uio_rw = write ? UIO_WRITE : UIO_READ;
		uio.uio_vmspace = l->l_proc->p_vmspace;
		return process_machdep_dovecregs(l, lt, &uio);
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
process_machdep_dovecregs(struct lwp *curl, struct lwp *l, struct uio *uio)
{
	struct vreg r;
	int error;
	char *kv;
	int kl;

	kl = sizeof(r);
	kv = (char *) &r;

	kv += uio->uio_offset;
	kl -= uio->uio_offset;
	if (kl > uio->uio_resid)
		kl = uio->uio_resid;

	if (kl < 0)
		error = EINVAL;
	else
		error = process_machdep_read_vecregs(l, &r);
	if (error == 0)
		error = uiomove(kv, kl, uio);
	if (error == 0 && uio->uio_rw == UIO_WRITE) {
		if (l->l_proc->p_stat != SSTOP)
			error = EBUSY;
		else
			error = process_machdep_write_vecregs(l, &r);
	}

	uio->uio_offset = 0;
	return (error);
}

int
process_machdep_validvecregs(struct proc *p)
{
	if (p->p_flag & PK_SYSTEM)
		return (0);

#ifdef ALTIVEC
	return (cpu_altivec);
#endif
#ifdef PPC_HAVE_SPE
	return 1;
#endif
}
#endif /* __HAVE_PTRACE_MACHDEP */
