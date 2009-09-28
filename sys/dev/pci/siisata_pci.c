/* $NetBSD: siisata_pci.c,v 1.1.14.2 2009/09/28 00:17:28 snj Exp $ */
/* Id: siisata_pci.c,v 1.11 2008/05/21 16:20:11 jakllsch Exp  */

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

/*-
 * Copyright (c) 2007, 2008 Jonathan A. Kollasch.
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


#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/ic/siisatavar.h>

struct siisata_pci_softc {
	struct siisata_softc si_sc;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	void * sc_ih;
};

static int siisata_pci_match(device_t, cfdata_t, void *);
static void siisata_pci_attach(device_t, device_t, void *);
static int siisata_pci_detach(device_t, int);
static bool siisata_pci_resume(device_t PMF_FN_PROTO);

static const struct siisata_pci_product {
	pci_vendor_id_t spp_vendor;
	pci_product_id_t spp_product;
	int spp_ports;
	int spp_chip;

}                   siisata_pci_products[] = {
	{
		PCI_VENDOR_CMDTECH, PCI_PRODUCT_CMDTECH_3124,
		4, 3124
	},
	{
		PCI_VENDOR_CMDTECH, PCI_PRODUCT_CMDTECH_3132,
		2, 3132
	},
	{
		PCI_VENDOR_CMDTECH, PCI_PRODUCT_CMDTECH_3531,
		1, 3531
	},
	{
		0, 0,
		0, 0
	},
};

CFATTACH_DECL_NEW(siisata_pci, sizeof(struct siisata_pci_softc),
    siisata_pci_match, siisata_pci_attach, siisata_pci_detach, NULL);

static const struct siisata_pci_product *
siisata_pci_lookup(const struct pci_attach_args * pa)
{
	const struct siisata_pci_product *spp;

	for (spp = siisata_pci_products; spp->spp_ports > 0; spp++) {
		if (PCI_VENDOR(pa->pa_id) == spp->spp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == spp->spp_product)
			return spp;
	}
	return NULL;
}

static int
siisata_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (siisata_pci_lookup(pa) != NULL)
		return 3;

	return 0;
}

static void
siisata_pci_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct siisata_pci_softc *psc = device_private(self);
	struct siisata_softc *sc = &psc->si_sc;
	char devinfo[256];
	const char *intrstr;
	pcireg_t csr, memtype;
	const struct siisata_pci_product *spp;
	pci_intr_handle_t intrhandle;
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	uint32_t gcreg;
	int memh_valid;
	bus_size_t grsize, prsize;

	sc->sc_atac.atac_dev = self;
	
	psc->sc_pc = pa->pa_pc;
	psc->sc_pcitag = pa->pa_tag;

	pci_devinfo(pa->pa_id, pa->pa_class, 1, devinfo, sizeof(devinfo));
	aprint_naive(": SATA-II HBA\n");
	aprint_normal(": %s\n", devinfo);

	/* map bar0 */
#if 1
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, SIISATA_PCI_BAR0);
#else
	memtype = PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT;
#endif
	switch (memtype) {
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT:
		memh_valid = (pci_mapreg_map(pa, SIISATA_PCI_BAR0,
			memtype, 0, &memt, &memh, NULL, &grsize) == 0);
		break;
	default:
		memh_valid = 0;
	}
	if (memh_valid) {
		sc->sc_grt = memt;
		sc->sc_grh = memh;
		sc->sc_grs = grsize;
	} else {
		aprint_error("%s: unable to map device global registers\n",
		    SIISATANAME(sc));
		return;
	}

	/* map bar1 */
#if 1
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, SIISATA_PCI_BAR1);
#else
	memtype = PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT;
