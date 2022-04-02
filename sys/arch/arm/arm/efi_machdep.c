/* $NetBSD: efi_machdep.c,v 1.1 2022/04/02 11:16:06 skrll Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca> and Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: efi_machdep.c,v 1.1 2022/04/02 11:16:06 skrll Exp $");

#include <sys/param.h>
#include <uvm/uvm_extern.h>

#include <arm/vfpreg.h>

#include <arm/arm/efi_runtime.h>

#include <arm/arm32/machdep.h>

static struct {
	struct faultbuf	aert_faultbuf;
	uint32_t	aert_tpidrprw;
	uint32_t	aert_fpexc;
} arm_efirt_state;

int
arm_efirt_md_enter(void)
{
	arm_efirt_state.aert_tpidrprw = armreg_tpidrprw_read();

	/* Disable the VFP.  */
	arm_efirt_state.aert_fpexc = armreg_fpexc_read();
	armreg_fpexc_write(arm_efirt_state.aert_fpexc & ~VFP_FPEXC_EN);
	isb();

	/*
	 * Install custom fault handler. EFI lock is held across calls so
	 * shared faultbuf is safe here.
	 */
	int err = cpu_set_onfault(&arm_efirt_state.aert_faultbuf);
	if (err)
		return err;

	pmap_activate_efirt();

	return 0;
}

void
arm_efirt_md_exit(void)
{
	pmap_deactivate_efirt();

	armreg_tpidrprw_write(arm_efirt_state.aert_tpidrprw);

	/* Restore FP access (if it existed) */
	armreg_fpexc_write(arm_efirt_state.aert_fpexc);
	isb();

	/* Remove custom fault handler */
	cpu_unset_onfault();
}


void
arm_efirt_md_map_range(vaddr_t va, paddr_t pa, size_t sz,
    enum arm_efirt_mem_type type)
{
	int flags = 0;
	int prot = 0;

	switch (type) {
	case ARM_EFIRT_MEM_CODE:
		/* need write permission because fw devs */
		prot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
		break;
	case ARM_EFIRT_MEM_DATA:
		prot = VM_PROT_READ | VM_PROT_WRITE;
		break;
	case ARM_EFIRT_MEM_MMIO:
		prot = VM_PROT_READ | VM_PROT_WRITE;
		flags = PMAP_DEV;
		break;
	default:
		panic("%s: unsupported type %d", __func__, type);
	}
	if (va >= VM_MAXUSER_ADDRESS || va >= VM_MAXUSER_ADDRESS - sz) {
		printf("Incorrect EFI mapping range %" PRIxVADDR
		    "- %" PRIxVADDR "\n", va, va + sz);
	}

	while (sz != 0) {
		pmap_enter(pmap_efirt(), va, pa, prot, flags | PMAP_WIRED);
		va += PAGE_SIZE;
		pa += PAGE_SIZE;
		sz -= PAGE_SIZE;
	}
	pmap_update(pmap_efirt());
}
