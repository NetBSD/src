/*	$NetBSD: bandit.c,v 1.9 1999/05/05 04:37:19 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/bus.h>

#define	PCI_BANDIT		11

#define	PCI_REG_BANDIT_CFG	0x40
#define	PCI_REG_ADDR_MASK	0x48
#define	PCI_REG_MODE_SELECT	0x50
#define	PCI_REG_ARBUS_HOLDOFF	0x58

#define	PCI_MS_BYTESWAP		0x001	/* Enable Big Endian mode. (R/W)*/
#define	PCI_MS_PASSATOMIC	0x002	/* PCI Bus to ARBus Lock are always allowed (R)*/
#define	PCI_MS_NUMBER_MASK	0x00C	/* PCI Bus Number (R) */
#define	PCI_MS_IS_SYNC		0x010	/* Is Synchronous (1) or Async (0) ? (R)*/
#define	PCI_MS_VGA_SPACE	0x020	/* Map VGA I/O space  (R/W) */
#define	PCI_MS_IO_COHERENT	0x040	/* I/O Coherent (R/W) */
#define	PCI_MS_INT_ENABLE	0x080	/* Allow TEA or PCI Abort INT to pass to Grand Central (R/W) */

#define	BANDIT_SPECIAL_CYCLE	0xe00000	/* Special Cycle offset */

static void bandit_init __P((pci_chipset_tag_t));
static void scan_pci_devs __P((void));
static void config_slot __P((int, pci_chipset_tag_t, int));

void
pci_init()
{

	scan_pci_devs();
}

static void
bandit_init(pc)
	pci_chipset_tag_t pc;
{
	u_int status;
	pcitag_t tag;

	tag = pci_make_tag(pc, 0, PCI_BANDIT, 0);
	if ((pci_conf_read(pc, tag, PCI_ID_REG) & 0xffff) == 0xffff)
		return;

	status = pci_conf_read(pc, tag, PCI_REG_MODE_SELECT);

	if ((status & PCI_MS_IO_COHERENT) == 0) {
		status |= PCI_MS_IO_COHERENT;
		pci_conf_write(pc, tag, PCI_REG_MODE_SELECT, status);
	}
}


static void
scan_pci_devs()
{
	int reglen, node, child, n, is_bandit, is_mpc106;
	char name[64];
	u_int32_t *p, reg[36];

	bzero(pci_bridges, sizeof(pci_bridges));

	node = OF_peer(0);
	node = OF_child(node);

	for (n = 0; node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0)
			continue;

		is_bandit = (strcmp(name, "bandit") == 0 ||
			     strcmp(name, "chaos") == 0) ? 1 : 0;

		/* XXX for now */
		is_mpc106 = (strcmp(name, "pci") == 0) ? 1 : 0;

		if (is_bandit == 0 && is_mpc106 == 0)
			continue;

		/*
		 * Get the "ranges" property.  We're expecting 6 32-bit
		 * values for each address space:
		 *
		 *	phys.hi phys.mid phys.lo host size.hi size.lo
		 *
		 * Note that the MPC106 maps PCI memory space into
		 * two regions, one of which has a 24-bit addressing
		 * mode.  We don't know the order of the "ranges"
		 * property, but both Address Map modes of the MPC106
		 * map the 32-bit memory range at a lower host address
		 * than the 24-bit range.  We always pick the lower
		 * of the memory ranges for this reason.
		 */

		pci_bridges[n].memt = (bus_space_tag_t) 0xffffffff;

		reglen = OF_getproplen(node, "ranges");
		if (reglen > sizeof(reg) || reglen <= 0 ||
		    reglen % (6 * sizeof(u_int32_t)) != 0)
			continue;	/* eek */

		reglen = OF_getprop(node, "ranges", reg, sizeof(reg));

		for (p = reg; reglen > 0;
		     reglen -= (6 * sizeof(u_int32_t)), p += 6) {
			switch (p[0] & OFW_PCI_PHYS_HI_SPACEMASK) {
			case OFW_PCI_PHYS_HI_SPACE_IO:
				pci_bridges[n].iot =
				    (bus_space_tag_t) p[3];
				break;

			case OFW_PCI_PHYS_HI_SPACE_MEM32:
#if 0	/* XXX XXX XXX */
				if (pci_bridges[n].memt >
				    (bus_space_tag_t)p[3])
					pci_bridges[n].memt =
					    (bus_space_tag_t) p[3];
#else
				/*
				 * XXX The Power Mac firmware seems to
				 * XXX include the PCI memory space base
				 * XXX in the device's BARs.  We can remove
				 * XXX this kludge from here once we
				 * XXX fix the bus_space(9) implelentation.
				 */
				pci_bridges[n].memt = (bus_space_tag_t) 0;
#endif
				break;

			/* XXX What about OFW_PCI_PHYS_HI_SPACE_MEM64? */
			}
		}

		/*
		 * The "bus-range" property tells us our PCI bus number.
		 */
		if (OF_getprop(node, "bus-range", reg, sizeof(reg)) != 8)
			continue;

		pci_bridges[n].bus = reg[0];

		/*
		 * Map the PCI configuration space access registers,
		 * and perform any PCI-Host bridge initialization.
		 */
		if (is_bandit) {
			/* XXX magic numbers */
			if (OF_getprop(node, "reg", reg, sizeof(reg)) != 8)
				continue;
			pci_bridges[n].addr = mapiodev(reg[0] + 0x800000, 4);
			pci_bridges[n].data = mapiodev(reg[0] + 0xc00000, 4);
			pci_bridges[n].pc = n;
			bandit_init(n);
		} else if (is_mpc106) {
			/* XXX magic numbers */
			pci_bridges[n].addr = mapiodev(0xfec00000, 4);
			pci_bridges[n].data = mapiodev(0xfee00000, 4);
			pci_bridges[n].pc = PCI_CHIPSET_MPC106; /* for now */
		}

		/*
		 * Configure all of the PCI devices attached to this
		 * PCI-Host bridge.
		 */
		child = OF_child(node);
		while (child) {
			config_slot(child, pci_bridges[n].pc, -1);
			child = OF_peer(child);
		}

		/* Bridge found, increment bridge instance. */
		n++;
	}
}

