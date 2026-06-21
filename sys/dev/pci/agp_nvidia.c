/*	$NetBSD: agp_nvidia.c,v 1.1 2026/06/21 18:38:35 andvar Exp $ */

/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2003 Matthew N. Dodd <winter@jurai.net>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agp_nvidia.c,v 1.1 2026/06/21 18:38:35 andvar Exp $");

#include <sys/param.h>
#include <sys/agpio.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <dev/pci/agpreg.h>
#include <dev/pci/agpvar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/cpufunc.h>

#define AGP_NFORCE_MEMCTLTAG(pc, dev)	pci_make_tag(pc, 0, 0, dev)
#define AGP_NFORCE_PPBTAG(pc)	pci_make_tag(pc, 0, 30, 0)

#define	SYSCFG		0xC0010010
#define	IORR_BASE0	0xC0010016
#define	IORR_MASK0	0xC0010017
#define	AMD_K7_NUM_IORR	2

struct agp_nvidia_softc {
	struct agp_softc	agp;
	uint32_t		initial_aperture; /* aperture size at startup */
	struct agp_gatt *	gatt;

	device_t		dev;		/* AGP Controller */
	pcitag_t		mc1_tag;  /* memory controller func 1 */
	pcitag_t		mc2_tag;  /* memory controller func 2 */
	pcitag_t		bdev_tag;		/* Bridge */

	uint32_t		wbc_mask;
	int				num_dirs;
	int				num_active_entries;
	off_t			pg_offset;
};


static int agp_nvidia_init(struct agp_softc *);
static uint32_t agp_nvidia_get_aperture(struct agp_softc *);
static int agp_nvidia_set_aperture(struct agp_softc *, uint32_t);
static int agp_nvidia_bind_page(struct agp_softc *, off_t, bus_addr_t);
static int agp_nvidia_unbind_page(struct agp_softc *, off_t);
static void agp_nvidia_flush_tlb(struct agp_softc *);
#if 0
static int agp_nvidia_detach(struct agp_softc *);
#endif
static int nvidia_init_iorr(uint32_t, uint32_t);


static struct agp_methods agp_nvidia_methods = {
	agp_nvidia_get_aperture,
	agp_nvidia_set_aperture,
	agp_nvidia_bind_page,
	agp_nvidia_unbind_page,
	agp_nvidia_flush_tlb,
	agp_generic_enable,
	agp_generic_alloc_memory,
	agp_generic_free_memory,
	agp_generic_bind_memory,
	agp_generic_unbind_memory,
};

int
agp_nvidia_attach(device_t parent, device_t self, void *aux)
{
	struct agp_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct agp_nvidia_softc *nsc;
	struct agp_gatt *gatt;

	nsc = malloc(sizeof *nsc, M_AGP, M_WAITOK);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_NVIDIA_NFORCE_PCHB:
		nsc->wbc_mask = AGP_NVIDIA_NFORCE_WBC_MASK;
		break;
	case PCI_PRODUCT_NVIDIA_NFORCE2_PCHB:
		nsc->wbc_mask = AGP_NVIDIA_NFORCE2_WBC_MASK;
		break;
	default:
		/* Should never happen */
		aprint_error_dev(self, "Bad chip id\n");
		return ENODEV;
	}

	sc->as_chipc = nsc;
	sc->as_methods = &agp_nvidia_methods;
	nsc->mc1_tag = AGP_NFORCE_MEMCTLTAG(pa->pa_pc, 1);
	nsc->mc2_tag = AGP_NFORCE_MEMCTLTAG(pa->pa_pc, 2);
	nsc->bdev_tag = AGP_NFORCE_PPBTAG(pa->pa_pc);

	pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_AGP, &sc->as_capoff,
	    NULL);

	if (agp_map_aperture(pa, sc, AGP_APBASE) != 0) {
		aprint_error(": can't map aperture\n");
		free(nsc, M_AGP);
		sc->as_chipc = NULL;
		return ENXIO;
	}
	
	nsc->initial_aperture = AGP_GET_APERTURE(sc);

	for (;;) {
		gatt = agp_alloc_gatt(sc);
		if (gatt)
			break;

		/*
		 * Probably contigmalloc failure. Try reducing the
		 * aperture so that the gatt size reduces.
		 */
		if (AGP_SET_APERTURE(sc, AGP_GET_APERTURE(sc) / 2)) {
			agp_generic_detach(sc);
			aprint_error(": failed to set aperture\n");
			return ENOMEM;
		}
	}
	nsc->gatt = gatt;

	return agp_nvidia_init(sc);
}

