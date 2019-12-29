/* $NetBSD: acpi_machdep.c,v 1.14 2019/12/29 23:47:56 jmcneill Exp $ */

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

#include "pci.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_machdep.c,v 1.14 2019/12/29 23:47:56 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdtvar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#if NPCI > 0
#include <dev/acpi/acpi_mcfg.h>
#endif

#include <arm/arm/efi_runtime.h>

#include <arm/pic/picvar.h>

#include <arm/locore.h>

#include <machine/acpi_machdep.h>

extern struct bus_space arm_generic_bs_tag;
extern struct arm32_bus_dma_tag acpi_coherent_dma_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;

bus_dma_tag_t	arm_acpi_dma_tag(struct acpi_softc *, struct acpi_devnode *);

static int
acpi_md_pmapflags(paddr_t pa)
{
	int len;

	const int chosen = OF_finddevice("/chosen");
	if (chosen == -1)
		return 0;

	const uint32_t *map = fdtbus_get_prop(chosen, "netbsd,uefi-memmap", &len);
	if (map == NULL)
		return 0;

	while (len >= 28) {
		const uint32_t type = be32dec(&map[0]);
		const uint64_t phys_start = be64dec(&map[1]);
		const uint64_t num_pages = be64dec(&map[3]);
		const uint64_t attr = be64dec(&map[5]);

		if (pa >= phys_start && pa < phys_start + (num_pages * EFI_PAGE_SIZE)) {
			switch (type) {
			case EFI_MD_TYPE_RECLAIM:
				/* ACPI table memory */
				return PMAP_WRITE_BACK;

			case EFI_MD_TYPE_IOMEM:
			case EFI_MD_TYPE_IOPORT:
				return PMAP_DEV;

			default:
				if ((attr & EFI_MD_ATTR_WB) != 0)
					return PMAP_WRITE_BACK;
				else if ((attr & EFI_MD_ATTR_WC) != 0)
					return PMAP_WRITE_COMBINE;
				else if ((attr & EFI_MD_ATTR_WT) != 0)
					return 0;	/* XXX */

				return PMAP_DEV;
			}
		}

		map += 7;
		len -= 28;
	}

	/* Not found; assume device memory */
	return PMAP_DEV;
}

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
	return AE_NOT_IMPLEMENTED;
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

	const int pmapflags = acpi_md_pmapflags(spa);

	aprint_debug("%s: 0x%lx 0x%x flags = %#x\n", __func__, pa, size, pmapflags);

	for (curpa = spa, curva = va; curpa < epa; curpa += PAGE_SIZE, curva += PAGE_SIZE)
		pmap_kenter_pa(curva, curpa, VM_PROT_READ | VM_PROT_WRITE, pmapflags);
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
		if ((*pte & (LX_BLKPAG_AF|LX_BLKPAG_AP)) != (LX_BLKPAG_AF|LX_BLKPAG_AP_RO))
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
		if ((*pte & (LX_BLKPAG_AF|LX_BLKPAG_AP)) != (LX_BLKPAG_AF|LX_BLKPAG_AP_RW))
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

void *
acpi_md_intr_establish(uint32_t irq, int ipl, int type, int (*handler)(void *), void *arg, bool mpsafe, const char *xname)
{
	return intr_establish_xname(irq, ipl, type | (mpsafe ? IST_MPSAFE : 0), handler, arg, xname);
}

void
acpi_md_intr_mask(void *ih)
{
	intr_mask(ih);
}

void
acpi_md_intr_unmask(void *ih)
{
	intr_unmask(ih);
}

void
acpi_md_intr_disestablish(void *ih)
{
	intr_disestablish(ih);
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
acpi_md_madt_probe_cpu(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	struct acpi_softc * const sc = aux;

	if (hdrp->Type == ACPI_MADT_TYPE_GENERIC_INTERRUPT)
		config_found_ia(sc->sc_dev, "acpimadtbus", hdrp, NULL);

	return AE_OK;
}

static ACPI_STATUS
acpi_md_madt_probe_gic(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	struct acpi_softc * const sc = aux;

	if (hdrp->Type == ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR)
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

#if NPCI > 0
static struct bus_space acpi_md_mcfg_bs_tag;

static int
acpi_md_mcfg_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	return arm_generic_bs_tag.bs_map(t, bpa, size,
	    flag | _ARM_BUS_SPACE_MAP_STRONGLY_ORDERED, bshp);
}
#endif

void
acpi_md_callback(struct acpi_softc *sc)
{
	ACPI_TABLE_HEADER *hdrp;

#if NPCI > 0
	acpi_md_mcfg_bs_tag = arm_generic_bs_tag;
	acpi_md_mcfg_bs_tag.bs_map = acpi_md_mcfg_bs_map;
	acpimcfg_init(&acpi_md_mcfg_bs_tag, NULL);
#endif

	if (acpi_madt_map() != AE_OK)
		panic("Failed to map MADT");
	acpi_madt_walk(acpi_md_madt_probe_cpu, sc);
	acpi_madt_walk(acpi_md_madt_probe_gic, sc);
	acpi_madt_unmap();

	if (acpi_gtdt_map() != AE_OK)
		panic("Failed to map GTDT");
	acpi_gtdt_walk(acpi_md_gtdt_probe, sc);
	acpi_gtdt_unmap();

	if (ACPI_SUCCESS(AcpiGetTable(ACPI_SIG_GTDT, 0, &hdrp)))
		config_found_ia(sc->sc_dev, "acpisdtbus", hdrp, NULL);
}

bus_dma_tag_t
arm_acpi_dma_tag(struct acpi_softc *sc, struct acpi_devnode *ad)
{
	ACPI_INTEGER cca;

	if (ACPI_FAILURE(acpi_eval_integer(ad->ad_handle, "_CCA", &cca)))
		cca = 1;

	if (cca)
		return &acpi_coherent_dma_tag;
	else
		return &arm_generic_dma_tag;
}
__strong_alias(acpi_get_dma_tag,arm_acpi_dma_tag);
__strong_alias(acpi_get_dma64_tag,arm_acpi_dma_tag);
