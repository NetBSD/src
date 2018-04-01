/* $NetBSD: process_machdep.c,v 1.2 2018/04/01 04:35:03 ryo Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: process_machdep.c,v 1.2 2018/04/01 04:35:03 ryo Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/lwp.h>

#include <aarch64/pcb.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	*regs = l->l_md.md_utf->tf_regs;
	regs->r_tpidr = (uint64_t)(uintptr_t)l->l_private;
	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regs)
{
	if ((regs->r_spsr & ~SPSR_NZCV) != 0
	    || (regs->r_sp & 15) || regs->r_sp >= VM_MAXUSER_ADDRESS
	    || (regs->r_pc &  3) || regs->r_pc >= VM_MAXUSER_ADDRESS)
		return EINVAL;

	l->l_md.md_utf->tf_regs = *regs;
	l->l_private = regs->r_tpidr;
	return 0;
}

int
process_read_fpregs(struct lwp *l, struct fpreg *fpregs, size_t *lenp)
{
	struct pcb * const pcb = lwp_getpcb(l);
	KASSERT(*lenp <= sizeof(*fpregs));
	fpu_save(l);

	memcpy(fpregs, &pcb->pcb_fpregs, *lenp);

	return 0;
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *fpregs, size_t len)
{
	struct pcb * const pcb = lwp_getpcb(l);
	KASSERT(len <= sizeof(*fpregs));
	fpu_discard(l, true);		// set used flag

	memcpy(&pcb->pcb_fpregs, fpregs, len);

	return 0;
}

int
process_set_pc(struct lwp *l, void *addr)
{

	l->l_md.md_utf->tf_pc = (uintptr_t) addr;

	return 0;
}
