/* $NetBSD: acpi_pci_layerscape_gen4.c,v 1.5 2022/10/15 11:07:38 jmcneill Exp $ */

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

/*
 * NXP Layerscape PCIe Gen4 controller (not ECAM compliant)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_pci_layerscape_gen4.c,v 1.5 2022/10/15 11:07:38 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/cpu.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pci.h>
#include <dev/acpi/acpi_mcfg.h>

#include <arm/acpi/acpi_pci_machdep.h>

#define	PAB_CTRL			0x808
#define	 PAB_CTRL_PAGE_SEL		__BITS(18,13)
#define	PAB_AXI_AMAP_PEX_WIN_L(x)	(0xba8 + 0x10 * (x))
#define	PAB_AXI_AMAP_PEX_WIN_H(x)	(0xbac + 0x10 * (x))
#define	INDIRECT_ADDR_BOUNDARY		0xc00

#define	LUT_BASE			0x80000
#define	LUT_GCR				0x28
#define	LUT_GCR_RRE			__BIT(0)

#define	REG_TO_PAGE_INDEX(reg)	(((reg) >> 10) & 0x3ff)
#define	REG_TO_PAGE_ADDR(reg)	(((reg) & 0x3ff) | INDIRECT_ADDR_BOUNDARY)

#define	PAB_TARGET_BUS(b)		((b) << 24)
#define	PAB_TARGET_DEV(d)		((d) << 19)
#define	PAB_TARGET_FUNC(f)		((f) << 16)

struct acpi_pci_layerscape_gen4 {
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_space_handle_t win_bsh;
	uint8_t rev;
	kmutex_t lock;
};

static void
acpi_pci_layerscape_gen4_ccsr_setpage(struct acpi_pci_layerscape_gen4 *pcie, u_int page_index)
{
	uint32_t val;

	val = bus_space_read_4(pcie->bst, pcie->bsh, PAB_CTRL);
	val &= ~PAB_CTRL_PAGE_SEL;
	val |= __SHIFTIN(page_index, PAB_CTRL_PAGE_SEL);
	bus_space_write_4(pcie->bst, pcie->bsh, PAB_CTRL, val);
}

static uint32_t
acpi_pci_layerscape_gen4_ccsr_read4(struct acpi_pci_layerscape_gen4 *pcie, bus_size_t reg)
{
	const bool indirect = reg >= INDIRECT_ADDR_BOUNDARY;
	const u_int page_index = indirect ? REG_TO_PAGE_INDEX(reg) : 0;
	const bus_size_t page_addr = indirect ? REG_TO_PAGE_ADDR(reg) : reg;

	acpi_pci_layerscape_gen4_ccsr_setpage(pcie, page_index);
	return bus_space_read_4(pcie->bst, pcie->bsh, page_addr);
}

static void
acpi_pci_layerscape_gen4_ccsr_write4(struct acpi_pci_layerscape_gen4 *pcie,
    bus_size_t reg, pcireg_t data)
{
	const bool indirect = reg >= INDIRECT_ADDR_BOUNDARY;
	const u_int page_index = indirect ? REG_TO_PAGE_INDEX(reg) : 0;
	const bus_size_t page_addr = indirect ? REG_TO_PAGE_ADDR(reg) : reg;

	acpi_pci_layerscape_gen4_ccsr_setpage(pcie, page_index);
	bus_space_write_4(pcie->bst, pcie->bsh, page_addr, data);
}

static void
acpi_pci_layerscape_gen4_select_target(struct acpi_pci_layerscape_gen4 *pcie,
    pci_chipset_tag_t pc, pcitag_t tag)
{
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	const uint32_t target = PAB_TARGET_BUS(b) |
	    PAB_TARGET_DEV(d) | PAB_TARGET_FUNC(f);

	acpi_pci_layerscape_gen4_ccsr_write4(pcie, PAB_AXI_AMAP_PEX_WIN_L(0), target);
	acpi_pci_layerscape_gen4_ccsr_write4(pcie, PAB_AXI_AMAP_PEX_WIN_H(0), 0);
}

static bool
acpi_pci_layerscape_gen4_is_tag_okay(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (b <= ap->ap_bus + 1 && d > 0)
		return false;

	if (b != ap->ap_bus)
		return acpimcfg_conf_valid(pc, tag, reg);

	return true;
}

static int
acpi_pci_layerscape_gen4_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t *data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	struct acpi_pci_layerscape_gen4 *pcie = ap->ap_conf_priv;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (!acpi_pci_layerscape_gen4_is_tag_okay(pc, tag, reg)) {
		*data = -1;
		return EINVAL;
	}

	mutex_enter(&pcie->lock);

	if (pcie->rev == 0x10 && reg == PCI_ID_REG)
		bus_space_write_4(pcie->bst, pcie->bsh, LUT_BASE + LUT_GCR, 0);

	if (b == ap->ap_bus) {
		*data = acpi_pci_layerscape_gen4_ccsr_read4(pcie, reg);
	} else {
		acpi_pci_layerscape_gen4_select_target(pcie, pc, tag);
		*data = bus_space_read_4(pcie->bst, pcie->win_bsh, reg);
	}

	if (pcie->rev == 0x10 && reg == PCI_ID_REG)
		bus_space_write_4(pcie->bst, pcie->bsh, LUT_BASE + LUT_GCR, LUT_GCR_RRE);

	mutex_exit(&pcie->lock);

	return 0;
}

static int
acpi_pci_layerscape_gen4_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	struct acpi_pci_context *ap = pc->pc_conf_v;
	struct acpi_pci_layerscape_gen4 *pcie = ap->ap_conf_priv;
	int b, d, f;

	pci_decompose_tag(pc, tag, &b, &d, &f);

	if (!acpi_pci_layerscape_gen4_is_tag_okay(pc, tag, reg))
		return EINVAL;

	mutex_enter(&pcie->lock);

	if (b == ap->ap_bus) {
		acpi_pci_layerscape_gen4_ccsr_write4(pcie, reg, data);
	} else {
		acpi_pci_layerscape_gen4_select_target(pcie, pc, tag);
		bus_space_write_4(pcie->bst, pcie->win_bsh, reg, data);
	}

	mutex_exit(&pcie->lock);

	return 0;
}

static UINT64
acpi_pci_layerscape_win_base(ACPI_INTEGER seg)
{
	ACPI_TABLE_MCFG *mcfg;
	ACPI_MCFG_ALLOCATION *ama;
	ACPI_STATUS rv;
	uint32_t off;
	int i;

	rv = AcpiGetTable(ACPI_SIG_MCFG, 0, (ACPI_TABLE_HEADER **)&mcfg);
	if (ACPI_FAILURE(rv))
		return 0;

	off = sizeof(ACPI_TABLE_MCFG);
	ama = ACPI_ADD_PTR(ACPI_MCFG_ALLOCATION, mcfg, off);
	for (i = 0; off + sizeof(ACPI_MCFG_ALLOCATION) <= mcfg->Header.Length; i++) {
		if (ama->PciSegment == seg)
			return ama->Address;
		off += sizeof(ACPI_MCFG_ALLOCATION);
		ama = ACPI_ADD_PTR(ACPI_MCFG_ALLOCATION, mcfg, off);
	}

	return 0;	/* not found */
}

