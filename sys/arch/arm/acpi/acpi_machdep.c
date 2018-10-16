/* $NetBSD: acpi_machdep.c,v 1.3 2018/10/16 16:38:22 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_machdep.c,v 1.3 2018/10/16 16:38:22 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdtvar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_mcfg.h>

#include <arm/pic/picvar.h>

#include <arm/locore.h>

#include <machine/acpi_machdep.h>

extern struct bus_space arm_generic_bs_tag;

ACPI_STATUS
acpi_md_OsInitialize(void)
{
	return AE_OK;
}

ACPI_PHYSICAL_ADDRESS
acpi_md_OsGetRootPointer(void)
{
	uint64_t pa;

	const int chosen = OF_finddevice("/chosen");
	if (chosen == -1)
		return 0;

	if (of_getprop_uint64(chosen, "netbsd,acpi-root-table", &pa) != 0)
		return 0;

	return (ACPI_PHYSICAL_ADDRESS)pa;
}

ACPI_STATUS
acpi_md_OsInstallInterruptHandler(UINT32 irq, ACPI_OSD_HANDLER handler, void *context,
    void **cookiep, const char *xname)
{
	const int ipl = IPL_TTY;
	const int type = IST_LEVEL;	/* TODO: MADT */

	*cookiep = intr_establish(irq, ipl, type, (int (*)(void *))handler, context);

	return *cookiep == NULL ? AE_NO_MEMORY : AE_OK;
}

void
acpi_md_OsRemoveInterruptHandler(void *cookie)
{
	intr_disestablish(cookie);
}

ACPI_STATUS
acpi_md_OsMapMemory(ACPI_PHYSICAL_ADDRESS pa, UINT32 size, void **vap)
{
	paddr_t spa, epa, curpa;
	vaddr_t va, curva;

	spa = trunc_page(pa);
	epa = round_page(pa + size);

	va = uvm_km_alloc(kernel_map, epa - spa, 0, UVM_KMF_VAONLY);
	if (va == 0)
		return AE_NO_MEMORY;

	for (curpa = spa, curva = va; curpa < epa; curpa += PAGE_SIZE, curva += PAGE_SIZE)
		pmap_kenter_pa(curva, curpa, VM_PROT_READ | VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());

	*vap = (void *)(va + (pa - spa));

	return AE_OK;
}

void
acpi_md_OsUnmapMemory(void *va, UINT32 size)
{
	vaddr_t ova;
	vsize_t osz;

	ova = trunc_page((vaddr_t)va);
	osz = round_page((vaddr_t)va + size) - ova;

	pmap_kremove(ova, osz);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, ova, osz, UVM_KMF_VAONLY);
}

ACPI_STATUS
acpi_md_OsGetPhysicalAddress(void *va, ACPI_PHYSICAL_ADDRESS *pap)
{
	paddr_t pa;

	if (!pmap_extract(pmap_kernel(), (vaddr_t)va, &pa))
		return AE_ERROR;

	*pap = pa;

	return AE_OK;
}

BOOLEAN
acpi_md_OsReadable(void *va, UINT32 len)
{
	vaddr_t sva, eva;
	pt_entry_t *pte;

	sva = trunc_page((vaddr_t)va);
	eva = round_page((vaddr_t)va + len);

	if (sva < VM_MIN_KERNEL_ADDRESS)
		return FALSE;

	for (; sva < eva; sva += PAGE_SIZE) {
		pte = kvtopte(sva);
		if ((*pte & (LX_BLKPAG_AF|LX_BLKPAG_AP_RO)) != (LX_BLKPAG_AF|LX_BLKPAG_AP_RO))
			return FALSE;
	}

	return TRUE;
}

BOOLEAN
acpi_md_OsWritable(void *va, UINT32 len)
{
	vaddr_t sva, eva;
	pt_entry_t *pte;

	sva = trunc_page((vaddr_t)va);
	eva = round_page((vaddr_t)va + len);

	if (sva < VM_MIN_KERNEL_ADDRESS)
		return FALSE;

	for (; sva < eva; sva += PAGE_SIZE) {
		pte = kvtopte(sva);
		if ((*pte & (LX_BLKPAG_AF|LX_BLKPAG_AP_RW)) != (LX_BLKPAG_AF|LX_BLKPAG_AP_RW))
			return FALSE;
	}

	return TRUE;
}

void
acpi_md_OsEnableInterrupt(void)
{
	cpsie(I32_bit);
}

void
acpi_md_OsDisableInterrupt(void)
{
	cpsid(I32_bit);
}

int
acpi_md_sleep(int state)
{
	printf("ERROR: ACPI sleep not implemented on this platform\n");
	return -1;
}

uint32_t
acpi_md_pdc(void)
{
	return 0;
}

uint32_t
acpi_md_ncpus(void)
{
	return kcpuset_countset(kcpuset_attached);
}

static ACPI_STATUS
acpi_md_madt_probe(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	struct acpi_softc * const sc = aux;

	config_found_ia(sc->sc_dev, "acpimadtbus", hdrp, NULL);

	return AE_OK;
}

static ACPI_STATUS
acpi_md_gtdt_probe(ACPI_GTDT_HEADER *hdrp, void *aux)
{
	struct acpi_softc * const sc = aux;

	config_found_ia(sc->sc_dev, "acpigtdtbus", hdrp, NULL);

	return AE_OK;
}

void
acpi_md_callback(struct acpi_softc *sc)
{
	ACPI_TABLE_HEADER *hdrp;

	acpimcfg_init(&arm_generic_bs_tag, NULL);

	if (acpi_madt_map() != AE_OK)
		panic("Failed to map MADT");
	acpi_madt_walk(acpi_md_madt_probe, sc);
	acpi_madt_unmap();

	if (acpi_gtdt_map() != AE_OK)
		panic("Failed to map GTDT");
	acpi_gtdt_walk(acpi_md_gtdt_probe, sc);
	acpi_gtdt_unmap();

	if (ACPI_SUCCESS(AcpiGetTable(ACPI_SIG_GTDT, 0, &hdrp)))
		config_found_ia(sc->sc_dev, "acpisdtbus", hdrp, NULL);
}
