/* $NetBSD: process_machdep.c,v 1.4.2.1 2018/05/21 04:36:02 pgoyette Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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


/* from sys/arch/amd64/amd64/process_machdep.c */
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
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.4.2.1 2018/05/21 04:36:02 pgoyette Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ptrace.h>

#include <sys/ucontext.h>
#include <machine/pcb.h>
#include <sys/lwp.h>

#include <machine/thunk.h>

int
process_read_regs(struct lwp *l, struct reg *regs)
{
 	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp;
 	register_t *reg;

	ucp = &pcb->pcb_userret_ucp;
	reg = (register_t *) &ucp->uc_mcontext.__gregs;

	memcpy(regs, reg, sizeof(__gregset_t));

	return 0;
}

int
process_read_fpregs(struct lwp *l, struct fpreg *regs, size_t *sz)
{
 	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp;
 	register_t *reg;

	ucp = &pcb->pcb_userret_ucp;
	reg = (register_t *) &ucp->uc_mcontext.__fpregs;

	*sz = sizeof(__fpregset_t);
	memcpy(regs, reg, *sz);

	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regs)
{
thunk_printf("%s called, not implemented\n", __func__);
	return 0;
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *regs, size_t sz)
{
thunk_printf("%s called, not implemented\n", __func__);
	return 0;
}

int
process_sstep(struct lwp *l, int sstep)
{
thunk_printf("%s called, not implemented\n", __func__);
	return 0;
}

int
process_set_pc(struct lwp *l, void *addr)
{
thunk_printf("%s called, not implemented\n", __func__);
	return 0;
}

int
process_write_dbregs(struct lwp *l, const struct dbreg *regs, size_t sz)
{
thunk_printf("%s called, not implemented\n", __func__);
	return 0;
}

int
process_read_dbregs(struct lwp *l, struct dbreg *regs, size_t *sz)
{
thunk_printf("%s called, not implemented\n", __func__);
	return 0;
}

