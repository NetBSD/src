/*	$NetBSD: agp_intel.c,v 1.3.4.2 2001/10/01 12:45:51 fvdl Exp $	*/

/*-
 * Copyright (c) 2000 Doug Rabson
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
 *
 *	$FreeBSD: src/sys/pci/agp_intel.c,v 1.4 2001/07/05 21:28:47 jhb Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/proc.h>
#include <sys/agpio.h>
#include <sys/device.h>
#include <sys/agpio.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/agpvar.h>
#include <dev/pci/agpreg.h>

#include <machine/bus.h>

struct agp_intel_softc {
	u_int32_t	initial_aperture; /* aperture size at startup */
	struct agp_gatt *gatt;
};

static u_int32_t agp_intel_get_aperture(struct agp_softc *);
static int agp_intel_set_aperture(struct agp_softc *, u_int32_t);
static int agp_intel_bind_page(struct agp_softc *, off_t, bus_addr_t);
static int agp_intel_unbind_page(struct agp_softc *, off_t);
static void agp_intel_flush_tlb(struct agp_softc *);

struct agp_methods agp_intel_methods = {
	agp_intel_get_aperture,
	agp_intel_set_aperture,
	agp_intel_bind_page,
	agp_intel_unbind_page,
	agp_intel_flush_tlb,
	agp_generic_enable,
	agp_generic_alloc_memory,
	agp_generic_free_memory,
	agp_generic_bind_memory,
	agp_generic_unbind_memory,
};

int
agp_intel_attach(struct device *parent, struct device *self, void *aux)
{
	struct agp_softc *sc = (struct agp_softc *)self;
	struct pci_attach_args *pa= aux;
	struct agp_intel_softc *isc;
	struct agp_gatt *gatt;
	pcireg_t reg;

	isc = malloc(sizeof *isc, M_AGP, M_NOWAIT);
	if (isc == NULL) {
		printf(": can't allocate chipset-specific softc\n");
		return ENOMEM;
	}
	memset(isc, 0, sizeof *isc);

	sc->as_methods = &agp_intel_methods;
	sc->as_chipc = isc;

	pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_AGP, &sc->as_capoff,
	    NULL);

	if (agp_map_aperture(pa, sc) != 0) {
		printf(": can't map aperture\n");
		free(isc, M_AGP);
		sc->as_chipc = NULL;
		return ENXIO;
	}

	isc->initial_aperture = AGP_GET_APERTURE(sc);

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
			printf(": failed to set aperture\n");
			return ENOMEM;
		}
	}
	isc->gatt = gatt;

	/* Install the gatt. */
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_ATTBASE,
	    gatt->ag_physical);
	
	/* Enable things, clear errors etc. */
	/* XXXfvdl get rid of the magic constants */
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_AGPCTRL, 0x2280);
	reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_INTEL_NBXCFG);
	reg &= ~(1 << 10);
	reg |=  (1 << 9);
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_NBXCFG, reg);

	reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_INTEL_STS);
	reg &= ~0x00ff0000;
	reg |= (7 << 16);
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_STS, reg);

	return 0;
}

#if 0
static int
agp_intel_detach(struct agp_softc *sc)
{
	int error;
	pcireg_t reg;
	struct agp_intel_softc *isc = sc->as_chipc;

	error = agp_generic_detach(sc);
	if (error)
		return error;

	reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_INTEL_NBXCFG);
	reg &= ~(1 << 9);
	printf("%s: set NBXCFG to %x\n", __FUNCTION__, reg);
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_NBXCFG, reg);
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_ATTBASE, 0);
	AGP_SET_APERTURE(sc, isc->initial_aperture);
	agp_free_gatt(sc, isc->gatt);

	return 0;
}
#endif

static u_int32_t
agp_intel_get_aperture(struct agp_softc *sc)
{
	u_int32_t apsize;

	apsize = pci_conf_read(sc->as_pc, sc->as_tag, AGP_INTEL_APSIZE) & 0x1f;

	/*
	 * The size is determined by the number of low bits of
	 * register APBASE which are forced to zero. The low 22 bits
	 * are always forced to zero and each zero bit in the apsize
	 * field just read forces the corresponding bit in the 27:22
	 * to be zero. We calculate the aperture size accordingly.
	 */
	return (((apsize ^ 0x1f) << 22) | ((1 << 22) - 1)) + 1;
}

static int
agp_intel_set_aperture(struct agp_softc *sc, u_int32_t aperture)
{
	u_int32_t apsize;
	pcireg_t reg;

	/*
	 * Reverse the magic from get_aperture.
	 */
	apsize = ((aperture - 1) >> 22) ^ 0x1f;

	/*
	 * Double check for sanity.
	 */
	if ((((apsize ^ 0x1f) << 22) | ((1 << 22) - 1)) + 1 != aperture)
		return EINVAL;

	reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_INTEL_APSIZE);
	reg = (reg & 0xffffff00) | apsize;
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_APSIZE, reg);

	return 0;
}

static int
agp_intel_bind_page(struct agp_softc *sc, off_t offset, bus_addr_t physical)
{
	struct agp_intel_softc *isc = sc->as_chipc;

	if (offset < 0 || offset >= (isc->gatt->ag_entries << AGP_PAGE_SHIFT))
		return EINVAL;

	isc->gatt->ag_virtual[offset >> AGP_PAGE_SHIFT] = physical | 0x17;
	return 0;
}

static int
agp_intel_unbind_page(struct agp_softc *sc, off_t offset)
{
	struct agp_intel_softc *isc = sc->as_chipc;

	if (offset < 0 || offset >= (isc->gatt->ag_entries << AGP_PAGE_SHIFT))
		return EINVAL;

	isc->gatt->ag_virtual[offset >> AGP_PAGE_SHIFT] = 0;
	return 0;
}

static void
agp_intel_flush_tlb(struct agp_softc *sc)
{
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_AGPCTRL, 0x2200);
	pci_conf_write(sc->as_pc, sc->as_tag, AGP_INTEL_AGPCTRL, 0x2280);
}
