/*	$NetBSD: pci_bus_fixup.c,v 1.7 2006/10/12 01:30:43 christos Exp $	*/

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

/*
 * PCI bus renumbering support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_bus_fixup.c,v 1.7 2006/10/12 01:30:43 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/ppbreg.h>

#include <i386/pci/pci_bus_fixup.h>

/* this array lists the parent for each bus number */
int pci_bus_parent[256];

/* this array lists the pcitag to program each bridge */
pcitag_t pci_bus_tag[256];

static void pci_bridge_reset(pci_chipset_tag_t, pcitag_t, void *);

int
pci_bus_fixup(pci_chipset_tag_t pc, int bus)
{
	static int bus_total;
	static int bridge_cnt;
	int device, maxdevs, function, nfuncs, bridge, bus_max, bus_sub;
	const struct pci_quirkdata *qd;
	pcireg_t reg;
	pcitag_t tag;

	bus_max = bus;
	bus_sub = 0;

	if (++bus_total > 256)
		panic("pci_bus_fixup: more than 256 PCI busses?");

	/* Reset bridge configuration on this bus */
	pci_bridge_foreach(pc, bus, bus, pci_bridge_reset, 0);

	maxdevs = pci_bus_maxdevs(pc, bus);

	for (device = 0; device < maxdevs; device++) {
		tag = pci_make_tag(pc, bus, device, 0);
		reg = pci_conf_read(pc, tag, PCI_ID_REG);

		/* Invalid vendor ID value? */
		if (PCI_VENDOR(reg) == PCI_VENDOR_INVALID)
			continue;
		/* XXX Not invalid, but we've done this ~forever. */
		if (PCI_VENDOR(reg) == 0)
			continue;

		qd = pci_lookup_quirkdata(PCI_VENDOR(reg), PCI_PRODUCT(reg));

		reg = pci_conf_read(pc, tag, PCI_BHLC_REG);
		if (PCI_HDRTYPE_MULTIFN(reg) ||
		    (qd != NULL &&
		     (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0))
			nfuncs = 8;
		else
			nfuncs = 1;

		for (function = 0; function < nfuncs; function++) {
			tag = pci_make_tag(pc, bus, device, function);
			reg = pci_conf_read(pc, tag, PCI_ID_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(reg) == PCI_VENDOR_INVALID)
				continue;
			/* XXX Not invalid, but we've done this ~forever. */
			if (PCI_VENDOR(reg) == 0)
				continue;

			aprint_debug("PCI fixup examining %02x:%02x\n",
			       PCI_VENDOR(reg), PCI_PRODUCT(reg));

			reg = pci_conf_read(pc, tag, PCI_CLASS_REG);
			if (PCI_CLASS(reg) == PCI_CLASS_BRIDGE &&
			    (PCI_SUBCLASS(reg) == PCI_SUBCLASS_BRIDGE_PCI ||
			     PCI_SUBCLASS(reg) == PCI_SUBCLASS_BRIDGE_CARDBUS)) {
				/* Assign the bridge #. */
				bridge = bridge_cnt++;

				/* Assign the bridge's secondary bus #. */
				bus_max++;

				reg = pci_conf_read(pc, tag, PPB_REG_BUSINFO);
				reg &= 0xff000000;
				reg |= bus | (bus_max << 8) | (0xff << 16);
				pci_conf_write(pc, tag, PPB_REG_BUSINFO, reg);

				/* Scan subordinate bus. */
				bus_sub = pci_bus_fixup(pc, bus_max);

				/* Configure the bridge. */
				reg &= 0xff000000;
				reg |= bus | (bus_max << 8) | (bus_sub << 16);
				pci_conf_write(pc, tag, PPB_REG_BUSINFO, reg);

				/* record relationship */
				pci_bus_parent[bus_max]=bus;

				pci_bus_tag[bus_max]=tag;

				aprint_debug("PCI bridge %d: primary %d, "
				    "secondary %d, subordinate %d\n",
				    bridge, bus, bus_max, bus_sub);

				/* Next bridge's secondary bus #. */
				bus_max = (bus_sub > bus_max) ?
				    bus_sub : bus_max;
			}
		}
	}

	return (bus_max);	/* last # of subordinate bus */
}

/* Reset bus-bridge configuration */
void
pci_bridge_reset(pci_chipset_tag_t pc, pcitag_t tag, void *ctx __unused)
{
	pcireg_t reg;

	reg = pci_conf_read(pc, tag, PPB_REG_BUSINFO);
	reg &= 0xff000000;
	reg |= 0x00ffffff;	/* max bus # */
	pci_conf_write(pc, tag, PPB_REG_BUSINFO, reg);
}
