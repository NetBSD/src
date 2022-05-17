/*	$NetBSD: pcib.c,v 1.11 2022/05/17 05:05:20 andvar Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.11 2022/05/17 05:05:20 andvar Exp $");

#include "isa.h"
#include "isadma.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/isa/isavar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

int	pcibmatch(device_t, cfdata_t, void *);
void	pcibattach(device_t, device_t, void *);

struct pcib_softc {
	device_t sc_dev;
	struct powerpc_isa_chipset *sc_chipset;
};

extern struct powerpc_isa_chipset genppc_ict;
extern struct genppc_pci_chipset *genppc_pct;

CFATTACH_DECL_NEW(pcib, sizeof(struct pcib_softc),
    pcibmatch, pcibattach, NULL, NULL);

void	pcib_callback(device_t);

int
pcibmatch(device_t parent, cfdata_t cf, void *aux)
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
pcibattach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pcib_softc *sc = device_private(self);
	char devinfo[256];
	u_int32_t v;
	int lvlmask = 0;
#ifdef prep
	prop_bool_t rav;
#endif

	sc->sc_dev = self;

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	v = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x40);
	if ((v & 0x20) == 0) {
		aprint_verbose_dev(self, "PIRQ[0-3] not used\n");
	} else {
		v = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x60);
		if ((v & 0x80808080) == 0x80808080) {
			aprint_verbose_dev(self, "PIRQ[0-3] disabled\n");
		} else {
			int i;
			aprint_verbose("%s:", device_xname(self));
			for (i = 0; i < 4; i++, v >>= 8) {
				if ((v & 0x80) == 0 && (v & 0x0f) != 0) {
					aprint_verbose(" PIRQ[%d]=%d", i,
					    v & 0x0f);
					lvlmask |= (1 << (v & 0x0f));
				}
			}
			aprint_verbose("\n");
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
	 * is improperly initialized, causing all sorts of weird interrupt
	 * issues on the machine.  The manual says it should default to
	 * 0000h (index 45-44h) however it would appear that PPCBUG is
	 * setting it up differently.  Reset it to 0000h.
	 */

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SYMPHONY &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SYMPHONY_83C553) {
		v = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x44) & 0xffff0000;
		pci_conf_write(pa->pa_pc, pa->pa_tag, 0x44, v);
	}

#ifdef prep
	rav = prop_dictionary_get(device_properties(parent),
	    "prep-raven-pchb");

	if (rav != NULL && prop_bool_true(rav) &&
	    PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SYMPHONY &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SYMPHONY_83C553) {

		prop_dictionary_t dict, devsub;
		prop_number_t pinsub;
		struct genppc_pci_chipset_businfo *pbi;

		v = pci_conf_read(pa->pa_pc, pa->pa_tag, 0x40) & 0x00ffffff;
		v |= 0xee000000;
		pci_conf_write(pa->pa_pc, pa->pa_tag, 0x40, v);

		pbi = SIMPLEQ_FIRST(&genppc_pct->pc_pbi);
		dict = prop_dictionary_get(pbi->pbi_properties,
		    "prep-pci-intrmap");
		devsub = prop_dictionary_get(dict, "devfunc-11");
		pinsub = prop_number_create_integer(14);
		prop_dictionary_set(devsub, "pin-A", pinsub);
		prop_object_release(pinsub);
		aprint_verbose_dev(self, "setting pciide irq to 14\n");
		/* irq 14 is level */
		lvlmask = 0x0040;
	}
#endif /* prep */

	config_defer(self, pcib_callback);
}

void
pcib_callback(device_t self)
{
	struct pcib_softc *sc = device_private(self);
#if NISA > 0
	struct isabus_attach_args iba;

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&iba, 0, sizeof(iba));
	sc->sc_chipset = &genppc_ict;
	iba.iba_ic = sc->sc_chipset;
	iba.iba_iot = &genppc_isa_io_space_tag;
	iba.iba_memt = &genppc_isa_mem_space_tag;
#if NISADMA > 0
	iba.iba_dmat = &isa_bus_dma_tag;
#endif
	config_found(sc->sc_dev, &iba, isabusprint, CFARGS_NONE);
#endif
}