static void
config_slot(node, pc, irq)
	int node;
	pci_chipset_tag_t pc;
	int irq;
{
	pcitag_t tag;
	pcireg_t csr, intr;
	int i, sz;
	int bus, dev, func;
	u_int32_t reg[40], *rp;
	char name[16];
	struct {
		u_int32_t device;
		u_int32_t junk[3];
		u_int32_t intrnode;
		u_int32_t interrupt;
	} imap[16];

	bzero(name, sizeof(name));
	OF_getprop(node, "name", name, sizeof(name));
	if (strcmp(name, "pci-bridge") == 0) {
		if (irq == -1)
			OF_getprop(node, "AAPL,interrupts", &irq, sizeof(irq));

		node = OF_child(node);
		while (node) {
			config_slot(node, pc, irq);
			node = OF_peer(node);
		}
		return;
	}

	sz = OF_getprop(node, "assigned-addresses", reg, sizeof(reg));
	if (sz < 4)
		return;

	rp = reg;
	bus = (rp[0] & OFW_PCI_PHYS_HI_BUSMASK) >>
	    OFW_PCI_PHYS_HI_BUSSHIFT;
	dev = (rp[0] & OFW_PCI_PHYS_HI_DEVICEMASK) >>
	    OFW_PCI_PHYS_HI_DEVICESHIFT;
	func = (rp[0] & OFW_PCI_PHYS_HI_FUNCTIONMASK) >>
	    OFW_PCI_PHYS_HI_FUNCTIONSHIFT;

	tag = pci_make_tag(pc, bus, dev, func);
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);

	/*
	 * Make sure the IO and MEM enable bits are set in the CSR.
	 */
	for (; sz > 0; sz -= 5 * sizeof(u_int32_t), rp += 5) {
		csr &= ~(PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE);
		switch (rp[0] & OFW_PCI_PHYS_HI_SPACEMASK) {
		case OFW_PCI_PHYS_HI_SPACE_IO:
			csr |= PCI_COMMAND_IO_ENABLE;
			break;

		case OFW_PCI_PHYS_HI_SPACE_MEM32:
			csr |= PCI_COMMAND_MEM_ENABLE;
			break;
		}
	}

	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

	/*
	 * Make sure the line register is programmed with the interrupt
	 * mapping.
	 */
	if (irq == -1 &&
	    OF_getprop(node, "AAPL,interrupts", &irq, sizeof(irq)) == -1 &&
	    OF_getprop(node, "interrupts", &irq, sizeof(irq)) == -1)
		return;

	if (irq == 1) {		/* XXX */
		sz = OF_getprop(OF_parent(node), "interrupt-map",
		    imap, sizeof(imap));
		if (sz != -1) {
			for (i = 0; i < sz / sizeof(*imap); i++) {
				/* XXX should use interrupt-map-mask */
				if (((imap[i].device &
				      OFW_PCI_PHYS_HI_DEVICEMASK) >>
				     OFW_PCI_PHYS_HI_DEVICESHIFT) == dev) {
					irq = imap[i].interrupt;
					break;
				}
			}
		}
	}

	intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
	intr = (intr & ~PCI_INTERRUPT_LINE_MASK) |
	    (irq & PCI_INTERRUPT_LINE_MASK);
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, intr);
}