#endif
	switch (memtype) {
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT:
		memh_valid = (pci_mapreg_map(pa, SIISATA_PCI_BAR1,
			memtype, 0, &memt, &memh, NULL, &prsize) == 0);
		break;
	default:
		memh_valid = 0;
	}
	if (memh_valid) {
		sc->sc_prt = memt;
		sc->sc_prh = memh;
		sc->sc_prs = prsize;
	} else {
		bus_space_unmap(sc->sc_grt, sc->sc_grh, grsize);
		aprint_error("%s: unable to map device port registers\n",
		    SIISATANAME(sc));
		return;
	}

	if (pci_dma64_available(pa)) {
		sc->sc_dmat = pa->pa_dmat64;
		sc->sc_have_dma64 = 1;
		aprint_debug("64-bit PCI DMA available\n");
	} else {
		sc->sc_dmat = pa->pa_dmat;
		sc->sc_have_dma64 = 0;
	}

	/* map interrupt */
	if (pci_intr_map(pa, &intrhandle) != 0) {
		bus_space_unmap(sc->sc_grt, sc->sc_grh, grsize);
		bus_space_unmap(sc->sc_prt, sc->sc_prh, prsize);
		aprint_error("%s: couldn't map interrupt\n", SIISATANAME(sc));
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, intrhandle);
	psc->sc_ih = pci_intr_establish(pa->pa_pc, intrhandle,
	    IPL_BIO, siisata_intr, sc);
	if (psc->sc_ih == NULL) {
		bus_space_unmap(sc->sc_grt, sc->sc_grh, grsize);
		bus_space_unmap(sc->sc_prt, sc->sc_prh, prsize);
		aprint_error("%s: couldn't establish interrupt"
		    "at %s\n", SIISATANAME(sc), intrstr);
		return;
	}
	aprint_normal("%s: interrupting at %s\n", SIISATANAME(sc),
	    intrstr ? intrstr : "unknown interrupt");

	/* fill in number of ports on this device */
	spp = siisata_pci_lookup(pa);
	if (spp != NULL) {
		sc->sc_atac.atac_nchannels = spp->spp_ports;
		sc->sc_chip = spp->spp_chip;
	} else
		/* _match() should prevent us from getting here */
		panic("siisata: the universe might be falling apart!\n");

	/* set the necessary bits in case the firmware didn't */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_MASTER_ENABLE;
	csr |= PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, csr);

	gcreg = GRREAD(sc, GR_GC);

	aprint_normal("%s: SiI%d on ", SIISATANAME(sc), sc->sc_chip);
	if (sc->sc_chip == 3124) {
		aprint_normal("%d-bit, ", (gcreg & GR_GC_REQ64) ? 64 : 32);
		switch (gcreg & (GR_GC_DEVSEL | GR_GC_STOP | GR_GC_TRDY)) {
		case 0:
			aprint_normal("%d", (gcreg & GR_GC_M66EN) ? 66 : 33);
			break;
		case GR_GC_TRDY:
			aprint_normal("%d", 66);
			break;
		case GR_GC_STOP:
			aprint_normal("%d", 100);
			break;
		case GR_GC_STOP | GR_GC_TRDY:
			aprint_normal("%d", 133);
			break;
		default:
			break;
		}
		aprint_normal("MHz PCI%s bus.", (gcreg & (GR_GC_DEVSEL | GR_GC_STOP | GR_GC_TRDY)) ? "-X" : "");
	} else {
		/* XXX - but only x1 devices so far */
		aprint_normal("PCI-Express x1 port.");
	}
	if (gcreg & GR_GC_3GBPS)
		aprint_normal(" 3.0Gb/s capable.\n");
	else
		aprint_normal("\n");

	siisata_attach(sc);

	if (!pmf_device_register(self, NULL, siisata_pci_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
siisata_pci_detach(device_t dv, int flags)
{
	struct siisata_pci_softc *psc = device_private(dv);
	struct siisata_softc *sc = &psc->si_sc;
	int rv;

	rv = siisata_detach(sc, flags);
	if (rv)
		return rv;

	if (psc->sc_ih != NULL) {
		pci_intr_disestablish(psc->sc_pc, psc->sc_ih);
	}

	bus_space_unmap(sc->sc_prt, sc->sc_prh, sc->sc_prs);
	bus_space_unmap(sc->sc_grt, sc->sc_grh, sc->sc_grs);
	
	return 0;
}

static bool
siisata_pci_resume(device_t dv PMF_FN_ARGS)
{
	struct siisata_pci_softc *psc = device_private(dv);
	struct siisata_softc *sc = &psc->si_sc;
	int s;

	s = splbio();
	siisata_resume(sc);
	splx(s);
	
	return true;
}
