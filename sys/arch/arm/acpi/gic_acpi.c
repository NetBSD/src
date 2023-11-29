/* $NetBSD: gic_acpi.c,v 1.7.6.1 2023/11/29 12:33:08 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gic_acpi.c,v 1.7.6.1 2023/11/29 12:33:08 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/cortex/gic_intr.h>
#include <arm/cortex/gic_reg.h>
#include <arm/cortex/gic_v2m.h>
#include <arm/cortex/mpcore_var.h>

#include <arm/acpi/gic_v2m_acpi.h>

#define	GICD_SIZE		0x1000
#define	GICC_SIZE		0x1000

extern struct bus_space arm_generic_bs_tag;
extern struct pic_softc *pic_list[];

static int	gic_acpi_match(device_t, cfdata_t, void *);
static void	gic_acpi_attach(device_t, device_t, void *);

static ACPI_STATUS gic_acpi_find_gicc(ACPI_SUBTABLE_HEADER *, void *);

CFATTACH_DECL_NEW(gic_acpi, 0, gic_acpi_match, gic_acpi_attach, NULL, NULL);

static int
gic_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	ACPI_SUBTABLE_HEADER *hdrp = aux;
	ACPI_MADT_GENERIC_DISTRIBUTOR *gicd;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR)
		return 0;

	gicd = (ACPI_MADT_GENERIC_DISTRIBUTOR *)hdrp;

	switch (gicd->Version) {
	case ACPI_MADT_GIC_VERSION_NONE:
		return __SHIFTOUT(reg_id_aa64pfr0_el1_read(), ID_AA64PFR0_EL1_GIC) == 0;
	case ACPI_MADT_GIC_VERSION_V1:
	case ACPI_MADT_GIC_VERSION_V2:
		return 1;
	default:
		return 0;
	}
}

static void
gic_acpi_attach(device_t parent, device_t self, void *aux)
{
	ACPI_MADT_GENERIC_DISTRIBUTOR *gicd = aux;
	ACPI_MADT_GENERIC_INTERRUPT *gicc = NULL;
	bus_space_handle_t bsh;
	device_t armgic;
	int error;

	acpi_madt_walk(gic_acpi_find_gicc, &gicc);
	if (gicc == NULL) {
		aprint_error(": couldn't find GICC registers\n");
		return;
	}

	const bus_addr_t addr = ulmin(gicd->BaseAddress, gicc->BaseAddress);
	const bus_size_t end = ulmax(gicd->BaseAddress + GICD_SIZE, gicc->BaseAddress + GICC_SIZE);
	const bus_size_t size = end - addr;

	error = bus_space_map(&arm_generic_bs_tag, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map registers: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GIC\n");

	struct mpcore_attach_args mpcaa = {
		.mpcaa_name = "armgic",
		.mpcaa_memt = &arm_generic_bs_tag,
		.mpcaa_memh = bsh,
		.mpcaa_off1 = gicd->BaseAddress - addr,
		.mpcaa_off2 = gicc->BaseAddress - addr,
	};

	armgic = config_found(self, &mpcaa, NULL, CFARGS_NONE);
	if (armgic != NULL) {
		arm_fdt_irq_set_handler(armgic_irq_handler);

#if NPCI > 0
		acpi_madt_walk(gic_v2m_acpi_find_msi_frame, armgic);
#endif
	}
}

static ACPI_STATUS
gic_acpi_find_gicc(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	ACPI_MADT_GENERIC_INTERRUPT **giccp = aux;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_INTERRUPT)
		return AE_OK;

	*giccp = (ACPI_MADT_GENERIC_INTERRUPT *)hdrp;

	return AE_LIMIT;
}
