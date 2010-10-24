/*	$NetBSD: pcib.c,v 1.7.8.3 2010/10/24 22:48:17 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.7.8.3 2010/10/24 22:48:17 jym Exp $");

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
#include "pcibvar.h"

int	pcibmatch(device_t, cfdata_t, void *);

CFATTACH_DECL3_NEW(pcib, sizeof(struct pcib_softc),
    pcibmatch, pcibattach, pcibdetach, NULL, pcibrescan, pcibchilddet,
    DVF_DETACH_SHUTDOWN);

void	pcib_callback(device_t);

int
pcibmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

#if 0
	/*
	 * PCI-ISA bridges are matched on class/subclass.
	 * This list contains only the bridges where correct
	 * (or incorrect) behaviour is not yet confirmed.
	 */
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82426EX:
		case PCI_PRODUCT_INTEL_82380AB:
			return (1);
		}
		break;

	case PCI_VENDOR_UMC:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_UMC_UM8886F:
		case PCI_PRODUCT_UMC_UM82C886:
			return (1);
		}
		break;
	case PCI_VENDOR_ALI:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_ALI_M1449:
		case PCI_PRODUCT_ALI_M1543:
			return (1);
		}
		break;
	case PCI_VENDOR_COMPAQ:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_COMPAQ_PCI_ISA_BRIDGE:
			return (1);
		}
		break;
	case PCI_VENDOR_VIATECH:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_VIATECH_VT82C570MV:
		case PCI_PRODUCT_VIATECH_VT82C586_ISA:
			return (1);
		}
		break;
	}
#endif

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
		case PCI_PRODUCT_INTEL_82371MX:
			/*
			 * The Intel 82371MX identifies itself erroneously as a
			 * miscellaneous bridge.
			 */
		case PCI_PRODUCT_INTEL_82371AB_ISA:
			/*
			 * Some Intel 82371AB PCI-ISA bridge identifies
			 * itself as miscellaneous bridge.
			 */
			return (1);
		}
		break;
	case PCI_VENDOR_SIS:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_SIS_85C503:
			/*
			 * The SIS 85C503 identifies itself as a
			 * miscellaneous prehistoric.
			 */
			return (1);
		}
		break;
	case PCI_VENDOR_VIATECH:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_VIATECH_VT82C686A_SMB:
			/*
			 * The VIA VT82C686A SMBus Controller itself as 
			 * ISA bridge, but it's wrong !
			 */
			return (0);
		}
	/*
	 * The Cyrix cs5530 PCI host bridge does not have a broken
	 * latch on the i8254 clock core, unlike its predecessors
	 * the cs5510 and cs5520. This reverses the setting from
	 * i386/i386/identcpu.c where it arguably should not have
	 * been set in the first place. XXX
	 */
	case PCI_VENDOR_CYRIX:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_CYRIX_CX5530_PCIB:
#if !defined(XEN)
			{
				extern int clock_broken_latch;

				clock_broken_latch = 0;
			}
#endif
			return(1);
		}
		break;
	}

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_ISA) {
		return (1);
	}

	return (0);
}

void
pcibattach(device_t parent, device_t self, void *aux)
{
	struct pcib_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	aprint_naive("\n");

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;

	/* If a more specific pcib implementation has already registered a
	 * power handler, don't overwrite it.
	 */
 	if (!device_pmf_is_registered(self)) {
 		if (!pmf_device_register(self, NULL, NULL))
 	    		aprint_error_dev(self, "couldn't establish power handler\n");
	}

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
		iba.iba_iot = x86_bus_space_io;
		iba.iba_memt = x86_bus_space_mem;
#if NISA > 0
		iba.iba_dmat = &isa_bus_dma_tag;
#endif
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
