/* $NetBSD: efi_runtime.c,v 1.1 2018/10/28 10:21:42 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: efi_runtime.c,v 1.1 2018/10/28 10:21:42 jmcneill Exp $");

#include <sys/param.h>
#include <sys/mutex.h>

#include <uvm/uvm_extern.h>

#include <arm/arm/efi_runtime.h>

static kmutex_t efi_lock;

static struct efi_systbl *ST = NULL;
static struct efi_rt *RT = NULL;

int
arm_efirt_init(paddr_t efi_system_table)
{
	const size_t sz = PAGE_SIZE * 2;
	vaddr_t va, cva;
	paddr_t cpa;

	va = uvm_km_alloc(kernel_map, sz, 0, UVM_KMF_VAONLY);
	if (va == 0) {
		aprint_error("%s: can't allocate VA\n", __func__);
		return ENOMEM;
	}
	for (cva = va, cpa = trunc_page(efi_system_table);
	     cva < va + sz;
	     cva += PAGE_SIZE, cpa += PAGE_SIZE) {
		pmap_kenter_pa(cva, cpa, VM_PROT_READ, 0);
	}
	pmap_update(pmap_kernel());

	ST = (void *)(va + (efi_system_table - trunc_page(efi_system_table)));
	if (ST->st_hdr.th_sig != EFI_SYSTBL_SIG) {
		aprint_error("EFI: signature mismatch (%#lx != %#lx)\n",
		    ST->st_hdr.th_sig, EFI_SYSTBL_SIG);
		return EINVAL;
	}

	RT = ST->st_rt;
	mutex_init(&efi_lock, MUTEX_DEFAULT, IPL_HIGH);

	return 0;
}

int
arm_efirt_gettime(struct efi_tm *tm)
{
	efi_status status;

	if (RT == NULL || RT->rt_gettime == NULL)
		return ENXIO;

	mutex_enter(&efi_lock);
	status = RT->rt_gettime(tm, NULL);
	mutex_exit(&efi_lock);
	if (status)
		return EIO;

	return 0;
}

int
arm_efirt_settime(struct efi_tm *tm)
{
	efi_status status;

	if (RT == NULL || RT->rt_settime == NULL)
		return ENXIO;

	mutex_enter(&efi_lock);
	status = RT->rt_settime(tm);
	mutex_exit(&efi_lock);
	if (status)
		return EIO;

	return 0;
}

int
arm_efirt_reset(enum efi_reset type)
{
	efi_status status;

	if (RT == NULL || RT->rt_reset == NULL)
		return ENXIO;

	mutex_enter(&efi_lock);
	status = RT->rt_reset(type, 0, 0, NULL);
	mutex_exit(&efi_lock);
	if (status)
		return EIO;

	return 0;
}