static ACPI_STATUS
acpi_pci_layerscape_gen4_map(ACPI_HANDLE handle, UINT32 level, void *ctx, void **retval)
{
	struct acpi_pci_context *ap = ctx;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_pci_layerscape_gen4 *pcie;
	bus_space_handle_t bsh;
	ACPI_HANDLE parent;
	ACPI_INTEGER seg;
	ACPI_STATUS rv;
	UINT64 win_base;
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

	win_base = acpi_pci_layerscape_win_base(seg);
	if (win_base == 0) {
		aprint_error_dev(ap->ap_dev, "couldn't find MCFG entry for segment %ld\n", seg);
		return AE_NOT_FOUND;
	}

	error = bus_space_map(ap->ap_bst, mem->ar_base, mem->ar_length,
	    BUS_SPACE_MAP_NONPOSTED, &bsh);
	if (error != 0)
		return AE_NO_MEMORY;

	pcie = kmem_alloc(sizeof(*pcie), KM_SLEEP);
	pcie->bst = ap->ap_bst;
	pcie->bsh = bsh;
	mutex_init(&pcie->lock, MUTEX_DEFAULT, IPL_HIGH);

	error = bus_space_map(ap->ap_bst, win_base, PCI_EXTCONF_SIZE,
	    BUS_SPACE_MAP_NONPOSTED, &pcie->win_bsh);
	if (error != 0)
		return AE_NO_MEMORY;

	const pcireg_t cr = bus_space_read_4(pcie->bst, pcie->bsh, PCI_CLASS_REG);
	pcie->rev = PCI_REVISION(cr);

	ap->ap_conf_read = acpi_pci_layerscape_gen4_conf_read;
	ap->ap_conf_write = acpi_pci_layerscape_gen4_conf_write;
	ap->ap_conf_priv = pcie;

	aprint_verbose_dev(ap->ap_dev,
	    "PCIe segment %lu: Layerscape Gen4 rev. %#x found at %#lx-%#lx\n",
	    seg, pcie->rev, mem->ar_base, mem->ar_base + mem->ar_length - 1);

	return AE_CTRL_TERMINATE;
}

void
acpi_pci_layerscape_gen4_init(struct acpi_pci_context *ap)
{
	ACPI_STATUS rv;

	rv = AcpiGetDevices(__UNCONST("NXP0016"), acpi_pci_layerscape_gen4_map, ap, NULL);
	if (ACPI_FAILURE(rv))
		return;
}
