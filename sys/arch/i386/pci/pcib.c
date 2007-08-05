/*	$NetBSD: pcib.c,v 1.42.26.1 2007/08/05 16:38:54 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pcib.c,v 1.42.26.1 2007/08/05 16:38:54 jmcneill Exp $");

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

struct pcib_softc {
	struct device		sc_dev;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;
	struct pci_conf_state	sc_pciconf;
};

int	pcibmatch(struct device *, struct cfdata *, void *);
void	pcibattach(struct device *, struct device *, void *);

static pnp_status_t	pcib_power(device_t, pnp_request_t, void *);

CFATTACH_DECL(pcib, sizeof(struct pcib_softc),
    pcibmatch, pcibattach, NULL, NULL);

void	pcib_callback(struct device *);

int
pcibmatch(struct device *parent, struct cfdata *match,
    void *aux)
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
			{
				extern int clock_broken_latch;

				clock_broken_latch = 0;
			}
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
pcibattach(struct device *parent, struct device *self, void *aux)
{
	struct pcib_softc *sc = (struct pcib_softc *)self;
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	aprint_naive("\n");
	aprint_normal("\n");

	/*
	 * Just print out a description and defer configuration
	 * until all PCI devices have been attached.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;

	if (pnp_register(self, pcib_power) != PNP_STATUS_SUCCESS)
		aprint_error("%s: couldn't establish power handler\n",
		    device_xname(self));

	config_defer(self, pcib_callback);
}

static pnp_status_t
pcib_power(device_t dv, pnp_request_t req, void *opaque)
{
	struct pcib_softc *sc;
	pnp_capabilities_t *pcaps;
	pnp_state_t *pstate;
	pci_chipset_tag_t pc;
	pcitag_t tag;

	sc = (struct pcib_softc *)dv;
	pc = sc->sc_pc;
	tag = sc->sc_tag;

	switch (req) {
	case PNP_REQUEST_GET_CAPABILITIES:
		pcaps = opaque;
		pcaps->state = PNP_STATE_D0 | PNP_STATE_D3;
		break;
	case PNP_REQUEST_GET_STATE:
		pstate = opaque;
		*pstate = PNP_STATE_D0; /* XXX */
		break;
	case PNP_REQUEST_SET_STATE:
		pstate = opaque;
		switch (*pstate) {
		case PNP_STATE_D0:
			pci_conf_restore(pc, tag, &sc->sc_pciconf);
			break;
		case PNP_STATE_D3:
			pci_conf_capture(pc, tag, &sc->sc_pciconf);
			break;
		default:
			return PNP_STATUS_UNSUPPORTED;
		}
		break;
	default:
		return PNP_STATUS_UNSUPPORTED;
	}

	return PNP_STATUS_SUCCESS;
}

void
pcib_callback(struct device *self)
{
	struct isabus_attach_args iba;

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	memset(&iba, 0, sizeof(iba));
	iba.iba_iot = X86_BUS_SPACE_IO;
	iba.iba_memt = X86_BUS_SPACE_MEM;
#if NISA > 0
	iba.iba_dmat = &isa_bus_dma_tag;
#endif
	config_found_ia(self, "isabus", &iba, isabusprint);
}
