/* $NetBSD: efi_machdep.c,v 1.3.6.3 2020/04/08 14:07:23 martin Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
__KERNEL_RCSID(0, "$NetBSD: efi_machdep.c,v 1.3.6.3 2020/04/08 14:07:23 martin Exp $");

#include <sys/param.h>
#include <uvm/uvm_extern.h>
#include <machine/cpufunc.h>

#include <arm/arm/efi_runtime.h>

#include <aarch64/machdep.h>

static struct {
	struct faultbuf	faultbuf;
	bool		fpu_used;
} arm_efirt_state;

void
arm_efirt_md_map_range(vaddr_t va, paddr_t pa, size_t sz, enum arm_efirt_mem_type type)
{       
	pt_entry_t attr;

	switch (type) {
	case ARM_EFIRT_MEM_CODE:
		attr = LX_BLKPAG_OS_READ | LX_BLKPAG_OS_WRITE |
		       LX_BLKPAG_AF | LX_BLKPAG_AP_RW | LX_BLKPAG_UXN |
		       LX_BLKPAG_ATTR_NORMAL_WB;
		break;
	case ARM_EFIRT_MEM_DATA:
		attr = LX_BLKPAG_OS_READ | LX_BLKPAG_OS_WRITE |
		       LX_BLKPAG_AF | LX_BLKPAG_AP_RW | LX_BLKPAG_UXN | LX_BLKPAG_PXN |
		       LX_BLKPAG_ATTR_NORMAL_WB;
		break;
	case ARM_EFIRT_MEM_MMIO:
		attr = LX_BLKPAG_OS_READ | LX_BLKPAG_OS_WRITE |
		       LX_BLKPAG_AF | LX_BLKPAG_AP_RW | LX_BLKPAG_UXN | LX_BLKPAG_PXN |
		       LX_BLKPAG_ATTR_DEVICE_MEM;
		break;
	default:
		panic("arm_efirt_md_map_range: unsupported type %d", type);
	}

	pmapboot_enter(va, pa, sz, L3_SIZE, attr, 0, bootpage_alloc, NULL);
	while (sz >= PAGE_SIZE) {
		aarch64_tlbi_by_va(va);
		va += PAGE_SIZE;
		sz -= PAGE_SIZE;
	}
}

int
arm_efirt_md_enter(void)
{
	struct lwp *l = curlwp;

	/* Save FPU state */
	arm_efirt_state.fpu_used = fpu_used_p(l) != 0;
	if (arm_efirt_state.fpu_used)
		fpu_save(l);

	/* Enable FP access (AArch64 UEFI calling convention) */
	reg_cpacr_el1_write(CPACR_FPEN_ALL);
	__asm __volatile ("isb");

	/*
	 * Install custom fault handler. EFI lock is held across calls so
	 * shared faultbuf is safe here.
	 */
	return cpu_set_onfault(&arm_efirt_state.faultbuf);
}

void
arm_efirt_md_exit(void)
{
	struct lwp *l = curlwp;

	/* Disable FP access */
	reg_cpacr_el1_write(CPACR_FPEN_NONE);
	__asm __volatile ("isb");

	/* Restore FPU state */
	if (arm_efirt_state.fpu_used)
		fpu_load(l);

	/* Remove custom fault handler */
	cpu_unset_onfault();
}
