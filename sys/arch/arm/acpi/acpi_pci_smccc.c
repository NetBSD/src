/* $NetBSD: acpi_pci_smccc.c,v 1.2 2022/10/15 10:45:40 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_pci_smccc.c,v 1.2 2022/10/15 10:45:40 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pci.h>
#include <dev/acpi/acpi_mcfg.h>

#include <arm/acpi/acpi_pci_machdep.h>

#include <arm/pci/pci_smccc.h>

static int
acpi_pci_smccc_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg,
    pcireg_t *data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;
	int status;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (b < ap->ap_bus || b > ap->ap_maxbus) {
		*data = -1;
		return EINVAL;
	}

	status = pci_smccc_read(PCI_SMCCC_SBDF(ap->ap_seg, b, d, f), reg,
				PCI_SMCCC_ACCESS_32BIT, data);
	if (!PCI_SMCCC_SUCCESS(status)) {
		*data = -1;
		return EINVAL;
	}

	return 0;
}

static int
acpi_pci_smccc_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg,
    pcireg_t data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;
	int status;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (b < ap->ap_bus || b > ap->ap_maxbus) {
		return EINVAL;
	}

	status = pci_smccc_write(PCI_SMCCC_SBDF(ap->ap_seg, b, d, f), reg,
				 PCI_SMCCC_ACCESS_32BIT, data);
	if (!PCI_SMCCC_SUCCESS(status)) {
		return EINVAL;
	}

	return 0;
}


void
acpi_pci_smccc_init(struct acpi_pci_context *ap)
{
	int status, ver;
	uint8_t bus_start, bus_end;
	uint16_t next_seg;

	ver = pci_smccc_version();
	if (!PCI_SMCCC_SUCCESS(ver)) {
		aprint_error_dev(ap->ap_dev,
		    "SMCCC: PCI_VERSION call failed, status %#x\n", ver);
		return;
	}
	aprint_normal_dev(ap->ap_dev, "SMCCC: PCI impl. version %u.%u\n",
	    (ver >> 16) & 0x7fff, ver & 0xffff);

	status = pci_smccc_get_seg_info(ap->ap_seg, &bus_start, &bus_end,
	    &next_seg);
	if (!PCI_SMCCC_SUCCESS(status)) {
		aprint_error_dev(ap->ap_dev,
		    "SMCCC: No info for segment %u, status %#x\n",
		    ap->ap_seg, status);
		return;
	}
	aprint_normal_dev(ap->ap_dev, "SMCCC: segment %u, bus %u-%u\n",
	    ap->ap_seg, bus_start, bus_end);

	ap->ap_bus = bus_start;
	ap->ap_maxbus = bus_end;
	ap->ap_conf_read = acpi_pci_smccc_conf_read;
	ap->ap_conf_write = acpi_pci_smccc_conf_write;
	ap->ap_flags |= ACPI_PCI_FLAG_NO_MCFG;
	ap->ap_pciflags_clear = PCI_FLAGS_MSI_OKAY | PCI_FLAGS_MSIX_OKAY;
}
