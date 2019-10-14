/* $NetBSD: gic_v2m_acpi.c,v 1.1 2019/10/14 11:00:13 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gic_v2m_acpi.c,v 1.1 2019/10/14 11:00:13 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <arm/cortex/gic_v2m.h>

#include <arm/acpi/gic_v2m_acpi.h>

#define	GICMSIFRAME_SIZE	0x1000

extern struct bus_space arm_generic_bs_tag;
extern struct pic_softc *pic_list[];

static const struct gic_v2m_acpi_quirk {
	const char			q_oemid[ACPI_OEM_ID_SIZE+1];
	const char			q_oemtableid[ACPI_OEM_TABLE_ID_SIZE+1];
	uint32_t			q_flags;
} gic_v2m_acpi_quirks[] = {
	{ "AMAZON",     "GRAVITON",     GIC_V2M_FLAG_GRAVITON },
};

static uint32_t
gic_v2m_acpi_flags(void)
{
	ACPI_TABLE_MADT *madt;
	ACPI_STATUS rv;
	int n;

	rv = AcpiGetTable(ACPI_SIG_MADT, 0, (ACPI_TABLE_HEADER **)&madt);
	if (ACPI_FAILURE(rv))
		return 0;

	for (n = 0; n < __arraycount(gic_v2m_acpi_quirks); n++) {
		const struct gic_v2m_acpi_quirk *q = &gic_v2m_acpi_quirks[n];
		if (memcmp(q->q_oemid, madt->Header.OemId, ACPI_OEM_ID_SIZE) == 0 &&
		    memcmp(q->q_oemtableid, madt->Header.OemTableId, ACPI_OEM_TABLE_ID_SIZE) == 0)
			return q->q_flags;
	}

	return 0;
}

ACPI_STATUS
gic_v2m_acpi_find_msi_frame(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	ACPI_MADT_GENERIC_MSI_FRAME *msi_frame = (ACPI_MADT_GENERIC_MSI_FRAME *)hdrp;
	struct gic_v2m_frame *frame;
	struct pic_softc *pic = pic_list[0];
	device_t dev = aux;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_MSI_FRAME)
		return AE_OK;

	frame = kmem_zalloc(sizeof(*frame), KM_SLEEP);
	frame->frame_reg = msi_frame->BaseAddress;
	frame->frame_pic = pic;
	frame->frame_flags = gic_v2m_acpi_flags();
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

	if (gic_v2m_init(frame, dev, msi_frame->MsiFrameId) != 0)
		aprint_error_dev(dev, "failed to initialize GICv2m\n");
	else
		aprint_normal_dev(dev, "GICv2m @ %#" PRIx64 ", SPIs %u-%u\n",
		    (uint64_t)frame->frame_reg, frame->frame_base,
		    frame->frame_base + frame->frame_count);

	return AE_OK;
}
