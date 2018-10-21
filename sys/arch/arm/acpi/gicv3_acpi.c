/* $NetBSD: gicv3_acpi.c,v 1.1 2018/10/21 21:18:41 jmcneill Exp $ */

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

#define	_INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gicv3_acpi.c,v 1.1 2018/10/21 21:18:41 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/cortex/gicv3.h>
#include <arm/cortex/gic_reg.h>

#define	GICD_SIZE	0x10000
#define	GICR_SIZE	0x20000

extern struct bus_space arm_generic_bs_tag;

struct gicv3_acpi_softc {
	struct gicv3_softc	sc_gic;

	ACPI_MADT_GENERIC_DISTRIBUTOR *sc_madt_gicd;
};

static int	gicv3_acpi_match(device_t, cfdata_t, void *);
static void	gicv3_acpi_attach(device_t, device_t, void *);

static int	gicv3_acpi_map_dist(struct gicv3_acpi_softc *);
static int	gicv3_acpi_map_redist(struct gicv3_acpi_softc *);

CFATTACH_DECL_NEW(gicv3_acpi, sizeof(struct gicv3_acpi_softc), gicv3_acpi_match, gicv3_acpi_attach, NULL, NULL);

static int
gicv3_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	ACPI_SUBTABLE_HEADER *hdrp = aux;
	ACPI_MADT_GENERIC_DISTRIBUTOR *gicd;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR)
		return 0;

	gicd = (ACPI_MADT_GENERIC_DISTRIBUTOR *)hdrp;

	switch (gicd->Version) {
	case ACPI_MADT_GIC_VERSION_NONE:
		return __SHIFTOUT(reg_id_aa64pfr0_el1_read(), ID_AA64PFR0_EL1_GIC) == 1;
	case ACPI_MADT_GIC_VERSION_V3:
	case ACPI_MADT_GIC_VERSION_V4:
		return 1;
	default:
		return 0;
	}
}

static void
gicv3_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct gicv3_acpi_softc * const sc = device_private(self);
	ACPI_MADT_GENERIC_DISTRIBUTOR *gicd = aux;
	int error;

	sc->sc_gic.sc_dev = self;
	sc->sc_gic.sc_bst = &arm_generic_bs_tag;
	sc->sc_madt_gicd = gicd;

	aprint_naive("\n");
	aprint_normal(": GICv3\n");

	error = gicv3_acpi_map_dist(sc);
	if (error) {
		aprint_error_dev(self, "failed to map distributor: %d\n", error);
		return;
	}

	error = gicv3_acpi_map_redist(sc);
	if (error) {
		aprint_error_dev(self, "failed to map redistributor: %d\n", error);
		return;
	}

	error = gicv3_init(&sc->sc_gic);
	if (error) {
		aprint_error_dev(self, "failed to initialize GIC: %d\n", error);
		return;
	}

	arm_fdt_irq_set_handler(gicv3_irq_handler);
}

static int
gicv3_acpi_map_dist(struct gicv3_acpi_softc *sc)
{
	const bus_addr_t addr = sc->sc_madt_gicd->BaseAddress;
	const bus_size_t size = GICD_SIZE;
	int error;

	error = bus_space_map(sc->sc_gic.sc_bst, addr, size, 0, &sc->sc_gic.sc_bsh_d);
	if (error)
		return error;

	return 0;
}

static ACPI_STATUS
gicv3_acpi_count_gicr(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	ACPI_MADT_GENERIC_REDISTRIBUTOR *gicr;
	int *count = aux;

	if (hdrp->Type == ACPI_MADT_TYPE_GENERIC_REDISTRIBUTOR) {
		gicr = (ACPI_MADT_GENERIC_REDISTRIBUTOR *)hdrp;
		*count += howmany(gicr->Length, GICR_SIZE);
	}

	return AE_OK;
}

