/* $NetBSD: gic_acpi.c,v 1.2 2018/10/21 00:42:05 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gic_acpi.c,v 1.2 2018/10/21 00:42:05 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <arm/cortex/gic_intr.h>
#include <arm/cortex/gic_reg.h>
#include <arm/cortex/gic_v2m.h>
#include <arm/cortex/mpcore_var.h>

#include <dev/fdt/fdtvar.h>

#define	GICD_SIZE		0x1000
#define	GICC_SIZE		0x1000
#define	GICMSIFRAME_SIZE	0x1000

extern struct bus_space arm_generic_bs_tag;
extern struct pic_softc *pic_list[];

static int	gic_acpi_match(device_t, cfdata_t, void *);
static void	gic_acpi_attach(device_t, device_t, void *);

static ACPI_STATUS gic_acpi_find_gicc(ACPI_SUBTABLE_HEADER *, void *);
static ACPI_STATUS gic_acpi_find_msi_frame(ACPI_SUBTABLE_HEADER *, void *);

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

	const bus_addr_t addr = uimin(gicd->BaseAddress, gicc->BaseAddress);
	const bus_size_t end = uimax(gicd->BaseAddress + GICD_SIZE, gicc->BaseAddress + GICC_SIZE);
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

	armgic = config_found(self, &mpcaa, NULL);

	arm_fdt_irq_set_handler(armgic_irq_handler);

	acpi_madt_walk(gic_acpi_find_msi_frame, armgic);
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

static ACPI_STATUS
gic_acpi_find_msi_frame(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	ACPI_MADT_GENERIC_MSI_FRAME *msi_frame = (ACPI_MADT_GENERIC_MSI_FRAME *)hdrp;
	struct gic_v2m_frame *frame;
	struct pic_softc *pic = pic_list[0];
	device_t armgic = aux;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_MSI_FRAME)
		return AE_OK;

	frame = kmem_zalloc(sizeof(*frame), KM_SLEEP);
	frame->frame_reg = msi_frame->BaseAddress;
	frame->frame_pic = pic;
	if (msi_frame->Flags & ACPI_MADT_OVERRIDE_SPI_VALUES) {
		frame->frame_base = msi_frame->SpiBase;
		frame->frame_count = msi_frame->SpiCount;
	} else {
		bus_space_tag_t bst = &arm_generic_bs_tag;
		bus_space_handle_t bsh;
		if (bus_space_map(bst, frame->frame_reg, GICMSIFRAME_SIZE, 0, &bsh) != 0) {
			printf("%s: failed to map frame\n", __func__);
			return AE_OK;
		}
		const uint32_t typer = bus_space_read_4(bst, bsh, GIC_MSI_TYPER);
		bus_space_unmap(bst, bsh, GICMSIFRAME_SIZE);

		frame->frame_base = __SHIFTOUT(typer, GIC_MSI_TYPER_BASE);
		frame->frame_count = __SHIFTOUT(typer, GIC_MSI_TYPER_NUMBER);
	}

	if (gic_v2m_init(frame, armgic, msi_frame->MsiFrameId) != 0)
		aprint_error_dev(armgic, "failed to initialize GICv2m\n");
	else
		aprint_normal_dev(armgic, "GICv2m @ %#" PRIx64 ", SPIs %u-%u\n",
		    (uint64_t)frame->frame_reg, frame->frame_base,
		    frame->frame_base + frame->frame_count);

	return AE_OK;
}
