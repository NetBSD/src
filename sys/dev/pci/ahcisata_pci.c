/*	$NetBSD: ahcisata_pci.c,v 1.1.12.1 2007/08/04 18:20:51 he Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ahcisata_pci.c,v 1.1.12.1 2007/08/04 18:20:51 he Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/disklabel.h>
#include <sys/pnp.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/ic/ahcisatavar.h>

struct ahci_pci_softc {
	struct ahci_softc ah_sc; /* must come first, struct device */
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	struct pci_conf_state sc_pciconf;
};


static int  ahci_pci_match(struct device *, struct cfdata *, void *);
static void ahci_pci_attach(struct device *, struct device *, void *);

static pnp_status_t ahci_pci_power(device_t, pnp_request_t, void *);


CFATTACH_DECL(ahcisata_pci, sizeof(struct ahci_pci_softc),
    ahci_pci_match, ahci_pci_attach, NULL, NULL);

static int
ahci_pci_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa = aux;
	bus_space_tag_t regt;
	bus_space_handle_t regh;
	bus_size_t size;
	int ret = 0;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_MASS_STORAGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_SATA &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_SATA_AHCI) {
		/* check if the chip is in ahci mode */
		if (pci_mapreg_map(pa, AHCI_PCI_ABAR,
		    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
		    &regt, &regh, NULL, &size) != 0)
			return (0);
		if (bus_space_read_4(regt, regh, AHCI_GHC) & AHCI_GHC_AE)
			ret = 3;
		bus_space_unmap(regt, regh, size);
		return (3);
	}

	return (ret);
}

static void
ahci_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct ahci_pci_softc *psc = (struct ahci_pci_softc *)self;
	struct ahci_softc *sc = &psc->ah_sc;
	bus_size_t size;
	char devinfo[256];
	const char *intrstr;
	pci_intr_handle_t intrhandle;
	void *ih;

	if (pci_mapreg_map(pa, AHCI_PCI_ABAR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_ahcit, &sc->sc_ahcih, NULL, &size) != 0) {
		aprint_error("%s: can't map ahci registers\n", AHCINAME(sc));
		return;
	}
	psc->sc_pc = pa->pa_pc;
	psc->sc_pcitag = pa->pa_tag;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_naive(": AHCI disk controller\n");
	aprint_normal(": %s\n", devinfo);
	
	if (pci_intr_map(pa, &intrhandle) != 0) {
		aprint_error("%s: couldn't map interrupt\n", AHCINAME(sc));
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, intrhandle);
	ih = pci_intr_establish(pa->pa_pc, intrhandle, IPL_BIO, ahci_intr, sc);
	if (ih == NULL) {
		aprint_error("%s: couldn't establish interrupt", AHCINAME(sc));
		return;
	}
	aprint_normal("%s: interrupting at %s\n", AHCINAME(sc),
	    intrstr ? intrstr : "unknown interrupt");
	sc->sc_dmat = pa->pa_dmat;
	ahci_attach(sc);

	/* register device power management */
	pnp_register(self, ahci_pci_power);
}

static pnp_status_t
ahci_pci_power(device_t dv, pnp_request_t req, void *opaque)
{
	struct ahci_pci_softc *psc = (struct ahci_pci_softc *)dv;
	struct ahci_softc *sc = &psc->ah_sc;
	pnp_status_t status;
	pnp_state_t *state;
	pnp_capabilities_t *caps;
	pci_chipset_tag_t pc;
	pcireg_t val;
	pcitag_t tag;
	int off, s;

	status = PNP_STATUS_UNSUPPORTED;
	pc = psc->sc_pc;
	tag = psc->sc_pcitag;

	switch (req) {
	case PNP_REQUEST_GET_CAPABILITIES:
		caps = opaque;

		if (!pci_get_capability(pc, tag, PCI_CAP_PWRMGMT, &off, &val))
			return PNP_STATUS_UNSUPPORTED;
		caps->state = pci_pnp_capabilities(val);
		status = PNP_STATUS_SUCCESS;
		break;
	case PNP_REQUEST_SET_STATE:
		state = opaque;
		switch (*state) {
		case PNP_STATE_D0:
			val = PCI_PMCSR_STATE_D0;
			break;
		case PNP_STATE_D3:
			val = PCI_PMCSR_STATE_D3;
			s = splbio();
			pci_conf_capture(pc, tag, &psc->sc_pciconf);
			splx(s);
			break;
		default:
			return PNP_STATUS_UNSUPPORTED;
		}

		if (pci_set_powerstate(pc, tag, val) == 0) {
			status = PNP_STATUS_SUCCESS;
			if (*state != PNP_STATE_D0)
				break;

			s = splbio();
			pci_conf_restore(pc, tag, &psc->sc_pciconf);

			ahci_reset(sc);
			ahci_setup_ports(sc);
			ahci_reprobe_drives(sc);
			ahci_enable_intrs(sc);

			splx(s);
		}
	case PNP_REQUEST_GET_STATE:
		state = opaque;
		if (pci_get_powerstate(pc, tag, &val) != 0)
			return PNP_STATUS_UNSUPPORTED;

		*state = pci_pnp_powerstate(val);
		status = PNP_STATUS_SUCCESS;
		break;
	default:
		status = PNP_STATUS_UNSUPPORTED;
		break;
	}
	return status;
}