static int
agp_nvidia_init(struct agp_softc *sc)
{
	struct agp_nvidia_softc *nsc = sc->as_chipc;
	struct agp_gatt *gatt = nsc->gatt;
	uint32_t apbase;
	uint32_t aplimit;
	uint32_t temp;
	int size, i, error;

	apbase = sc->as_apaddr;
	aplimit = apbase + AGP_GET_APERTURE(sc) - 1;
	pci_conf_write(sc->as_pc, nsc->mc2_tag, AGP_NVIDIA_2_APBASE, apbase);
	pci_conf_write(sc->as_pc, nsc->mc2_tag, AGP_NVIDIA_2_APLIMIT, aplimit);
	pci_conf_write(sc->as_pc, nsc->bdev_tag, AGP_NVIDIA_3_APBASE, apbase);
	pci_conf_write(sc->as_pc, nsc->bdev_tag, AGP_NVIDIA_3_APLIMIT, aplimit);
	
	error = nvidia_init_iorr(apbase, AGP_GET_APERTURE(sc));
	error = 0;
	if (error) {
		aprint_error_dev(sc->as_dev, "Failed to setup IORRs\n");
		agp_generic_detach(sc);
		return ENOMEM;
	}

	/* directory size is 64k */
	size = AGP_GET_APERTURE(sc) / 1024 / 1024;
	nsc->num_dirs = size / 64;
	nsc->num_active_entries = (size == 32) ? 16384 : ((size * 1024) / 4);
	nsc->pg_offset = 0;
	if (nsc->num_dirs == 0) {
		nsc->num_dirs = 1;
		nsc->num_active_entries /= (64 / size);
		nsc->pg_offset = rounddown2(apbase & (64 * 1024 * 1024 - 1),
		    AGP_GET_APERTURE(sc)) / PAGE_SIZE;
	}

	/* (G)ATT Base Address */
	for (i = 0; i < 8; i++) {
		pci_conf_write(sc->as_pc, nsc->mc2_tag, AGP_NVIDIA_2_ATTBASE(i),
		    (gatt->ag_physical + (i % nsc->num_dirs) * 64 * 1024) | 1);
	}

	/* GTLB Control */
	temp = pci_conf_read(sc->as_pc, nsc->mc2_tag, AGP_NVIDIA_2_GARTCTRL);
	pci_conf_write(sc->as_pc, nsc->mc2_tag, AGP_NVIDIA_2_GARTCTRL, temp | 0x11);

	/* GART Control */
	temp = pci_conf_read(sc->as_pc, sc->as_tag, AGP_NVIDIA_0_APSIZE);
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_NVIDIA_0_APSIZE, temp | 0x100);

	return 0;
}

static uint32_t
agp_nvidia_get_aperture(struct agp_softc *sc)
{
	uint8_t apsize;

	apsize = pci_conf_read(sc->as_pc, sc->as_tag, AGP_NVIDIA_0_APSIZE)
	    & AGP_NVIDIA_0_APSIZE_MASK;
	switch (apsize) {
	case 0: return (512 * 1024 * 1024);
	case 8: return (256 * 1024 * 1024);
	case 12: return (128 * 1024 * 1024);
	case 14: return (64 * 1024 * 1024);
	case 15: return (32 * 1024 * 1024);
	default:
		aprint_error_dev(sc->as_dev, "Invalid aperture setting 0x%x\n",
		    apsize);
		return 0;
	}
}

static int
agp_nvidia_set_aperture(struct agp_softc *sc, uint32_t aperture)
{
	uint8_t apsize;
	pcireg_t reg;

	switch (aperture) {
	case (512 * 1024 * 1024): apsize = 0; break;
	case (256 * 1024 * 1024): apsize = 8; break;
	case (128 * 1024 * 1024): apsize = 12; break;
	case (64 * 1024 * 1024): apsize = 14; break;
	case (32 * 1024 * 1024): apsize = 15; break;
	default:
		aprint_error_dev(sc->as_dev, "Invalid aperture size (%uMB)\n",
		    aperture / 1024 / 1024);
		return EINVAL;
	}

	reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_NVIDIA_0_APSIZE);
	reg = (reg & ~AGP_NVIDIA_0_APSIZE_MASK) | apsize;
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_NVIDIA_0_APSIZE, reg);
	return 0;
}

static int
agp_nvidia_bind_page(struct agp_softc *sc, off_t offset, bus_addr_t physical)
{
	struct agp_nvidia_softc *nsc = sc->as_chipc;
	uint32_t index;

	if (offset >= (nsc->gatt->ag_entries << AGP_PAGE_SHIFT))
		return EINVAL;

	index = (nsc->pg_offset + offset) >> AGP_PAGE_SHIFT;
	nsc->gatt->ag_virtual[index] = physical | 1;

	return 0;
}

