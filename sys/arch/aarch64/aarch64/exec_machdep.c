/* $NetBSD: exec_machdep.c,v 1.1 2014/08/10 05:47:37 matt Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: exec_machdep.c,v 1.1 2014/08/10 05:47:37 matt Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <compat/common/compat_util.h>
#include <sys/exec_elf.h>			/* mandatory */

#ifdef COMPAT_NETBSD32
#include <compat/netbsd32/netbsd32_exec.h>
#endif

#include <aarch64/locore.h>

#if EXEC_ELF64
int
aarch64_netbsd_elf64_probe(struct lwp *l, struct exec_package *epp, void *eh0,
	char *itp, vaddr_t *start_p)
{
	return 0;
}
#endif

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;

	memset(tf, 0, sizeof(*tf));

	/*
	 * void __start(void (*cleanup)(void), const Obj_Entry *obj,  
	 *	struct ps_strings *ps_strings);
	 */

	tf->tf_reg[2] = p->p_psstrp;
	tf->tf_pc = pack->ep_entry;
	tf->tf_sp = stack & -16L;
}
