/* $NetBSD: fpu.c,v 1.1.28.1 2018/04/07 04:12:10 pgoyette Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: fpu.c,v 1.1.28.1 2018/04/07 04:12:10 pgoyette Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/lwp.h>
#include <sys/evcnt.h>

#include <aarch64/reg.h>
#include <aarch64/pcb.h>
#include <aarch64/armreg.h>
#include <aarch64/machdep.h>

static void fpu_state_load(lwp_t *, unsigned int);
static void fpu_state_save(lwp_t *);
static void fpu_state_release(lwp_t *);

const pcu_ops_t pcu_fpu_ops = {
	.pcu_id = PCU_FPU,
	.pcu_state_load = fpu_state_load,
	.pcu_state_save = fpu_state_save,
	.pcu_state_release = fpu_state_release
};

void
fpu_attach(struct cpu_info *ci)
{
	evcnt_attach_dynamic(&ci->ci_vfp_use, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp use");
	evcnt_attach_dynamic(&ci->ci_vfp_reuse, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp reuse");
	evcnt_attach_dynamic(&ci->ci_vfp_save, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp save");
	evcnt_attach_dynamic(&ci->ci_vfp_release, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp release");
}

static void
fpu_state_load(lwp_t *l, unsigned int flags)
{
	struct pcb * const pcb = lwp_getpcb(l);

	KASSERT(l == curlwp);

	if (__predict_false((flags & PCU_VALID) == 0)) {
		/* initialize fpregs */
		memset(&pcb->pcb_fpregs, 0, sizeof(pcb->pcb_fpregs));
		pcb->pcb_fpregs.fpcr =
		    FPCR_DN | FPCR_FZ | __SHIFTIN(FPCR_RN, FPCR_RMODE);

		curcpu()->ci_vfp_use.ev_count++;
	} else {
		curcpu()->ci_vfp_reuse.ev_count++;
	}

	/* allow user process to use FP */
	l->l_md.md_cpacr = CPACR_FPEN_ALL;
	reg_cpacr_el1_write(CPACR_FPEN_ALL);
	__asm __volatile ("isb");

	if ((flags & PCU_REENABLE) == 0)
		load_fpregs(&pcb->pcb_fpregs);
}

static void
fpu_state_save(lwp_t *l)
{
	struct pcb * const pcb = lwp_getpcb(l);

	curcpu()->ci_vfp_save.ev_count++;

	reg_cpacr_el1_write(CPACR_FPEN_EL1);	/* fpreg access enable */
	__asm __volatile ("isb");

	save_fpregs(&pcb->pcb_fpregs);

	reg_cpacr_el1_write(CPACR_FPEN_NONE);	/* fpreg access disable */
	__asm __volatile ("isb");
}

static void
fpu_state_release(lwp_t *l)
{
	curcpu()->ci_vfp_release.ev_count++;

	/* disallow user process to use FP */
	l->l_md.md_cpacr = CPACR_FPEN_NONE;
	reg_cpacr_el1_write(CPACR_FPEN_NONE);
	__asm __volatile ("isb");
}