static ACPI_STATUS
gicv3_acpi_map_gicr(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	struct gicv3_acpi_softc * const sc = aux;
	ACPI_MADT_GENERIC_REDISTRIBUTOR *gicr;
	bus_space_handle_t bsh;
	bus_size_t off;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_REDISTRIBUTOR)
		return AE_OK;

	gicr = (ACPI_MADT_GENERIC_REDISTRIBUTOR *)hdrp;

	if (bus_space_map(sc->sc_gic.sc_bst, gicr->BaseAddress, gicr->Length, 0, &bsh) != 0) {
		aprint_error_dev(sc->sc_gic.sc_dev, "failed to map redistributor at 0x%" PRIx64 " len %#x\n",
		    gicr->BaseAddress, gicr->Length);
		return AE_OK;
	}

	for (off = 0; off < gicr->Length; off += GICR_SIZE) {
		const int redist = sc->sc_gic.sc_bsh_r_count;
		if (bus_space_subregion(sc->sc_gic.sc_bst, bsh, off, GICR_SIZE, &sc->sc_gic.sc_bsh_r[redist]) != 0) {
			aprint_error_dev(sc->sc_gic.sc_dev, "couldn't subregion redistributor registers\n");
			return AE_OK;
		}

		aprint_debug_dev(sc->sc_gic.sc_dev, "redist at 0x%" PRIx64 " [GICR]\n", gicr->BaseAddress + off);

		sc->sc_gic.sc_bsh_r_count++;

		/* If this is the last redist in this region, skip to the next one */
		const uint32_t typer = bus_space_read_4(sc->sc_gic.sc_bst, sc->sc_gic.sc_bsh_r[redist], GICR_TYPER);
		if (typer & GICR_TYPER_Last)
			break;
	}

	return AE_OK;
}

static ACPI_STATUS
gicv3_acpi_count_gicc(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	ACPI_MADT_GENERIC_INTERRUPT *gicc;
	int *count = aux;

	if (hdrp->Type == ACPI_MADT_TYPE_GENERIC_INTERRUPT) {
		gicc = (ACPI_MADT_GENERIC_INTERRUPT *)hdrp;
		if ((gicc->Flags & ACPI_MADT_ENABLED) != 0)
			(*count)++;
	}

	return AE_OK;
}

static ACPI_STATUS
gicv3_acpi_map_gicc(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	struct gicv3_acpi_softc * const sc = aux;
	ACPI_MADT_GENERIC_INTERRUPT *gicc;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_INTERRUPT)
		return AE_OK;

	gicc = (ACPI_MADT_GENERIC_INTERRUPT *)hdrp;
	if ((gicc->Flags & ACPI_MADT_ENABLED) == 0)
		return AE_OK;

	const int redist = sc->sc_gic.sc_bsh_r_count;
	if (bus_space_map(sc->sc_gic.sc_bst, gicc->GicrBaseAddress, GICR_SIZE, 0, &sc->sc_gic.sc_bsh_r[redist]) != 0) {
		aprint_error_dev(sc->sc_gic.sc_dev, "failed to map redistributor at 0x%" PRIx64 " len %#x\n",
		    gicc->GicrBaseAddress, GICR_SIZE);
		return AE_OK;
	}

	aprint_debug_dev(sc->sc_gic.sc_dev, "redist at 0x%" PRIx64 " [GICC]\n", gicc->GicrBaseAddress);

	sc->sc_gic.sc_bsh_r_count++;

	return AE_OK;
}

static int
gicv3_acpi_map_redist(struct gicv3_acpi_softc *sc)
{
	bool use_gicr = false;
	int max_redist = 0;

	/*
	 * Try to use GICR structures to describe redistributors. If no GICR
	 * subtables are found, use the GICR address from the GICC subtables.
	 */
	acpi_madt_walk(gicv3_acpi_count_gicr, &max_redist);
	if (max_redist != 0)
		use_gicr = true;
	else
		acpi_madt_walk(gicv3_acpi_count_gicc, &max_redist);

	if (max_redist == 0)
		return ENODEV;

	sc->sc_gic.sc_bsh_r = kmem_alloc(sizeof(bus_space_handle_t) * max_redist, KM_SLEEP);
	if (use_gicr)
		acpi_madt_walk(gicv3_acpi_map_gicr, sc);
	else
		acpi_madt_walk(gicv3_acpi_map_gicc, sc);

	if (sc->sc_gic.sc_bsh_r_count == 0)
		return ENXIO;

	return 0;
}
