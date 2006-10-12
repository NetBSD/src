/* $NetBSD: if_mtd_pci.c,v 1.8 2006/10/12 01:31:30 christos Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Peter Bex <Peter.Bex@student.kun.nl>.
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
 * PCI interface for MTD803 cards
 * Written by Peter Bex (peter.bex@student.kun.nl)
 */

/* TODO: Check why in IO space, the MII won't work. Memory mapped works */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mtd_pci.c,v 1.8 2006/10/12 01:31:30 christos Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <machine/bus.h>
#include <dev/mii/miivar.h>
#include <dev/ic/mtd803reg.h>
#include <dev/ic/mtd803var.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#define PCI_IO_MAP_REG		0x10
#define PCI_MEM_MAP_REG		0x14

struct mtd_pci_device_id {
	pci_vendor_id_t		vendor;		/* PCI vendor ID */
	pci_product_id_t	product;	/* PCI product ID */
};

static struct mtd_pci_device_id mtd_ids[] = {
	{ PCI_VENDOR_MYSON, PCI_PRODUCT_MYSON_MTD803 },
	{ 0, 0 }
};

static int	mtd_pci_match(struct device *, struct cfdata *, void *);
static void	mtd_pci_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mtd_pci, sizeof(struct mtd_softc), mtd_pci_match, mtd_pci_attach,
    NULL, NULL);

static int
mtd_pci_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct pci_attach_args *pa = aux;
	struct mtd_pci_device_id *id;

	for (id = mtd_ids; id->vendor != 0; ++id) {
		if (PCI_VENDOR(pa->pa_id) == id->vendor &&
		    PCI_PRODUCT(pa->pa_id) == id->product)
			return (1);
	}
	return (0);
}

static void
mtd_pci_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct pci_attach_args * const pa = aux;
	struct mtd_softc * const sc = (void *)self;
	pci_intr_handle_t ih;
	const char *intrstring = NULL;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	int io_valid, mem_valid;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	io_valid = (pci_mapreg_map(pa, PCI_IO_MAP_REG, PCI_MAPREG_TYPE_IO,
			0, &iot, &ioh, NULL, NULL) == 0);
	mem_valid = (pci_mapreg_map(pa, PCI_MEM_MAP_REG, PCI_MAPREG_TYPE_MEM
			| PCI_MAPREG_MEM_TYPE_32BIT, 0, &memt, &memh,
			NULL, NULL) == 0);

	if (mem_valid) {
		sc->bus_tag = memt;
		sc->bus_handle = memh;
	} else if (io_valid) {
		sc->bus_tag = iot;
		sc->bus_handle = ioh;
	} else {
		printf("%s: could not map memory or i/o space\n",
			sc->dev.dv_xname);
		return;
	}
	sc->dma_tag = pa->pa_dmat;

	/* Do generic attach. Seems this must be done before setting IRQ */
	mtd_config(sc);

	if (pci_intr_map(pa, &ih)) {
		printf("%s: could not map interrupt\n", sc->dev.dv_xname);
		return;
	}
	intrstring = pci_intr_string(pa->pa_pc, ih);

	if (pci_intr_establish(pa->pa_pc, ih, IPL_NET, mtd_irq_h, sc) == NULL) {
		printf("%s: could not establish interrupt", sc->dev.dv_xname);
		if (intrstring != NULL)
			printf(" at %s", intrstring);
		printf("\n");
		return;
	} else {
		printf("%s: using %s for interrupt\n",
			sc->dev.dv_xname,
			intrstring ? intrstring : "unknown interrupt");
	}
}
