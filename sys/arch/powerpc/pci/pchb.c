/*	$NetBSD: pchb.c,v 1.8.2.1 2012/04/17 00:06:48 yamt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: pchb.c,v 1.8.2.1 2012/04/17 00:06:48 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/pio.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/agpreg.h>
#include <dev/pci/agpvar.h>

#include <dev/ic/mpc105reg.h>
#include <dev/ic/mpc106reg.h>
#include <dev/ic/ibm82660reg.h>

#include "agp.h"

int	pchbmatch(device_t, cfdata_t, void *);
void	pchbattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pchb, 0,
    pchbmatch, pchbattach, NULL, NULL);

int
pchbmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match all known PCI host chipsets.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_HOST) {
		return (1);
	}

	return (0);
}

static void
mpc105_print(struct pci_attach_args *pa, device_t self)
{
	pcireg_t reg1, reg2;
	const char *s1;

	reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag, MPC105_PICR1);
	reg2 = pci_conf_read(pa->pa_pc, pa->pa_tag, MPC105_PICR2);
	aprint_normal_dev(self, "L2 cache: ");

	switch (reg2 & MPC105_PICR2_L2_SIZE) {
	case MPC105_PICR2_L2_SIZE_256K:
		s1 = "256K";
		break;
	case MPC105_PICR2_L2_SIZE_512K:
		s1 = "512K";
		break;
	case MPC105_PICR2_L2_SIZE_1M:
		s1 = "1M";
		break;
	default:
		s1 = "reserved size";
		break;
	}

	aprint_normal("%s, ", s1);
	switch (reg1 & MPC105_PICR1_L2_MP) {
	case MPC105_PICR1_L2_MP_NONE:
		s1 = "uniprocessor/none";
		break;
	case MPC105_PICR1_L2_MP_WT:
		s1 = "write-through";
		break;
	case MPC105_PICR1_L2_MP_WB:
		s1 = "write-back";
		break;
	case MPC105_PICR1_L2_MP_MP:
		s1 = "multiprocessor";
		break;
	}
	aprint_normal("%s mode\n", s1);
}

static void
mpc106_print(struct pci_attach_args *pa, device_t self)
{
	pcireg_t reg1, reg2;
	const char *s1;

	reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag, MPC106_PICR1);
	reg2 = pci_conf_read(pa->pa_pc, pa->pa_tag, MPC106_PICR2);
	aprint_normal_dev(self, "L2 cache: ");

	switch (reg2 & MPC106_PICR2_L2_SIZE) {
	case MPC106_PICR2_L2_SIZE_256K:
		s1 = "256K";
		break;
	case MPC106_PICR2_L2_SIZE_512K:
		s1 = "512K";
		break;
	case MPC106_PICR2_L2_SIZE_1M:
		s1 = "1M";
		break;
	default:
		s1 = "reserved size";
		break;
	}

	aprint_normal("%s, ", s1);
	switch (reg1 & MPC106_PICR1_EXT_L2_EN) {
	case 0:
		switch (reg1 & MPC106_PICR1_L2_MP) {
		case MPC106_PICR1_L2_MP_NONE:
			s1 = "uniprocessor/none";
			break;
		case MPC106_PICR1_L2_MP_WT:
			s1 = "internally controlled write-through";
			break;
		case MPC106_PICR1_L2_MP_WB:
			s1 = "internally controlled write-back";
			break;
		case MPC106_PICR1_L2_MP_MP:
			s1 = "multiprocessor/none";
			break;
		}
		break;
	case 1:
		switch (reg1 & MPC106_PICR1_L2_MP) {
		case MPC106_PICR1_L2_MP_NONE:
			s1 = "uniprocessor/external";
			break;
		case MPC106_PICR1_L2_MP_MP:
			s1 = "multiprocessors/external";
			break;
		default:
			s1 = "reserved";
			break;
		}
	}
	aprint_normal("%s mode\n", s1);
}

static void
ibm82660_print(struct pci_attach_args *pa, device_t self)
{
	pcireg_t reg1;
#ifdef PREP_BUS_SPACE_IO
	pcireg_t reg2;
#endif
	const char *s1, *s2;

	reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag,
	    IBM_82660_CACHE_STATUS);
#ifdef PREP_BUS_SPACE_IO
	reg2 = in32rb(PREP_BUS_SPACE_IO+IBM_82660_SYSTEM_CTRL);
	if (reg2 & IBM_82660_SYSTEM_CTRL_L2_EN) {
		if (reg1 & IBM_82660_CACHE_STATUS_L2_EN)
			s1 = "internal enabled";
		else
			s1 = "enabled";
		if (reg2 & IBM_82660_SYSTEM_CTRL_L2_MI)
			s2 = " (normal operation)";
		else
			s2 = " (miss updates inhibited)";
	} else {
		s1 = "disabled";
		s2 = "";
	}
#else
	if (reg1 & IBM_82660_CACHE_STATUS_L2_EN)
		s1 = "enabled";
	else
		s1 = "disabled";
	s2 = "";
#endif
	aprint_normal_dev(self, "L1 %s L2 %s%s\n",
	    (reg1 & IBM_82660_CACHE_STATUS_L1_EN) ? "enabled" : "disabled",
	    s1, s2);

	reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag, IBM_82660_OPTIONS_1);
	aprint_verbose_dev(self, "MCP# assertion %s "
	    "TEA# assertion %s\n",
	    (reg1 & IBM_82660_OPTIONS_1_MCP) ? "enabled" : "disabled",
	    (reg1 & IBM_82660_OPTIONS_1_TEA) ? "enabled" : "disabled");
	aprint_verbose_dev(self, "PCI/ISA I/O mapping %s\n",
	    (reg1 & IBM_82660_OPTIONS_1_ISA) ? "contiguous" : "non-contiguous");

	reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag, IBM_82660_OPTIONS_3);
	aprint_normal_dev(self, "DRAM %s (%s) SRAM %s\n",
	    (reg1 & IBM_82660_OPTIONS_3_DRAM) ? "EDO" : "standard",
	    (reg1 & IBM_82660_OPTIONS_3_ECC) ? "ECC" : "parity",
	    (reg1 & IBM_82660_OPTIONS_3_SRAM) ? "sync" : "async");
	aprint_verbose_dev(self, "Snoop mode %s\n",
	    (reg1 & IBM_82660_OPTIONS_3_SNOOP) ? "603" : "601/604");
}

void
pchbattach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];
#if NAGP > 0
	struct agpbus_attach_args apa;
#endif
	volatile unsigned char *python;
	uint32_t v;
	
	aprint_naive("\n");
	aprint_normal("\n");

	/*
	 * All we do is print out a description.  Eventually, we
	 * might want to add code that does something that's
	 * possibly chipset-specific.
	 */

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal_dev(self, "%s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_IBM:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_IBM_82660:
			ibm82660_print(pa, self);
			break;
		case PCI_PRODUCT_IBM_PYTHON:
			python = mapiodev(0xfeff6000, 0x60, false);
			v = 0x88b78e01; /* taken from linux */
			out32rb(python+0x30, v);
			v = in32rb(python+0x30);
			aprint_debug("Reset python reg 30 to 0x%x\n", v);
			break;
		}
		break;
	case PCI_VENDOR_MOT:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_MOT_MPC105:
			mpc105_print(pa, self);
			break;
		case PCI_PRODUCT_MOT_MPC106:
			mpc106_print(pa, self);
			break;
		}
		break;
	}

#if NAGP > 0
	if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_AGP,
			       NULL, NULL) != 0) {
		apa.apa_pci_args = *pa;
		config_found_ia(self, "agpbus", &apa, agpbusprint);
	}
#endif /* NAGP */
}
