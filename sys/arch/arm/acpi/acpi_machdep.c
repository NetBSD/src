/* $NetBSD: acpi_machdep.c,v 1.26 2022/10/15 11:07:38 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_machdep.c,v 1.26 2022/10/15 11:07:38 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

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

struct acpi_intrhandler {
	int				(*ah_fn)(void *);
	void				*ah_arg;
	TAILQ_ENTRY(acpi_intrhandler)	ah_list;
};

struct acpi_intrvec {
	int				ai_irq;
	int				ai_ipl;
	int				ai_type;
	bool				ai_mpsafe;
	int				ai_refcnt;
	void				*ai_arg;
	void				*ai_ih;
	TAILQ_HEAD(, acpi_intrhandler)	ai_handlers;
	TAILQ_ENTRY(acpi_intrvec)	ai_list;
};

static TAILQ_HEAD(, acpi_intrvec) acpi_intrvecs =
    TAILQ_HEAD_INITIALIZER(acpi_intrvecs);

bus_dma_tag_t	arm_acpi_dma32_tag(struct acpi_softc *, struct acpi_devnode *);
bus_dma_tag_t	arm_acpi_dma64_tag(struct acpi_softc *, struct acpi_devnode *);

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

static struct acpi_intrvec *
acpi_md_intr_lookup(int irq)
{
	struct acpi_intrvec *ai;

	TAILQ_FOREACH(ai, &acpi_intrvecs, ai_list) {
		if (ai->ai_irq == irq) {
			return ai;
		}
	}

	return NULL;
}

static int
acpi_md_intr(void *arg)
{
	struct acpi_intrvec *ai = arg;
	struct acpi_intrhandler *ah;
	int rv = 0;

	TAILQ_FOREACH(ah, &ai->ai_handlers, ah_list) {
		rv += ah->ah_fn(ah->ah_arg);
	}

	return rv;
}

void *
acpi_md_intr_establish(uint32_t irq, int ipl, int type, int (*handler)(void *), void *arg, bool mpsafe, const char *xname)
{
	struct acpi_intrvec *ai;
	struct acpi_intrhandler *ah;

	ai = acpi_md_intr_lookup(irq);
	if (ai == NULL) {
		ai = kmem_zalloc(sizeof(*ai), KM_SLEEP);
		ai->ai_refcnt = 0;
		ai->ai_irq = irq;
		ai->ai_ipl = ipl;
		ai->ai_type = type;
		ai->ai_mpsafe = mpsafe;
		ai->ai_arg = arg;
		TAILQ_INIT(&ai->ai_handlers);
		if (arg == NULL) {
			ai->ai_ih = intr_establish_xname(irq, ipl,
			    type | (mpsafe ? IST_MPSAFE : 0), handler, NULL,
			    xname);
		} else {
			ai->ai_ih = intr_establish_xname(irq, ipl,
			    type | (mpsafe ? IST_MPSAFE : 0), acpi_md_intr, ai,
			    xname);
		}
		if (ai->ai_ih == NULL) {
			kmem_free(ai, sizeof(*ai));
			return NULL;
		}
		TAILQ_INSERT_TAIL(&acpi_intrvecs, ai, ai_list);
	} else {
		if (ai->ai_arg == NULL) {
			printf("ACPI: cannot share irq with NULL arg\n");
			return NULL;
		}
		if (ai->ai_ipl != ipl) {
			printf("ACPI: cannot share irq with different ipl\n");
			return NULL;
		}
		if (ai->ai_type != type) {
			printf("ACPI: cannot share edge and level interrupts\n");
			return NULL;
		}
		if (ai->ai_mpsafe != mpsafe) {
			printf("ACPI: cannot share between mpsafe/non-mpsafe\n");
			return NULL;
		}
	}

	ai->ai_refcnt++;

	ah = kmem_zalloc(sizeof(*ah), KM_SLEEP);
	ah->ah_fn = handler;
	ah->ah_arg = arg;
	TAILQ_INSERT_TAIL(&ai->ai_handlers, ah, ah_list);

	return ai->ai_ih;
}

void
acpi_md_intr_disestablish(void *ih)
{
	struct acpi_intrvec *ai;
	struct acpi_intrhandler *ah;

	TAILQ_FOREACH(ai, &acpi_intrvecs, ai_list) {
		if (ai->ai_ih == ih) {
			KASSERT(ai->ai_refcnt > 0);
			if (ai->ai_refcnt > 1) {
				panic("%s: cannot disestablish shared irq", __func__);
			}

			TAILQ_REMOVE(&acpi_intrvecs, ai, ai_list);
			ah = TAILQ_FIRST(&ai->ai_handlers);
			kmem_free(ah, sizeof(*ah));
			intr_disestablish(ai->ai_ih);
			kmem_free(ai, sizeof(*ai));
			return;
		}
	}

	panic("%s: interrupt not established", __func__);
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
		config_found(sc->sc_dev, hdrp, NULL,
		    CFARGS(.iattr = "acpimadtbus"));

	return AE_OK;
}

static ACPI_STATUS
acpi_md_madt_probe_gic(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	struct acpi_softc * const sc = aux;

	if (hdrp->Type == ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR)
		config_found(sc->sc_dev, hdrp, NULL,
		    CFARGS(.iattr = "acpimadtbus"));

	return AE_OK;
}

static ACPI_STATUS
acpi_md_gtdt_probe(ACPI_GTDT_HEADER *hdrp, void *aux)
{
	struct acpi_softc * const sc = aux;

	config_found(sc->sc_dev, hdrp, NULL,
	    CFARGS(.iattr = "acpigtdtbus"));

	return AE_OK;
}

#if NPCI > 0
static struct bus_space acpi_md_mcfg_bs_tag;

static int
acpi_md_mcfg_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	return arm_generic_bs_tag.bs_map(t, bpa, size,
	    flag | BUS_SPACE_MAP_NONPOSTED, bshp);
}
#endif

void
acpi_md_callback(struct acpi_softc *sc)
{
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
}

static const char * const module_hid[] = {
	"ACPI0004",	/* Module device */
	NULL
};

