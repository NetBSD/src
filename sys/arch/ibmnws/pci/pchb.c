/*	$NetBSD: pchb.c,v 1.3.14.1 2006/06/19 03:44:26 chap Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/pio.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

int	pchbmatch(struct device *, struct cfdata *, void *);
void	pchbattach(struct device *, struct device *, void *);

CFATTACH_DECL(pchb, sizeof(struct device),
    pchbmatch, pchbattach, NULL, NULL);

int
pchbmatch(struct device *parent, struct cfdata *cf, void *aux)
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

void
pchbattach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	pcireg_t reg1, reg2;
	char devinfo[256];
	const char * s1;
	const char * s2;

	printf("\n");

	/*
	 * All we do is print out a description.  Eventually, we
	 * might want to add code that does something that's
	 * possibly chipset-specific.
	 */

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

#define IBM_82660_CACHE_STATUS		0xB1
#define IBM_82660_CACHE_STATUS_L1_EN	0x01
#define IBM_82660_CACHE_STATUS_L2_EN	0x02

#define IBM_82660_OPTIONS_1		0xBA
#define IBM_82660_OPTIONS_1_MCP		0x01
#define IBM_82660_OPTIONS_1_TEA		0x02
#define IBM_82660_OPTIONS_1_ISA		0x04

#define IBM_82660_OPTIONS_3		0xD4
#define IBM_82660_OPTIONS_3_ECC		0x01
#define IBM_82660_OPTIONS_3_DRAM	0x04
#define IBM_82660_OPTIONS_3_SRAM	0x08
#define IBM_82660_OPTIONS_3_SNOOP	0x80

#define IBM_82660_SYSTEM_CTRL		0x81C
#define IBM_82660_SYSTEM_CTRL_L2_EN	0x40
#define IBM_82660_SYSTEM_CTRL_L2_MI	0x80

	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_IBM:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_IBM_82660:
			reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag,
			    IBM_82660_CACHE_STATUS);
			reg2 = in32rb (PREP_BUS_SPACE_IO + IBM_82660_SYSTEM_CTRL);
			if (reg2 & IBM_82660_SYSTEM_CTRL_L2_EN) {
				if (reg1 & IBM_82660_CACHE_STATUS_L2_EN)
					s1 = "internal enabled";
				else
					s1 = "enabled";
				if (reg2 & IBM_82660_SYSTEM_CTRL_L2_MI)
					s2 = "(normal operation)";
				else
					s2 = "(miss updates inhibited)";
			} else {
				s1 = "disabled";
				s2 = "";
			}
			printf("%s: L1: %s L2: %s %s\n", self->dv_xname,
			       (reg1 & IBM_82660_CACHE_STATUS_L1_EN) ? "enabled" : "disabled",
			       s1, s2);
			reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag,
			    IBM_82660_OPTIONS_1);

			printf ("%s: MCP# assertion %s TEA# assertion %s\n", self->dv_xname,
				(reg1 & IBM_82660_OPTIONS_1_MCP) ? "enabled" : "disabled",
				(reg1 & IBM_82660_OPTIONS_1_TEA) ? "enabled" : "disabled");
			printf ("%s: PCI/ISA I/O mapping %s\n", self->dv_xname,
				(reg1 & IBM_82660_OPTIONS_1_ISA) ? "contiguous" : "non-contiguous");

			reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag,
			    IBM_82660_OPTIONS_3);

			printf ("%s: DRAM %s (%s) SRAM %s\n", self->dv_xname,
			        (reg1 & IBM_82660_OPTIONS_3_DRAM) ? "EDO" : "standard",
			        (reg1 & IBM_82660_OPTIONS_3_ECC) ? "ECC" : "parity",
			        (reg1 & IBM_82660_OPTIONS_3_SRAM) ? "sync" : "async");

			printf ("%s: Snoop mode %s\n", self->dv_xname,
			        (reg1 & IBM_82660_OPTIONS_3_SNOOP) ? "603" : "601/604");
		}
	}
}
