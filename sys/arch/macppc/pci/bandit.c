/*	$NetBSD: bandit.c,v 1.2 1998/07/13 19:27:13 tsubai Exp $	*/

/*
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
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
 * Copyright 1996 1995 by Apple Computer, Inc. 1997 1996 1995 1994 1993 1992 1991  
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

static void bandit_init __P((pci_chipset_tag_t));
static void scan_pci_devs __P((void));
static void config_slot __P((int));

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
		if (strcmp(name, "bandit") == 0) {
			int child;

			if (OF_getprop(node, "reg", reg, sizeof(reg)) != 8)
				continue;

			pci_bridges[n].iot  = (bus_space_tag_t)reg[0];
			pci_bridges[n].addr = mapiodev(reg[0] + 0x800000, 4);
			pci_bridges[n].data = mapiodev(reg[0] + 0xc00000, 4);
			pci_bridges[n].pc = n;
			bandit_init(n++);

			child = OF_child(node);
			while (child) {
				config_slot(child);
				child = OF_peer(child);
			}
		}
		if (strcmp(name, "pci") == 0) {	/* XXX This is not a bandit :) */
			int child;

			if (OF_getprop(node, "reg", reg, sizeof(reg)) != 8)
				continue;

			pci_bridges[n].iot  = (bus_space_tag_t)reg[0];
			pci_bridges[n].addr = mapiodev(0xfec00000, 4); /* XXX */
			pci_bridges[n].data = mapiodev(0xfee00000, 4); /* XXX */
			pci_bridges[n].pc = PCI_CHIPSET_MPC106; /* for now */
		}

		node = OF_peer(node);
	}
}

void
config_slot(node)
	int node;
{
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int dev, irq, intr, csr;
	char slotname[8];

	if (OF_getprop(node, "AAPL,slot-name", slotname, sizeof(slotname)) < 2)
		return;

	slotname[2] = 0;

	/*
	 * slot A1 pci0 dev 13 irq 23
	 *      B1 pci0 dev 14 irq 24
	 *      C1 pci0 dev 15 irq 25
	 *      D2 pci1 dev 13 irq 27
	 *      E2 pci1 dev 14 irq 28
	 *      F2 pci1 dev 15 irq 29
	 *
	 * XXX Is this TRUE on all PowerMacs?
	 */
	pc  = slotname[1] - '1';
	dev = slotname[0] - 'A' + 13 - pc * 3;

	if (OF_getprop(node, "AAPL,interrupts", &irq, sizeof(irq)) ==
			sizeof(irq)) {

		tag = pci_make_tag(pc, 0, dev, 0);
		csr  = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);

#ifdef BANDIT_DEBUG
		printf(" pc=%d, dev=%d\n", pc, dev);
		printf(" OLD csr=0x%x, intr=0x%x\n", csr, intr);
#endif

		intr = (intr & 0xffffff00) | (irq & 0xff);
		pci_conf_write(pc, tag, PCI_INTERRUPT_REG, intr);

		/* XXX */
		csr |= (PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_IO_ENABLE);
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

#ifdef BANDIT_DEBUG
		printf(" NEW csr=0x%x, intr=0x%x\n", csr, intr);
#endif
	}
}
