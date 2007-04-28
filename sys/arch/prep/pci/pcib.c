/*	$NetBSD: pcib.c,v 1.18.6.1 2007/04/28 18:07:15 bouyer Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.18.6.1 2007/04/28 18:07:15 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/residual.h>

#include <dev/isa/isavar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

#include "isa.h"
#include "isadma.h"

int	pcibmatch(struct device *, struct cfdata *, void *);
void	pcibattach(struct device *, struct device *, void *);

struct pcib_softc {
	struct device sc_dev;
	struct prep_isa_chipset *sc_chipset;
};

extern struct prep_isa_chipset prep_isa_chipset;
extern struct prep_pci_chipset *prep_pct;

CFATTACH_DECL(pcib, sizeof(struct pcib_softc),
    pcibmatch, pcibattach, NULL, NULL);

void	pcib_callback(struct device *);

int
pcibmatch(struct device *parent, struct cfdata *cf, void *aux)
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
pcibattach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	u_int32_t v;
	int lvlmask = 0;

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

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

	/*
	 * If we have an 83C553F-G sitting on a RAVEN host bridge,
	 * then we need to rewire some interrupts.
	 * The IDE Interrupt Routing Control Register lives at 0x43,
	 * and defaults to 0xEF, which means the primary controller
	 * interrupts on ivr-14, and the secondary on ivr-15. We
	 * reset it to 0xEE to fire them both at ivr-14.
	 * We have to rewrite the interrupt map, because the bridge map told
	 * us that the interrupt is MPIC 0, which is the bridge intr for
	 * the 8259.
	 * Additionally, sometimes the PCI Interrupt Routing Control Register
	 * is improperly initialized, causing all sorts of wierd interrupt
	 * issues on the machine.  The manual says it should default to
	 * 0000h (index 45-44h) however it would appear that PPCBUG is
	 * setting it up differently.  Reset it to 0000h.
	 */

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SYMPHONY &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SYMPHONY_83C553 &&
	    strcmp(res->VitalProductData.PrintableModel,
		"000000000000000000000000000(e2)") == 0) {

		prop_dictionary_t dict, devsub;
		prop_number_t pinsub;
		struct prep_pci_chipset_businfo *pbi;

		v = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x40) & 0x00ffffff;
		v |= 0xee000000;
		pci_conf_write(pa->pa_pc, pa->pa_tag, 0x40, v);

		v = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x44) & 0xffff0000;
		pci_conf_write(pa->pa_pc, pa->pa_tag, 0x44, v);

		pbi = SIMPLEQ_FIRST(&prep_pct->pc_pbi);
		dict = prop_dictionary_get(pbi->pbi_properties,
		    "prep-pci-intrmap");
		devsub = prop_dictionary_get(dict, "devfunc-11");
		pinsub = prop_number_create_integer(14);
		prop_dictionary_set(devsub, "pin-A", pinsub);
		prop_object_release(pinsub);
		aprint_verbose("%s: setting pciide irq to 14\n",
		    self->dv_xname);
		/* irq 14 is level */
		lvlmask = 0x0040;
	}

#if NISA > 0
	/* if the lvlmask is different, reinitialize the icu, because we
	 * set it to zero in mainbus_attach()
	 */
	if (lvlmask)
		init_icu(lvlmask);
#endif

	config_defer(self, pcib_callback);
}

void
pcib_callback(struct device *self)
{
	struct pcib_softc *sc = (struct pcib_softc *)self;
#if NISA > 0
	struct isabus_attach_args iba;

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&iba, 0, sizeof(iba));
	sc->sc_chipset = &prep_isa_chipset;
	iba.iba_ic = sc->sc_chipset;
	iba.iba_iot = &prep_isa_io_space_tag;
	iba.iba_memt = &prep_isa_mem_space_tag;
#if NISADMA > 0
	iba.iba_dmat = &isa_bus_dma_tag;
#endif
	config_found_ia(&sc->sc_dev, "isabus", &iba, isabusprint);
#endif
}
