/* $NetBSD: acpi_pci_n1sdp.c,v 1.7 2022/10/15 11:07:38 jmcneill Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: acpi_pci_n1sdp.c,v 1.7 2022/10/15 11:07:38 jmcneill Exp $");

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

extern struct bus_space arm_generic_bs_tag;

/* Shared memory location written by the SCP at boot */
#define	N1SDP_SHARED_MEM_BASE	0x06000000

#define	N1SDP_NSEGS		2
#define	N1SDP_TABLE_SIZE	0x4000

#define	N1SDP_BUS_SHIFT		20
#define	N1SDP_DEV_SHIFT		15
#define	N1SDP_FUNC_SHIFT	12

struct n1sdp_pcie_discovery_data {
	uint32_t	rc_base_addr;
	uint32_t	nr_bdfs;
	uint32_t	valid_bdfs[0];
};

static struct n1sdp_pcie_discovery_data *n1sdp_data[N1SDP_NSEGS];

static bool
acpi_pci_n1sdp_valid(pci_chipset_tag_t pc, pcitag_t tag)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	u_int n, bdfaddr;
	int b, d, f;

	if (ap->ap_seg >= N1SDP_NSEGS || n1sdp_data[ap->ap_seg] == NULL)
		return false;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	bdfaddr = (b << N1SDP_BUS_SHIFT) +
		  (d << N1SDP_DEV_SHIFT) +
		  (f << N1SDP_FUNC_SHIFT);

	for (n = 0; n < n1sdp_data[ap->ap_seg]->nr_bdfs; n++) {
		if (n1sdp_data[ap->ap_seg]->valid_bdfs[n] == bdfaddr)
			return true;
	}

	return false;
}

static int
acpi_pci_n1sdp_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t *data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (ap->ap_bus == b) {
		if (d > 0 || f > 0 || reg >= PCI_EXTCONF_SIZE) {
			*data = -1;
			return EINVAL;
		}
		*data = bus_space_read_4(ap->ap_bst, ap->ap_conf_bsh, reg);
		return 0;
	}

	if (!acpi_pci_n1sdp_valid(pc, tag)) {
		*data = -1;
		return 0;
	}

	return acpimcfg_conf_read(pc, tag, reg, data);
}

static int
acpi_pci_n1sdp_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (ap->ap_bus == b) {
		if (d > 0 || f > 0 || reg >= PCI_EXTCONF_SIZE) {
			return EINVAL;
		}
		bus_space_write_4(ap->ap_bst, ap->ap_conf_bsh, reg, data);
		return 0;
	}

	if (!acpi_pci_n1sdp_valid(pc, tag))
		return 0;

	return acpimcfg_conf_write(pc, tag, reg, data);
}

void
acpi_pci_n1sdp_init(struct acpi_pci_context *ap)
{
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh;
	paddr_t pa;
	int error;
	u_int n;

	if (ap->ap_seg >= N1SDP_NSEGS)
		return;

	if (n1sdp_data[ap->ap_seg] == NULL) {
		aprint_normal_dev(ap->ap_dev, "applying N1SDP quirk for segment %d\n", ap->ap_seg);

		pa = N1SDP_SHARED_MEM_BASE + ap->ap_seg * N1SDP_TABLE_SIZE;
		if (bus_space_map(bst, pa, N1SDP_TABLE_SIZE, BUS_SPACE_MAP_LINEAR, &bsh) != 0)
			panic("acpi_pci_n1sdp_init: couldn't map PCIe discovery table");

		n1sdp_data[ap->ap_seg] = bus_space_vaddr(bst, bsh);
		if (n1sdp_data[ap->ap_seg] == NULL)
			panic("acpi_pci_n1sdp_init: couldn't get PCIe discovery table VA");

		error = bus_space_map(bst, n1sdp_data[ap->ap_seg]->rc_base_addr, PCI_EXTCONF_SIZE,
		    BUS_SPACE_MAP_NONPOSTED, &ap->ap_conf_bsh);
		if (error != 0)
			panic("acpi_pci_n1sdp_init: couldn't map segment %d", ap->ap_seg);

		aprint_debug_dev(ap->ap_dev, "N1SDP: RC @ 0x%08x, %d devices\n",
		    n1sdp_data[ap->ap_seg]->rc_base_addr, n1sdp_data[ap->ap_seg]->nr_bdfs);
		for (n = 0; n < n1sdp_data[ap->ap_seg]->nr_bdfs; n++) {
			const uint32_t bdf = le32toh(n1sdp_data[ap->ap_seg]->valid_bdfs[n]);
			const int b = (bdf >> N1SDP_BUS_SHIFT) & 0xff;
			const int d = (bdf >> N1SDP_DEV_SHIFT) & 0x1f;
			const int f = (bdf >> N1SDP_FUNC_SHIFT) & 0x7;
			aprint_debug_dev(ap->ap_dev, "N1SDP: %02x:%02x.%x (%08x)\n", b, d, f, bdf);
		}
	}

	ap->ap_conf_read = acpi_pci_n1sdp_conf_read;
	ap->ap_conf_write = acpi_pci_n1sdp_conf_write;

	/* IO space access seems to cause async SErrors, so disable for now */
	ap->ap_pciflags_clear = PCI_FLAGS_IO_OKAY;
}
