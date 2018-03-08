/*	$NetBSD: pcib.c,v 1.2 2018/03/08 18:48:25 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.2 2018/03/08 18:48:25 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>


#include <evbmips/loongson/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/isa/isavar.h>

#include <evbmips/loongson/loongson_bus_defs.h>
#include <evbmips/loongson/dev/pcibvar.h>

#include "isa.h"

int	pcibmatch(device_t, cfdata_t, void *);
void	pcib_callback(device_t);

CFATTACH_DECL3_NEW(pcib, sizeof(struct pcib_softc),
    pcibmatch, pcibattach, pcibdetach, NULL, pcibrescan, pcibchilddet,
	DVF_DETACH_SHUTDOWN);


int
pcibmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_SIO:
		case PCI_PRODUCT_INTEL_82371MX:
		case PCI_PRODUCT_INTEL_82371AB_ISA:
		case PCI_PRODUCT_INTEL_82440MX_ISA:
			/* The above bridges mis-identify themselves */
			return (1);
		}
		break;
	case PCI_VENDOR_SIS:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_SIS_85C503:
			/* mis-identifies itself as a miscellaneous prehistoric */
			return (1);
		}
		break;
	case PCI_VENDOR_VIATECH:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_VIATECH_VT82C686A_PWR:
			/* mis-identifies itself as a ISA bridge */
			return (0);
		}
		break;
	}

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_ISA)
		return (1);

	return (0);
}

void
pcibattach(device_t parent, device_t self, void *aux)
{
	struct pcib_softc *sc = device_private(self);
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	char devinfo[256];

	aprint_naive("\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	/*
	 * Wait until all PCI devices are attached before attaching isa;
	 * otherwise this might mess the interrupt setup on some systems.
	 */
	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	config_defer(self, pcib_callback);
}

int
pcibdetach(device_t self, int flags)
{
	int rc;

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;
	pmf_device_deregister(self);
	return 0;
}

void
pcibchilddet(device_t self, device_t child)
{
	struct pcib_softc *sc = device_private(self);

	if (sc->sc_isabus == child)
		sc->sc_isabus = NULL;
}

int
pcibrescan(device_t self, const char *ifattr, const int *loc)
{
	struct pcib_softc *sc = device_private(self);
	struct isabus_attach_args iba;

	if (ifattr_match(ifattr, "isabus") && sc->sc_isabus == NULL) {
		/*
		 * Attach the ISA bus behind this bridge.
		 */
		memset(&iba, 0, sizeof(iba));
		iba.iba_iot = &bonito_iot;
		iba.iba_memt = &bonito_memt;
#if NISA > 0
		iba.iba_dmat = &bonito_dmat;
#endif
		iba.iba_ic = sys_platform->isa_chipset;

		if (iba.iba_ic != NULL) 
			sc->sc_isabus =
			    config_found_ia(self, "isabus", &iba, isabusprint);
	}
	return 0;
}


void
pcib_callback(device_t self)
{
	pcibrescan(self, "isabus", NULL);
}
