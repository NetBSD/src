/*	$NetBSD: pcib.c,v 1.9.2.1 2001/08/03 04:11:20 lukem Exp $	*/

/*-
 * Copyright (c) 1996, 1998 The NetBSD Foundation, Inc.
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

#include <dev/isa/isavar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

#include "isa.h"

int	pcibmatch __P((struct device *, struct cfdata *, void *));
void	pcibattach __P((struct device *, struct device *, void *));

struct pcib_softc {
	struct device sc_dev;
	struct bebox_isa_chipset sc_chipset;
};

struct cfattach pcib_ca = {
	sizeof(struct pcib_softc), pcibmatch, pcibattach
};

void	pcib_callback __P((struct device *));
int	pcib_print __P((void *, const char *));

extern const struct powerpc_bus_space bebox_isa_io_bs_tag;
extern const struct powerpc_bus_space bebox_isa_mem_bs_tag;

int
pcibmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match SIO(82378ZB) PCI-ISA bridge.
	 */
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL ||
	    PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_INTEL_SIO)
			return (0);

	return (1);
}

void
pcibattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	printf("\n");

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	config_defer(self, pcib_callback);
}

void
pcib_callback(self)
	struct device *self;
{
	struct pcib_softc *sc = (struct pcib_softc *)self;
	struct isabus_attach_args iba;

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&iba, 0, sizeof(iba));
	iba.iba_busname = "isa";
	iba.iba_ic = &sc->sc_chipset;
	iba.iba_iot = &bebox_isa_io_bs_tag;
	iba.iba_memt = &bebox_isa_mem_bs_tag;
#if NISA > 0
	iba.iba_dmat = &isa_bus_dma_tag;

#if 0
/*
 * XXX THIS WILL NOT WORK, BUT ADDING SUPPORT FOR HIGHPAGE MIGHT BE
 * XXX WORTH WHILE EVENTUALLY.
 */
#define SIO_DMAHIGHPAGE	0x480
	if (bus_space_map(iba.iba_iot, SIO_DMAHIGHPAGE, 0xf, 0,
	    (bus_space_handle_t *)&iba.iba_ic))
		panic("pcib_callback: can't map DMA high page registers");
#endif
#endif
	config_found(&sc->sc_dev, &iba, pcib_print);
}

int
pcib_print(aux, pnp)
	void *aux;
	const char *pnp;
{

	/* Only ISAs can attach to pcib's; easy. */
	if (pnp)
		printf("isa at %s", pnp);
	return (UNCONF);
}