static ACPI_HANDLE
arm_acpi_dma_module(struct acpi_softc *sc, struct acpi_devnode *ad)
{
	ACPI_HANDLE tmp;
	ACPI_STATUS rv;

	/*
	 * Search up the tree for a module device with a _DMA method.
	 */
	for (; ad != NULL; ad = ad->ad_parent) {
		if (ad->ad_devinfo->Type != ACPI_TYPE_DEVICE)
			continue;
		if (!acpi_match_hid(ad->ad_devinfo, module_hid))
			continue;
		rv = AcpiGetHandle(ad->ad_handle, "_DMA", &tmp);
		if (ACPI_SUCCESS(rv))
			return ad->ad_handle;
	}

	return NULL;
}

static void
arm_acpi_dma_init_ranges(struct acpi_softc *sc, struct acpi_devnode *ad,
    struct arm32_bus_dma_tag *dmat, uint32_t flags)
{
	struct acpi_resources res;
	struct acpi_mem *mem;
	ACPI_HANDLE module;
	ACPI_STATUS rv;
	int n;

	module = arm_acpi_dma_module(sc, ad->ad_parent);
	if (module == NULL) {
default_tag:
		/* No translation required */
		dmat->_nranges = 1;
		dmat->_ranges = kmem_zalloc(sizeof(*dmat->_ranges), KM_SLEEP);
		dmat->_ranges[0].dr_sysbase = 0;
		dmat->_ranges[0].dr_busbase = 0;
		dmat->_ranges[0].dr_len = UINTPTR_MAX;
		dmat->_ranges[0].dr_flags = flags;
		return;
	}

	rv = acpi_resource_parse_any(sc->sc_dev, module, "_DMA", &res,
	    &acpi_resource_parse_ops_quiet);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev,
		    "failed to parse _DMA on %s: %s\n",
		    acpi_name(module), AcpiFormatException(rv));
		goto default_tag;
	}
	if (res.ar_nmem == 0) {
		acpi_resource_cleanup(&res);
		goto default_tag;
	}

	dmat->_nranges = res.ar_nmem;
	dmat->_ranges = kmem_zalloc(sizeof(*dmat->_ranges) * res.ar_nmem,
	    KM_SLEEP);

	for (n = 0; n < res.ar_nmem; n++) {
		mem = acpi_res_mem(&res, n);
		dmat->_ranges[n].dr_busbase = mem->ar_base;
		dmat->_ranges[n].dr_sysbase = mem->ar_xbase;
		dmat->_ranges[n].dr_len = mem->ar_length;
		dmat->_ranges[n].dr_flags = flags;

		aprint_debug_dev(sc->sc_dev,
		    "%s: DMA sys %#lx-%#lx bus %#lx-%#lx%s\n",
		    acpi_name(ad->ad_handle),
		    dmat->_ranges[n].dr_sysbase,
		    dmat->_ranges[n].dr_sysbase + dmat->_ranges[n].dr_len - 1,
		    dmat->_ranges[n].dr_busbase,
		    dmat->_ranges[n].dr_busbase + dmat->_ranges[n].dr_len - 1,
		    flags ? " (coherent)" : "");
	}

	acpi_resource_cleanup(&res);
}

static uint32_t
arm_acpi_dma_flags(struct acpi_softc *sc, struct acpi_devnode *ad)
{
	ACPI_INTEGER cca = 1;	/* default cache coherent */
	ACPI_STATUS rv;

	for (; ad != NULL; ad = ad->ad_parent) {
		if (ad->ad_devinfo->Type != ACPI_TYPE_DEVICE)
			continue;

		rv = acpi_eval_integer(ad->ad_handle, "_CCA", &cca);
		if (ACPI_SUCCESS(rv))
			break;
	}

	return cca ? _BUS_DMAMAP_COHERENT : 0;
}

bus_dma_tag_t
arm_acpi_dma32_tag(struct acpi_softc *sc, struct acpi_devnode *ad)
{
	bus_dma_tag_t dmat64, dmat32;
	int error;

	if (ad->ad_dmat != NULL)
		return ad->ad_dmat;

	dmat64 = arm_acpi_dma64_tag(sc, ad);

	const uint32_t flags = arm_acpi_dma_flags(sc, ad);
	error = bus_dmatag_subregion(dmat64, 0, UINT32_MAX, &dmat32, flags);
	if (error != 0)
		panic("arm_acpi_dma32_tag: bus_dmatag_subregion returned %d",
		    error);

	return dmat32;
}
__strong_alias(acpi_get_dma_tag,arm_acpi_dma32_tag);

bus_dma_tag_t
arm_acpi_dma64_tag(struct acpi_softc *sc, struct acpi_devnode *ad)
{
	struct arm32_bus_dma_tag *dmat;

	if (ad->ad_dmat64 != NULL)
		return ad->ad_dmat64;

	dmat = kmem_alloc(sizeof(*dmat), KM_SLEEP);
	*dmat = arm_generic_dma_tag;

	const uint32_t flags = arm_acpi_dma_flags(sc, ad);
	arm_acpi_dma_init_ranges(sc, ad, dmat, flags);

	return dmat;
}
__strong_alias(acpi_get_dma64_tag,arm_acpi_dma64_tag);