static int
agp_nvidia_unbind_page(struct agp_softc *sc, off_t offset)
{
	struct agp_nvidia_softc *nsc = sc->as_chipc;
	uint32_t index;

	if (offset >= (nsc->gatt->ag_entries << AGP_PAGE_SHIFT))
		return EINVAL;

	index = (nsc->pg_offset + offset) >> AGP_PAGE_SHIFT;
	nsc->gatt->ag_virtual[index] = 0;

	return 0;
}

static void
agp_nvidia_flush_tlb(struct agp_softc *sc)
{
	struct agp_nvidia_softc *nsc = sc->as_chipc;
	uint32_t wbc_reg;
	volatile uint32_t *ag_virtual;
	int i, pages;

	if (nsc->wbc_mask) {
		wbc_reg = pci_conf_read(sc->as_pc, nsc->mc1_tag, AGP_NVIDIA_1_WBC);
		wbc_reg |= nsc->wbc_mask;
		pci_conf_write(sc->as_pc, nsc->mc1_tag, AGP_NVIDIA_1_WBC, wbc_reg);

		/* Wait no more than 3 seconds. */
		for (i = 0; i < 3000; i++) {
			wbc_reg = pci_conf_read(sc->as_pc, nsc->mc1_tag, AGP_NVIDIA_1_WBC);

			if ((nsc->wbc_mask & wbc_reg) == 0)
				break;

			DELAY(1000);
			preempt_point();
		}
		if (i == 3000)
			aprint_debug_dev(sc->as_dev, "TLB flush took more than 3 seconds.\n");
	}

	ag_virtual = (volatile uint32_t *)nsc->gatt->ag_virtual;

	/* Flush TLB entries. */
	pages = nsc->gatt->ag_entries * sizeof(uint32_t) / PAGE_SIZE;
	for (i = 0; i < pages; i++)
		(void)ag_virtual[i * PAGE_SIZE / sizeof(uint32_t)];
	for (i = 0; i < pages; i++)
		(void)ag_virtual[i * PAGE_SIZE / sizeof(uint32_t)];
}

#if 0
static int
agp_nvidia_detach(struct agp_softc *sc)
{
	int error;
	uint32_t temp;
	struct agp_nvidia_softc *nsc = sc->as_chipc;

	error = agp_generic_detach(sc);
	if (error)
		return error;

	/* GART Control */
	temp = pci_conf_read(sc->as_pc, sc->as_tag, AGP_NVIDIA_0_APSIZE);
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_NVIDIA_0_APSIZE, temp & ~(0x100));

	/* GTLB Control */
	temp = pci_conf_read(sc->as_pc, nsc->mc2_tag, AGP_NVIDIA_2_GARTCTRL);
	pci_conf_write(sc->as_pc, nsc->mc2_tag, AGP_NVIDIA_2_GARTCTRL, temp & ~(0x11));

	/* Put the aperture back the way it started. */
	AGP_SET_APERTURE(sc, nsc->initial_aperture);

	/* restore iorr for previous aperture size */
	nvidia_init_iorr(AGP_GET_APERTURE(sc), nsc->initial_aperture);

	agp_free_gatt(sc, nsc->gatt);

	return 0;
}
#endif

static int
nvidia_init_iorr(uint32_t addr, uint32_t size)
{
	uint64_t base, mask, sys;
	uint32_t iorr_addr, free_iorr_addr;

	/* Find the iorr that is already used for the addr */
	/* If not found, determine the uppermost available iorr */
	free_iorr_addr = AMD_K7_NUM_IORR;
	for (iorr_addr = 0; iorr_addr < AMD_K7_NUM_IORR; iorr_addr++) {
		base = rdmsr(IORR_BASE0 + 2 * iorr_addr);
		mask = rdmsr(IORR_MASK0 + 2 * iorr_addr);

		if ((base & 0xfffff000ULL) == (addr & 0xfffff000))
			break;

		if ((mask & 0x00000800ULL) == 0)
			free_iorr_addr = iorr_addr;
	}

	if (iorr_addr >= AMD_K7_NUM_IORR) {
		iorr_addr = free_iorr_addr;
		if (iorr_addr >= AMD_K7_NUM_IORR)
			return EINVAL;
	}

	base = (addr & ~0xfff) | 0x18;
	mask = (0xfULL << 32) | rounddown2(0xfffff000, size) | 0x800;
	wrmsr(IORR_BASE0 + 2 * iorr_addr, base);
	wrmsr(IORR_MASK0 + 2 * iorr_addr, mask);

	sys = rdmsr(SYSCFG);
	sys |= 0x00100000ULL;
	wrmsr(SYSCFG, sys);

	return 0;
}