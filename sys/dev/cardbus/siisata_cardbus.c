/* $NetBSD: siisata_cardbus.c,v 1.4 2010/01/30 16:16:35 jakllsch Exp $ */
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

/*
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: siisata_cardbus.c,v 1.4 2010/01/30 16:16:35 jakllsch Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/pci/pcidevs.h>
#include <dev/ic/siisatavar.h>

#define cardbus_devinfo		pci_devinfo

struct siisata_cardbus_softc {
	struct siisata_softc si_sc;
	cardbus_chipset_tag_t sc_cc;
	cardbus_function_tag_t sc_cf;
	cardbus_devfunc_t sc_ct;

	bus_size_t sc_grsize;
	bus_size_t sc_prsize;
	void *sc_ih;
};

static int siisata_cardbus_match(device_t, cfdata_t, void *);
static void siisata_cardbus_attach(device_t, device_t, void *);
static int siisata_cardbus_detach(device_t, int);
static bool siisata_cardbus_resume(device_t, pmf_qual_t);

static const struct siisata_cardbus_product {
	cardbus_vendor_id_t scp_vendor;
	cardbus_product_id_t scp_product;
	int scp_ports;
	int scp_chip;

} siisata_cardbus_products[] = {
	{
		PCI_VENDOR_CMDTECH, PCI_PRODUCT_CMDTECH_3124,
		4, 3124
	},
	{
		0, 0,
		0, 0
	},
};

CFATTACH_DECL_NEW(siisata_cardbus, sizeof(struct siisata_cardbus_softc),
    siisata_cardbus_match, siisata_cardbus_attach, siisata_cardbus_detach,
    NULL);

static const struct siisata_cardbus_product *
siisata_cardbus_lookup(const struct cardbus_attach_args *ca)
{
	const struct siisata_cardbus_product *scp;

	for (scp = siisata_cardbus_products; scp->scp_ports > 0; scp++) {
		if (CARDBUS_VENDOR(ca->ca_id) == scp->scp_vendor &&
		    CARDBUS_PRODUCT(ca->ca_id) == scp->scp_product)
			return scp;
	}
	return NULL;
}

static int
siisata_cardbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct cardbus_attach_args *ca = aux;

	if (siisata_cardbus_lookup(ca) != NULL)
		return 3;

	return 0;
}

static void
siisata_cardbus_attach(device_t parent, device_t self, void *aux)
{
	struct cardbus_attach_args *ca = aux;
	struct cardbus_softc *cbsc = device_private(parent);
	struct siisata_cardbus_softc *csc = device_private(self);
	struct siisata_softc *sc = &csc->si_sc;
	cardbus_devfunc_t ct = ca->ca_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	cardbusreg_t csr;
	const struct siisata_cardbus_product *scp;
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	bus_addr_t base;
	bus_size_t grsize, prsize;
	uint32_t gcreg;
	char devinfo[256];

	sc->sc_atac.atac_dev = self;
	
	csc->sc_cc = cc;
	csc->sc_cf = cf;
	csc->sc_ct = ct;

	cardbus_devinfo(ca->ca_id, ca->ca_class, 0, devinfo, sizeof(devinfo));
	aprint_naive(": SATA-II HBA\n");
	aprint_normal(": %s\n", devinfo);

	/*
	 * XXXX
	 * Our BAR0/BAR1 type is 64bit Memory.  Cardbus_mapreg_map() don't
	 * support 64bit Memory.  We map ourself...
	 */
	/* map bar0 */
	{
#define SIISATA_BAR0_SIZE	128
		grsize = SIISATA_BAR0_SIZE;
		csr =
		    cardbus_conf_read(cc, cf, ca->ca_tag, SIISATA_CARDBUS_BAR0);
		base = PCI_MAPREG_MEM_ADDR(csr);
		memt = cbsc->sc_memt;
		if ((*cf->cardbus_space_alloc)(cc, cbsc->sc_rbus_memt, base,
		    grsize, grsize - 1, grsize, 0, &base, &memh)) {
			aprint_error(
			    "%s: unable to map device global registers\n",
			    SIISATANAME(sc));
			return;
		}
		cardbus_conf_write(cc, cf, ca->ca_tag, SIISATA_CARDBUS_BAR0,
		    base);
	}
	sc->sc_grt = memt;
	sc->sc_grh = memh;
	csc->sc_grsize = grsize;

	/* map bar1 */
	{
#define SIISATA_BAR1_SIZE	(32 * 1024)
		prsize = SIISATA_BAR1_SIZE;
		base = PCI_MAPREG_MEM_ADDR(cardbus_conf_read(cc, cf, ca->ca_tag,
		    SIISATA_CARDBUS_BAR1));
		memt = cbsc->sc_memt;
		if ((*cf->cardbus_space_alloc)(cc, cbsc->sc_rbus_memt, base,
		    prsize, prsize - 1, prsize, 0, &base, &memh)) {
			cardbus_conf_write(cc, cf, ca->ca_tag,
			    SIISATA_CARDBUS_BAR0, 0);
			(*cf->cardbus_space_free)(cc, cbsc->sc_rbus_memt,
			    sc->sc_grh, grsize);
			aprint_error(
			    "%s: unable to map device port registers\n",
			    SIISATANAME(sc));
			return;
		}
		cardbus_conf_write(cc, cf, ca->ca_tag, SIISATA_CARDBUS_BAR1,
		    base);
	}
	sc->sc_prt = memt;
	sc->sc_prh = memh;
	csc->sc_prsize = prsize;

	sc->sc_dmat = ca->ca_dmat;

	/* map interrupt */
	csc->sc_ih = cardbus_intr_establish(cc, cf, ca->ca_intrline, IPL_BIO,
	    siisata_intr, sc);
	if (csc->sc_ih == NULL) {
		cardbus_conf_write(cc, cf, ca->ca_tag, SIISATA_CARDBUS_BAR0, 0);
		(*cf->cardbus_space_free)(cc, cbsc->sc_rbus_memt, sc->sc_grh,
		    grsize);
		cardbus_conf_write(cc, cf, ca->ca_tag, SIISATA_CARDBUS_BAR1, 0);
		(*cf->cardbus_space_free)(cc, cbsc->sc_rbus_memt, sc->sc_prh,
		    prsize);
		aprint_error("%s: couldn't establish interrupt\n",
		    SIISATANAME(sc));
		return;
	}

	/* fill in number of ports on this device */
	scp = siisata_cardbus_lookup(ca);
	if (scp != NULL)
		sc->sc_atac.atac_nchannels = scp->scp_ports;
	else
		/* _match() should prevent us from getting here */
		panic("siisata: the universe might be falling apart!\n");

	/* enable bus mastering in case the firmware didn't */
	csr = cardbus_conf_read(cc, cf, ca->ca_tag, CARDBUS_COMMAND_STATUS_REG);
	csr |= CARDBUS_COMMAND_MASTER_ENABLE;
	csr |= CARDBUS_COMMAND_MEM_ENABLE;
	cardbus_conf_write(cc, cf, ca->ca_tag, CARDBUS_COMMAND_STATUS_REG, csr);

	gcreg = GRREAD(sc, GR_GC);

	/* CardBus supports only 32-bit 33MHz */
	KASSERT(!(gcreg &
	    (GR_GC_REQ64|GR_GC_DEVSEL|GR_GC_STOP|GR_GC_TRDY|GR_GC_M66EN)));

	aprint_normal("%s: SiI%d on 32-bit, 33MHz PCI (CardBus).",
	    SIISATANAME(sc), scp->scp_chip);
	if (gcreg & GR_GC_3GBPS)
		aprint_normal(" 3.0Gb/s capable.\n");
	else
		aprint_normal("\n");

	siisata_attach(sc);

	if (!pmf_device_register(self, NULL, siisata_cardbus_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
siisata_cardbus_detach(device_t self, int flags)
{
	struct cardbus_softc *cbsc = device_private(device_parent(self));
	struct siisata_cardbus_softc *csc = device_private(self);
	struct siisata_softc *sc = &csc->si_sc;
	struct cardbus_devfunc *ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	cardbustag_t ctag = cardbus_make_tag(cc, cf, cbsc->sc_bus, ct->ct_func);
	int rv;

	rv = siisata_detach(sc, flags);
	if (rv)
		return (rv);
	if (csc->sc_ih != NULL) {
		cardbus_intr_disestablish(csc->sc_cc, csc->sc_cf, csc->sc_ih);
		csc->sc_ih = NULL;
	}
	if (csc->sc_grsize) {
		cardbus_conf_write(cc, cf, ctag, SIISATA_CARDBUS_BAR0, 0);
		(*cf->cardbus_space_free)(cc, cbsc->sc_rbus_memt, sc->sc_grh,
		    csc->sc_grsize);
		csc->sc_grsize = 0;
	}
	if (csc->sc_prsize) {
		cardbus_conf_write(cc, cf, ctag, SIISATA_CARDBUS_BAR1, 0);
		(*cf->cardbus_space_free)(cc, cbsc->sc_rbus_memt, sc->sc_prh,
		    csc->sc_prsize);
		csc->sc_prsize = 0;
	}
	cardbus_free_tag(cc, cf, ctag);
	return 0;
}

static bool
siisata_cardbus_resume(device_t dv, pmf_qual_t qual)
{
	struct siisata_cardbus_softc *csc = device_private(dv);
	struct siisata_softc *sc = &csc->si_sc;
	int s;

	s = splbio();
	siisata_resume(sc);
	splx(s);
	
	return true;
}
