/*	$NetBSD: process_machdep.c,v 1.43 2022/12/05 16:03:50 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.43 2022/12/05 16:03:50 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_altivec.h"
#include "opt_ppcarch.h"
#endif

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <powerpc/fpu.h>
#include <powerpc/pcb.h>
#include <powerpc/psl.h>
#include <powerpc/reg.h>

#include <powerpc/altivec.h>	/* also for e500 SPE */

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe * const tf = l->l_md.md_utf;

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
	struct trapframe * const tf = l->l_md.md_utf;

	memcpy(tf->tf_fixreg, regs->fixreg, sizeof(regs->fixreg));
	tf->tf_lr = regs->lr;
	tf->tf_cr = regs->cr;
	tf->tf_xer = regs->xer;
	tf->tf_ctr = regs->ctr;
	tf->tf_srr0 = regs->pc;

	return 0;
}

int
process_read_fpregs(struct lwp *l, struct fpreg *fpregs, size_t *sz)
{
	struct pcb * const pcb = lwp_getpcb(l);

	/* Is the process using the fpu? */
	if (!fpu_used_p(l)) {
		memset(fpregs, 0, sizeof (*fpregs));
#ifdef PPC_HAVE_FPU
	} else {
		fpu_save(l);
#endif
	}
	*fpregs = pcb->pcb_fpu;
	fpu_mark_used(l);

	return 0;
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *fpregs, size_t sz)
{
	struct pcb * const pcb = lwp_getpcb(l);

#ifdef PPC_HAVE_FPU
	fpu_discard(l);
#endif
	pcb->pcb_fpu = *fpregs;
	fpu_mark_used(l);		/* pcb_fpu is initialized now. */

	return 0;
}

/*
 * Set the process's program counter.
 */
int
process_set_pc(struct lwp *l, void *addr)
{
	struct trapframe * const tf = l->l_md.md_utf;
	
	tf->tf_srr0 = (register_t)addr;

	return 0;
}

int
process_sstep(struct lwp *l, int sstep)
{
#if !defined(PPC_BOOKE) && !defined(PPC_IBM4XX)
	struct trapframe * const tf = l->l_md.md_utf;
	
	if (sstep) {
		tf->tf_srr1 |= PSL_SE;
	} else {
		tf->tf_srr1 &= ~PSL_SE;
	}
	return 0;
#else
/*
 * We use the software single-stepping for booke/ibm4xx.
 */
	return ppc_sstep(l, sstep);
#endif
}


#ifdef __HAVE_PTRACE_MACHDEP
static int
process_machdep_read_vecregs(struct lwp *l, struct vreg *vregs)
{
	struct pcb * const pcb = lwp_getpcb(l);

#ifdef ALTIVEC
	if (cpu_altivec == 0)
		return EINVAL;
#endif

	/* Is the process using AltiVEC? */
	if (!vec_used_p(l)) {
		memset(vregs, 0, sizeof (*vregs));
	} else {
		vec_save(l);
		*vregs = pcb->pcb_vr;
	}
	vec_mark_used(l);

	return 0;
}

static int
process_machdep_write_vecregs(struct lwp *l, struct vreg *vregs)
{
	struct pcb * const pcb = lwp_getpcb(l);

#ifdef ALTIVEC
	if (cpu_altivec == 0)
		return (EINVAL);
#endif

#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
	vec_discard(l);
#endif
	pcb->pcb_vr = *vregs;		/* pcb_vr is initialized now. */
	vec_mark_used(l);

	return (0);
}

