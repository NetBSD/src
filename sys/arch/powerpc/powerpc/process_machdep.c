/*	$NetBSD: process_machdep.c,v 1.20.6.1 2006/04/22 11:37:53 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.20.6.1 2006/04/22 11:37:53 simonb Exp $");

#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/ptrace.h>

#include <machine/fpu.h>
#include <machine/pcb.h>
#include <machine/reg.h>

#include <uvm/uvm_extern.h>

#ifdef ALTIVEC
#include <powerpc/altivec.h>
#endif

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe * const tf = trapframe(l);

	memcpy(regs->fixreg, tf->fixreg, sizeof(regs->fixreg));
	regs->lr = tf->lr;
	regs->cr = tf->cr;
	regs->xer = tf->xer;
	regs->ctr = tf->ctr;
	regs->pc = tf->srr0;

	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regs)
{
	struct trapframe * const tf = trapframe(l);

	memcpy(tf->fixreg, regs->fixreg, sizeof(regs->fixreg));
	tf->lr = regs->lr;
	tf->cr = regs->cr;
	tf->xer = regs->xer;
	tf->ctr = regs->ctr;
	tf->srr0 = regs->pc;

	return 0;
}

int
process_read_fpregs(struct lwp *l, struct fpreg *fpregs)
{
	struct pcb * const pcb = &l->l_addr->u_pcb;

	/* Is the process using the fpu? */
	if ((pcb->pcb_flags & PCB_FPU) == 0) {
		memset(fpregs, 0, sizeof (*fpregs));
		return 0;
	}

#ifdef PPC_HAVE_FPU
	save_fpu_lwp(l, FPU_SAVE);
#endif
	*fpregs = pcb->pcb_fpu;

	return 0;
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *fpregs)
{
	struct pcb * const pcb = &l->l_addr->u_pcb;

#ifdef PPC_HAVE_FPU
	save_fpu_lwp(l, FPU_DISCARD);
#endif

	pcb->pcb_fpu = *fpregs;

	/* pcb_fpu is initialized now. */
	pcb->pcb_flags |= PCB_FPU;

	return 0;
}

/*
 * Set the process's program counter.
 */
int
process_set_pc(struct lwp *l, caddr_t addr)
{
	struct trapframe * const tf = trapframe(l);
	
	tf->srr0 = (register_t)addr;
	return 0;
}

int
process_sstep(struct lwp *l, int sstep)
{
	struct trapframe *tf = trapframe(l);
	
	if (sstep)
		tf->srr1 |= PSL_SE;
	else
		tf->srr1 &= ~PSL_SE;
	return 0;
}


#ifdef __HAVE_PTRACE_MACHDEP
static int
process_machdep_read_vecregs(struct lwp *l, struct vreg *vregs)
{
	struct pcb * const pcb = &l->l_addr->u_pcb;

	if (cpu_altivec == 0)
		return (EINVAL);

	/* Is the process using AltiVEC? */
	if ((pcb->pcb_flags & PCB_ALTIVEC) == 0) {
		memset(vregs, 0, sizeof (*vregs));
		return 0;
	}
	save_vec_lwp(l, ALTIVEC_SAVE);
	*vregs = pcb->pcb_vr;

	return (0);
}

static int
process_machdep_write_vecregs(struct lwp *l, struct vreg *vregs)
{
	struct pcb * const pcb = &l->l_addr->u_pcb;

	if (cpu_altivec == 0)
		return (EINVAL);

	save_vec_lwp(l, ALTIVEC_DISCARD);

	pcb->pcb_vr = *vregs;
	pcb->pcb_flags |= PCB_ALTIVEC;	/* pcb_vr is initialized now. */

	return (0);
}

int
ptrace_machdep_dorequest(struct lwp *l, struct lwp *lt,
	int req, caddr_t addr, int data)
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

	if ((error = process_checkioperm(curl, l->l_proc)) != 0)
		return (error);

	kl = sizeof(r);
	kv = (char *) &r;

	kv += uio->uio_offset;
	kl -= uio->uio_offset;
	if (kl > uio->uio_resid)
		kl = uio->uio_resid;

	PHOLD(l);

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

	PRELE(l);

	uio->uio_offset = 0;
	return (error);
}

int
process_machdep_validvecregs(struct proc *p)
{
	if (p->p_flag & P_SYSTEM)
		return (0);

	return (cpu_altivec);
}
#endif /* __HAVE_PTRACE_MACHDEP */
