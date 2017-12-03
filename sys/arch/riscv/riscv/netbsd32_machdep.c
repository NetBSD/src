/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

__RCSID("$NetBSD: netbsd32_machdep.c,v 1.3.12.2 2017/12/03 11:36:39 jdolecek Exp $");

#include <sys/param.h>
#include <sys/ucontext.h>

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <riscv/locore.h>

char machine_arch32[] = MACHINE_ARCH32;
char machine32[] = MACHINE;

void
netbsd32_setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	l->l_proc->p_flag |= PK_32;
	setregs(l, pack, stack);
}

/* 
 * Start a new LWP
 */
void
startlwp32(void *arg)
{
	ucontext32_t * const uc = arg;
	int error __diagused;

	error = cpu_setmcontext32(curlwp, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	// Even though this is a ucontext32_t, the space allocated was for a
	// full ucontext_t
	kmem_free(uc, sizeof(ucontext_t));
	userret(curlwp);
}

// We've worked hard to make sure struct reg32 and __gregset32_t are the same.
// Ditto for struct fpreg and fregset_t.

CTASSERT(sizeof(struct reg32) == sizeof(__gregset32_t));
CTASSERT(sizeof(struct fpreg) == sizeof(__fregset_t));

void
cpu_getmcontext32(struct lwp *l, mcontext32_t *mcp, unsigned int *flags)
{
	const struct trapframe * const tf = l->l_md.md_utf;

	/* Save register context. */
	for (size_t i = _X_RA; i <= _X_GP; i++) {
		mcp->__gregs[i] = tf->tf_reg[i];
	}
	mcp->__gregs[_REG_PC] = tf->tf_pc;

	mcp->__private = (intptr_t)l->l_private;

	*flags |= _UC_CPU | _UC_TLSBASE;

	/* Save floating point register context, if any. */
	KASSERT(l == curlwp);
	if (fpu_valid_p(l)) {
		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 */
		fpu_save(l);

		struct pcb * const pcb = lwp_getpcb(l);
		*(struct fpreg *)mcp->__fregs = pcb->pcb_fpregs;
		*flags |= _UC_FPU;
	}
}

int
cpu_mcontext32_validate(struct lwp *l, const mcontext32_t *mcp)
{
	/*
	 * Verify that at least the PC and SP are user addresses.
	 */
	if ((int32_t) mcp->__gregs[_REG_PC] < 0
	    || (int32_t) mcp->__gregs[_REG_SP] < 0
	    || (mcp->__gregs[_REG_PC] & 1))
		return EINVAL;

	return 0;
}

int
cpu_setmcontext32(struct lwp *l, const mcontext32_t *mcp, unsigned int flags)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;
	const __greg32_t * const gr = mcp->__gregs;
	int error;

	/* Restore register context, if any. */
	if (flags & _UC_CPU) {
		error = cpu_mcontext32_validate(l, mcp);
		if (error)
			return error;

		/* Save register context. */
		for (size_t i = _X_RA; i <= _X_GP; i++) {
			tf->tf_reg[i] = (int32_t)gr[i];
		}
		tf->tf_pc = (int32_t) gr[_REG_PC];
	}

	/* Restore the private thread context */
	if (flags & _UC_TLSBASE) {
		lwp_setprivate(l, (void *)(intptr_t)(int32_t)mcp->__private);
	}

	/* Restore floating point register context, if any. */
	if (flags & _UC_FPU) {
		KASSERT(l == curlwp);
		/* Tell PCU we are replacing the FPU contents. */
		fpu_replace(l);

		/*
		 * The PCB FP regs struct includes the FP CSR, so use the
		 * proper size of fpreg when copying.
		 */
		struct pcb * const pcb = lwp_getpcb(l);
		pcb->pcb_fpregs = *(const struct fpreg *)mcp->__fregs;
	}

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return (0);
}

int
netbsd32_sysarch(struct lwp *l, const struct netbsd32_sysarch_args *uap,
    register_t *retval)
{
	return ENOSYS;
}

vaddr_t
netbsd32_vm_default_addr(struct proc *p, vaddr_t base, vsize_t size,
    int topdown)
{          
	if (topdown)
		return VM_DEFAULT_ADDRESS32_TOPDOWN(base, size);
	else
		return VM_DEFAULT_ADDRESS32_BOTTOMUP(base, size);
}
