/*	$NetBSD: bandit.c,v 1.6 1998/12/29 06:27:59 tsubai Exp $	*/

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
static void config_slot __P((int, pci_chipset_tag_t));

void
pci_init()
{
	scan_pci_devs();
}

void
bandit_init(pc)
	pci_chipset_tag_t pc;
{
	u_int status;
	pcitag_t tag;

	tag = pci_make_tag(pc, 0, PCI_BANDIT, 0);
	if ((pci_conf_read(pc, tag, PCI_ID_REG) & 0xffff) == 0xffff)
		return;

	status  = pci_conf_read(pc, tag, PCI_REG_MODE_SELECT);

	if ((status & PCI_MS_IO_COHERENT) == 0) {
		status |= PCI_MS_IO_COHERENT;
		pci_conf_write(pc, tag, PCI_REG_MODE_SELECT, status);
	}

	return;
}


void
scan_pci_devs()
{
	int node;
	char name[64];
	int n = 0;
	u_int reg[2];

	bzero(pci_bridges, sizeof(pci_bridges));

	node = OF_peer(0);
	node = OF_child(node);

	while (node) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0)
			continue;
		if (strcmp(name, "bandit") == 0 ||
		    strcmp(name, "chaos") == 0) {
			int child;

			if (OF_getprop(node, "reg", reg, sizeof(reg)) != 8)
				continue;

			pci_bridges[n].iot  = (bus_space_tag_t)reg[0];
			pci_bridges[n].addr = mapiodev(reg[0] + 0x800000, 4);
			pci_bridges[n].data = mapiodev(reg[0] + 0xc00000, 4);
			pci_bridges[n].pc = n;

			if (OF_getprop(node, "bus-range",
				       reg, sizeof(reg)) != 8) {
				pci_bridges[n].addr = NULL;
				continue;
			}
			pci_bridges[n].bus = reg[0];

			bandit_init(n);

			child = OF_child(node);
			while (child) {
				config_slot(child, n);
				child = OF_peer(child);
			}
			n++;
		}
		if (strcmp(name, "pci") == 0) {	/* XXX This is not a bandit :) */
			int child;

			if (OF_getprop(node, "reg", reg, sizeof(reg)) != 8)
				continue;

			pci_bridges[n].iot  = (bus_space_tag_t)reg[0];
			pci_bridges[n].addr = mapiodev(0xfec00000, 4); /* XXX */
			pci_bridges[n].data = mapiodev(0xfee00000, 4); /* XXX */
			pci_bridges[n].pc = PCI_CHIPSET_MPC106; /* for now */

			if (OF_getprop(node, "bus-range",
				       reg, sizeof(reg)) != 8) {
				pci_bridges[n].addr = NULL;
				continue;
			}
			pci_bridges[n].bus = reg[0];

			child = OF_child(node);
			while (child) {
				config_slot(child, pci_bridges[n].pc);
				child = OF_peer(child);
			}
			n++;
		}

		node = OF_peer(node);
	}
}

void
config_slot(node, pc)
	int node;
	pci_chipset_tag_t pc;
{
	pcitag_t tag;
	int sp, irq, intr, csr;
	int bus, dev, func;
	int sz;
	u_int reg[40], *rp;

	sz = OF_getprop(node, "assigned-addresses", reg, sizeof(reg));
	if (sz < 4)
		return;

	/*
	 * npt000ss bbbbbbbb dddddfff rrrrrrrr
	 *
	 * ss	space code (01:I/O, 10:32bit mem)
	 * b...	8-bit Bus Number
	 * d...	5-bit Device Number
	 * f...	3-bit Function Number
	 * r... 8-bit Register Number
	 */
	rp = &reg[0];
	bus = (*rp >> 16) & 0xff;
	dev = (*rp >> 11) & 0x1f;
	func = (*rp >> 8) & 0x07;

	tag = pci_make_tag(pc, bus, dev, func);
	csr  = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);

	/* Fix mem/io bits */
	while (sz > 0) {
		sp = (*rp >> 24) & 0x03;
		if (sp == 1)
			csr |= PCI_COMMAND_IO_ENABLE;
		if (sp == 2)
			csr |= PCI_COMMAND_MEM_ENABLE;
		sz -= 5 * sizeof(int);
		rp += 5;
	}

	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

	/* Fix intr bits */
	if (OF_getprop(node, "AAPL,interrupts", &irq, sizeof(irq)) ==
			sizeof(irq)) {

		intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
		intr = (intr & 0xffffff00) | (irq & 0xff);
		pci_conf_write(pc, tag, PCI_INTERRUPT_REG, intr);
	} else if (OF_getprop(node, "interrupts", &irq, sizeof(irq)) ==
			sizeof(irq)) {
		/* XXX USB on iMac */
		if (bus == 0 && dev == 20 && func == 0 && irq == 1)
			irq = 28;
		intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
		intr = (intr & 0xffffff00) | (irq & 0xff);
		pci_conf_write(pc, tag, PCI_INTERRUPT_REG, intr);
	}
}

/*
 * slot A1 pci0 dev 13 irq 23
 *      B1 pci0 dev 14 irq 24
 *      C1 pci0 dev 15 irq 25
 *      D2 pci1 dev 13 irq 27
 *      E2 pci1 dev 14 irq 28
 *      F2 pci1 dev 15 irq 29
 */
