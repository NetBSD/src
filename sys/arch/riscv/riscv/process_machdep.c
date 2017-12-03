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

__RCSID("$NetBSD: process_machdep.c,v 1.2.12.2 2017/12/03 11:36:39 jdolecek Exp $");

#include <sys/param.h>
#include <sys/ptrace.h>

#include <uvm/uvm_extern.h>

#include <riscv/locore.h>

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	//struct trapframe * const tf = l->l_md.md_utf;

	*regs = l->l_md.md_utf->tf_regs;

	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regs)
{
	//struct trapframe * const tf = l->l_md.md_utf;

	l->l_md.md_utf->tf_regs = *regs;

	return 0;
}

int
process_read_fpregs(struct lwp *l, struct fpreg *fpregs, size_t *sz)
{
	struct pcb * const pcb = lwp_getpcb(l);

	/* Is the process using the fpu? */
	if (!fpu_valid_p(l)) {
		memset(fpregs, 0, sizeof (*fpregs));
		return 0;
	}
	fpu_save(l);
	*fpregs = pcb->pcb_fpregs;

	return 0;
}

int
process_write_fpregs(struct lwp *l, const struct fpreg *fpregs, size_t sz)
{
	struct pcb * const pcb = lwp_getpcb(l);

	fpu_replace(l);
	pcb->pcb_fpregs = *fpregs;

	return 0;
}

/*
 * Set the process's program counter.
 */
int
process_set_pc(struct lwp *l, void *addr)
{
	//struct trapframe * const tf = l->l_md.md_utf;
	
	l->l_md.md_utf->tf_pc = (register_t)addr;

	return 0;
}
