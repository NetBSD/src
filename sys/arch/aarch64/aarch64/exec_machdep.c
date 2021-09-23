/* $NetBSD: exec_machdep.c,v 1.10 2021/09/23 15:19:03 ryo Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: exec_machdep.c,v 1.10 2021/09/23 15:19:03 ryo Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/cprng.h>

#include <uvm/uvm_extern.h>

#include <compat/common/compat_util.h>
#include <sys/exec_elf.h>			/* mandatory */

#ifdef COMPAT_NETBSD32
#include <compat/netbsd32/netbsd32_exec.h>
#endif

#include <aarch64/armreg.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>

#include <arm/cpufunc.h>

#if EXEC_ELF64
int
aarch64_netbsd_elf64_probe(struct lwp *l, struct exec_package *epp, void *eh0,
	char *itp, vaddr_t *start_p)
{
	return 0;
}
#endif

#if EXEC_ELF32
int
aarch64_netbsd_elf32_probe(struct lwp *l, struct exec_package *epp, void *eh0,
	char *itp, vaddr_t *start_p)
{
	const Elf32_Ehdr * const eh = eh0;
	const bool elf_aapcs_p =
	    (eh->e_flags & EF_ARM_EABIMASK) >= EF_ARM_EABI_VER4;

	/* OABI not support */
	if (!elf_aapcs_p)
		return ENOEXEC;
#ifdef __AARCH64EB__
	/* BE32 not support */
	if ((eh->e_flags & EF_ARM_BE8) == 0)
		return ENOEXEC;
#endif

	/*
	 * require aarch32 feature.
	 * XXX should consider some cluster may have no aarch32?
	 */
	if (__SHIFTOUT(l->l_cpu->ci_id.ac_aa64pfr0, ID_AA64PFR0_EL1_EL0) !=
	    ID_AA64PFR0_EL1_EL0_64_32)
		return ENOEXEC;

	/*
	 * Copy (if any) the machine_arch of the executable to the proc.
	 */
	CTASSERT(sizeof(l->l_proc->p_md.md_march32) ==
	    sizeof(epp->ep_machine_arch));
	if (epp->ep_machine_arch[0] != 0)
		strlcpy(l->l_proc->p_md.md_march32, epp->ep_machine_arch,
		    sizeof(l->l_proc->p_md.md_march32));

	return 0;
}
#endif

void
aarch64_setregs_ptrauth(struct lwp *l, bool randomize)
{
#ifdef ARMV83_PAC
	if (!aarch64_pac_enabled)
		return;

	if (randomize) {
		cprng_strong(kern_cprng, l->l_md.md_ia_user,
		    sizeof(l->l_md.md_ia_user), 0);
		cprng_strong(kern_cprng, l->l_md.md_ib_user,
		    sizeof(l->l_md.md_ib_user), 0);
		cprng_strong(kern_cprng, l->l_md.md_da_user,
		    sizeof(l->l_md.md_da_user), 0);
		cprng_strong(kern_cprng, l->l_md.md_db_user,
		    sizeof(l->l_md.md_db_user), 0);
		cprng_strong(kern_cprng, l->l_md.md_ga_user,
		    sizeof(l->l_md.md_ga_user), 0);
	} else {
		memset(l->l_md.md_ia_user, 0,
		    sizeof(l->l_md.md_ia_user));
		memset(l->l_md.md_ib_user, 0,
		    sizeof(l->l_md.md_ib_user));
		memset(l->l_md.md_da_user, 0,
		    sizeof(l->l_md.md_da_user));
		memset(l->l_md.md_db_user, 0,
		    sizeof(l->l_md.md_db_user));
		memset(l->l_md.md_ga_user, 0,
		    sizeof(l->l_md.md_ga_user));
	}
#endif
}

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = lwp_trapframe(l);

	aarch64_setregs_ptrauth(l, true);

	p->p_flag &= ~PK_32;

	/*
	 * void __start(void (*cleanup)(void), const Obj_Entry *obj,
	 *    struct ps_strings *ps_strings);
	 */
	memset(tf, 0, sizeof(*tf));
	tf->tf_reg[2] = p->p_psstrp;
	tf->tf_pc = pack->ep_entry;
	tf->tf_sp = stack & -16L;
	tf->tf_spsr = SPSR_M_EL0T;
}