int
ptrace_machdep_dorequest(struct lwp *l, struct lwp **lt,
	int req, void *addr, int data)
{
	struct uio uio;
	struct iovec iov;
	int write = 0, error;

	switch (req) {
	case PT_SETVECREGS:
		write = 1;

	case PT_GETVECREGS:
		/* write = 0 done above. */
		if ((error = ptrace_update_lwp((*lt)->l_proc, lt, data)) != 0)
			return error;
		if (!process_machdep_validvecregs((*lt)->l_proc))
			return (EINVAL);
		iov.iov_base = addr;
		iov.iov_len = sizeof(struct vreg);
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_resid = sizeof(struct vreg);
		uio.uio_rw = write ? UIO_WRITE : UIO_READ;
		uio.uio_vmspace = l->l_proc->p_vmspace;
		return process_machdep_dovecregs(l, *lt, &uio);
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

#if defined(PPC_BOOKE) || defined(PPC_IBM4XX)
/*
 * ppc_ifetch and ppc_istore:
 * fetch/store instructions from/to given process (therefore, we cannot use
 * ufetch/ustore(9) here).
 */

static int
ppc_ifetch(struct lwp *l, vaddr_t va, uint32_t *insn)
{
	struct uio uio;
	struct iovec iov;

	iov.iov_base = insn;
	iov.iov_len = sizeof(*insn);
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)va;
	uio.uio_resid = sizeof(*insn);
	uio.uio_rw = UIO_READ;
	UIO_SETUP_SYSSPACE(&uio);

	return process_domem(curlwp, l, &uio);
}

static int
ppc_istore(struct lwp *l, vaddr_t va, uint32_t insn)
{
	struct uio uio;
	struct iovec iov;

	iov.iov_base = &insn;
	iov.iov_len = sizeof(insn);
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)va;
	uio.uio_resid = sizeof(insn);
	uio.uio_rw = UIO_WRITE;
	UIO_SETUP_SYSSPACE(&uio);

	return process_domem(curlwp, l, &uio);
}

/*
 * Insert or remove single-step breakpoints:
 * We need two breakpoints, in general, at (SRR0 + 4) and the address to
 * which the process can branch into.
 */

int
ppc_sstep(struct lwp *l, int step)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;
	const uint32_t trap = 0x7d821008;	/* twge %r2, %r2 */
	uint32_t insn;
	vaddr_t va[2];
	int i, rv;

	if (step) {
		if (p->p_md.md_ss_addr[0] != 0)
			return 0; /* XXX Should we reset breakpoints? */

		va[0] = (vaddr_t)tf->tf_srr0;
		va[1] = 0;

		/*
		 * Find the address to which the process can branch into.
		 */
		if ((rv = ppc_ifetch(l, va[0], &insn)) != 0)
			return rv;
		if ((insn >> 28) == 4) {
			if ((insn >> 26) == 0x12) {
				const int32_t off =
				    ((int32_t)(insn << 6) >> 6) & ~3;
				va[1] = ((insn & 2) ? 0 : va[0]) + off;
			} else if ((insn >> 26) == 0x10) {
				const int16_t off = (int16_t)insn & ~3;
				va[1] = ((insn & 2) ? 0 : va[0]) + off;
			} else if ((insn & 0xfc00fffe) == 0x4c000420)
				va[1] = tf->tf_ctr;
			  else if ((insn & 0xfc00fffe) == 0x4c000020)
				va[1] = tf->tf_lr;
		}
		va[0] += sizeof(insn);
		if (va[1] == va[0])
			va[1] = 0;

		for (i = 0; i < 2; i++) {
			if (va[i] == 0)
				return 0;
			if ((rv = ppc_ifetch(l, va[i], &insn)) != 0)
				goto error;
			p->p_md.md_ss_insn[i] = insn;
			if ((rv = ppc_istore(l, va[i], trap)) != 0) {
error:				/* Recover as far as possible. */
				if (i == 1 && ppc_istore(l, va[0],
				    p->p_md.md_ss_insn[0]) == 0)
					p->p_md.md_ss_addr[0] = 0;
				return rv;
			}
			p->p_md.md_ss_addr[i] = va[i];
		}
	} else {
		for (i = 0; i < 2; i++) {
			va[i] = p->p_md.md_ss_addr[i];
			if (va[i] == 0)
				return 0;
			if ((rv = ppc_ifetch(l, va[i], &insn)) != 0)
				return rv;
			if (insn != trap) {
				panic("%s: ss_insn[%d] = 0x%x != trap",
				    __func__, i, insn);
			}
			if ((rv = ppc_istore(l, va[i], p->p_md.md_ss_insn[i]))
			    != 0)
				return rv;
			p->p_md.md_ss_addr[i] = 0;
		}
	}
	return 0;
}
#endif /* PPC_BOOKE || PPC_IBM4XX */
