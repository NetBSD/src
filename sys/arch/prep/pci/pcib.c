/*	$NetBSD: pcib.c,v 1.3 2000/12/22 18:16:37 tsutsui Exp $	*/

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
	struct prep_isa_chipset sc_chipset;
};

struct cfattach pcib_ca = {
	sizeof(struct pcib_softc), pcibmatch, pcibattach
};

void	pcib_callback __P((struct device *));
int	pcib_print __P((void *, const char *));

int
pcibmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match PCI-ISA bridge.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_ISA) {
		return (1);
	}

	/*
	 * some special cases:
	 */
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_SIO:
			/*
			 * The Intel SIO identifies itself as a
			 * miscellaneous prehistoric.
			 */
			return (1);
		}
		break;
	}

	return (0);
}

void
pcibattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	u_int32_t v;
	int lvlmask = 0;

	printf("\n");

	v = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x40);
	if ((v & 0x20) == 0) {
		printf("%s: PIRQ[0-3] not used\n", self->dv_xname);
	} else {
		v = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x60);
		if ((v & 0x80808080) == 0x80808080) {
			printf("%s: PIRQ[0-3] disabled\n", self->dv_xname);
		} else {
			int i;
			printf("%s:", self->dv_xname);
			for (i = 0; i < 4; i++, v >>= 8) {
				if ((v & 80) == 0 && (v & 0x0f) != 0) {
					printf(" PIRQ[%d]=%d", i, v & 0x0f);
					lvlmask |= (1 << (v & 0x0f));
				}
			}
			printf("\n");
		}
	}
#if NISA > 0
	/* Initialize interrupt controller */
	init_icu(lvlmask);
#endif

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
	bzero(&iba, sizeof(iba));
	iba.iba_busname = "isa";
	iba.iba_ic = &sc->sc_chipset;
	iba.iba_iot = &prep_isa_io_space_tag;
	iba.iba_memt = &prep_isa_mem_space_tag;
#if NISA > 0
	iba.iba_dmat = &isa_bus_dma_tag;
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
