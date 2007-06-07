/*	$NetBSD: obio.c,v 1.26.14.1 2007/06/07 20:30:44 garbled Exp $	*/

/*-
 * Copyright (C) 1998	Internet Research Institute, Inc.
 * All rights reserved.
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
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.26.14.1 2007/06/07 20:30:44 garbled Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

static void obio_attach(struct device *, struct device *, void *);
static int obio_match(struct device *, struct cfdata *, void *);
static int obio_print(void *, const char *);

struct obio_softc {
	struct device sc_dev;
	int sc_node;
};


CFATTACH_DECL(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

int
obio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE)
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_APPLE_GC:
		case PCI_PRODUCT_APPLE_OHARE:
		case PCI_PRODUCT_APPLE_HEATHROW:
		case PCI_PRODUCT_APPLE_PADDINGTON:
		case PCI_PRODUCT_APPLE_KEYLARGO:
		case PCI_PRODUCT_APPLE_PANGEA_MACIO:
		case PCI_PRODUCT_APPLE_INTREPID:
		case PCI_PRODUCT_APPLE_K2:
			return 1;
		}

	return 0;
}

/*
 * Attach all the sub-devices we can find
 */
void
obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_softc *sc = (struct obio_softc *)self;
	struct pci_attach_args *pa = aux;
	struct confargs ca;
	bus_space_handle_t bsh;
	int node, child, namelen, error;
	u_int reg[20];
	int intr[6], parent_intr = 0, parent_nintr = 0;
	char name[32];
	char compat[32];

	switch (PCI_PRODUCT(pa->pa_id)) {

	case PCI_PRODUCT_APPLE_GC:
	case PCI_PRODUCT_APPLE_OHARE:
	case PCI_PRODUCT_APPLE_HEATHROW:
	case PCI_PRODUCT_APPLE_PADDINGTON:
	case PCI_PRODUCT_APPLE_KEYLARGO:
	case PCI_PRODUCT_APPLE_PANGEA_MACIO:
	case PCI_PRODUCT_APPLE_INTREPID:
		node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
		if (node == -1)
			node = OF_finddevice("mac-io");
			if (node == -1)
				node = OF_finddevice("/pci/mac-io");
		break;
	case PCI_PRODUCT_APPLE_K2:
		node = OF_finddevice("mac-io");
		break;

	default:
		node = -1;
		break;
	}
	if (node == 1)
		panic("macio not found or unknown");

	sc->sc_node = node;

#if defined (PMAC_G5)
	if (OF_getprop(node, "assigned-addresses", reg, sizeof(reg)) < 20)
	{
		return;
	}
#else
	if (OF_getprop(node, "assigned-addresses", reg, sizeof(reg)) < 12)
		return;
#endif /* PMAC_G5 */

	ca.ca_baseaddr = reg[2];
	error = bus_space_map (ca.ca_tag, ca.ca_baseaddr, 0x80, 0, &bsh);
	if (error)
		panic(": failed to map mac-io %#x", ca.ca_baseaddr);

	aprint_verbose(": addr 0x%x\n", ca.ca_baseaddr);

	/* Enable internal modem (KeyLargo) */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_KEYLARGO) {
		aprint_normal("%s: enabling KeyLargo internal modem\n",
		    self->dv_xname);
		bus_space_write_4(ca.ca_tag, bsh, 0x40, 
		    bus_space_read_4(ca.ca_tag, bsh, 0x40) & ~(1<<25));
	}

	/* Enable internal modem (Pangea) */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_PANGEA_MACIO) {
		bus_space_write_1(ca.ca_tag, bsh, 0x006a + 0x03, 0x04); /* set reset */
		bus_space_write_1(ca.ca_tag, bsh, 0x006a + 0x02, 0x04); /* power modem on */
		bus_space_write_1(ca.ca_tag, bsh, 0x006a + 0x03, 0x05); /* unset reset */ 
	}

	/* Gatwick and Paddington use same product ID */
	namelen = OF_getprop(node, "compatible", compat, sizeof(compat));

	if (strcmp(compat, "gatwick") == 0) {
		parent_nintr = OF_getprop(node, "AAPL,interrupts", intr,
					sizeof(intr));
		parent_intr = intr[0];
	} else {
  		/* Enable CD and microphone sound input. */
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_PADDINGTON)
			bus_space_write_1(ca.ca_tag, bsh, 0x37, 0x03);
	}
	bus_space_unmap(ca.ca_tag, bsh, 0x80);
  
	for (child = OF_child(node); child; child = OF_peer(child)) {
		namelen = OF_getprop(child, "name", name, sizeof(name));
		if (namelen < 0)
			continue;
		if (namelen >= sizeof(name))
			continue;

		name[namelen] = 0;
		ca.ca_name = name;
		ca.ca_node = child;
		ca.ca_tag = pa->pa_memt;

		ca.ca_nreg = OF_getprop(child, "reg", reg, sizeof(reg));

		if (strcmp(compat, "gatwick") != 0) {
			ca.ca_nintr = OF_getprop(child, "AAPL,interrupts", intr,
					sizeof(intr));
			if (ca.ca_nintr == -1)
				ca.ca_nintr = OF_getprop(child, "interrupts", intr,
						sizeof(intr));
		} else {
			intr[0] = parent_intr;
			ca.ca_nintr = parent_nintr;
		}
		ca.ca_reg = reg;
		ca.ca_intr = intr;

		config_found(self, &ca, obio_print);
	}
}

static const char * const skiplist[] = {
	"interrupt-controller",
	"gpio",
	"escc-legacy",
	"timer",
	"i2c",
	"power-mgt",
	"escc"
	
};

#define N_LIST (sizeof(skiplist) / sizeof(skiplist[0]))

int
obio_print(aux, obio)
	void *aux;
	const char *obio;
{
	struct confargs *ca = aux;
	int i;

	for (i = 0; i < N_LIST; i++)
		if (strcmp(ca->ca_name, skiplist[i]) == 0)
			return QUIET;

	if (obio)
		aprint_normal("%s at %s", ca->ca_name, obio);

	if (ca->ca_nreg > 0)
		aprint_normal(" offset 0x%x", ca->ca_reg[0]);

	return UNCONF;
}
