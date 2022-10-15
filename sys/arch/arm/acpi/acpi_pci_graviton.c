/* $NetBSD: acpi_pci_graviton.c,v 1.5 2022/10/15 11:07:38 jmcneill Exp $ */

/*-
 * Copyright (c) 2018, 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: acpi_pci_graviton.c,v 1.5 2022/10/15 11:07:38 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/cpu.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pci.h>
#include <dev/acpi/acpi_mcfg.h>

#include <arm/acpi/acpi_pci_machdep.h>

static int
acpi_pci_graviton_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t *data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (ap->ap_bus == b) {
		if (d > 0 || f > 0) {
			*data = -1;
			return EINVAL;
		}
		*data = bus_space_read_4(ap->ap_bst, ap->ap_conf_bsh, reg);
		return 0;
	}

	return acpimcfg_conf_read(pc, tag, reg, data);
}

static int
acpi_pci_graviton_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (ap->ap_bus == b) {
		if (d > 0 || f > 0) {
			return EINVAL;
		}
		bus_space_write_4(ap->ap_bst, ap->ap_conf_bsh, reg, data);
		return 0;
	}

	return acpimcfg_conf_write(pc, tag, reg, data);
}

static ACPI_STATUS
acpi_pci_graviton_map(ACPI_HANDLE handle, UINT32 level, void *ctx, void **retval)
{
	struct acpi_pci_context *ap = ctx;
	struct acpi_resources res;
	struct acpi_mem *mem;
	ACPI_HANDLE parent;
	ACPI_INTEGER seg;
	ACPI_STATUS rv;
	int error;

	rv = AcpiGetParent(handle, &parent);
	if (ACPI_FAILURE(rv))
		return rv;
	rv = acpi_eval_integer(parent, "_SEG", &seg);
	if (ACPI_FAILURE(rv))
		seg = 0;
	if (ap->ap_seg != seg)
		return AE_OK;

	rv = acpi_resource_parse(ap->ap_dev, handle, "_CRS", &res, &acpi_resource_parse_ops_quiet);
	if (ACPI_FAILURE(rv))
		return rv;

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		acpi_resource_cleanup(&res);
		return AE_NOT_FOUND;
	}

	error = bus_space_map(ap->ap_bst, mem->ar_base, mem->ar_length,
	    BUS_SPACE_MAP_NONPOSTED, &ap->ap_conf_bsh);
	if (error != 0)
		return AE_NO_MEMORY;

	ap->ap_conf_read = acpi_pci_graviton_conf_read;
	ap->ap_conf_write = acpi_pci_graviton_conf_write;

	return AE_CTRL_TERMINATE;
}

void
acpi_pci_graviton_init(struct acpi_pci_context *ap)
{
	ACPI_STATUS rv;

	rv = AcpiGetDevices(__UNCONST("AMZN0001"), acpi_pci_graviton_map, ap, NULL);
	if (ACPI_FAILURE(rv))
		return;
}
