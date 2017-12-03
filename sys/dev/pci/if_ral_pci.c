/*	$NetBSD: if_ral_pci.c,v 1.20.2.2 2017/12/03 11:37:08 jdolecek Exp $	*/
/*	$OpenBSD: if_ral_pci.c,v 1.24 2015/11/24 17:11:39 mpi Exp $  */

/*-
 * Copyright (c) 2005-2010 Damien Bergamini <damien.bergamini@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * PCI front-end for the Ralink RT2560/RT2561/RT2860/RT3090 driver.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ral_pci.c,v 1.20.2.2 2017/12/03 11:37:08 jdolecek Exp $");


#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <netinet/in.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_rssadapt.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/ic/rt2560var.h>
#include <dev/ic/rt2661var.h>
#include <dev/ic/rt2860var.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

static struct ral_opns {
	int	(*attach)(void *, int);
	int	(*detach)(void *);
	int	(*intr)(void *);

}  ral_rt2560_opns = {
	rt2560_attach,
	rt2560_detach,
	rt2560_intr

}, ral_rt2661_opns = {
	rt2661_attach,
	rt2661_detach,
	rt2661_intr

}, ral_rt2860_opns = {
	rt2860_attach,
	rt2860_detach,
	rt2860_intr
};

struct ral_pci_softc {
	union {
		struct rt2560_softc	sc_rt2560;
		struct rt2661_softc	sc_rt2661;
		struct rt2860_softc	sc_rt2860;
	} u;
#define sc_sc	u.sc_rt2560

	/* PCI specific goo */
	struct ral_opns		*sc_opns;
	pci_chipset_tag_t	sc_pc;
	void			*sc_ih;
	bus_size_t		sc_mapsize;
};

/* Base Address Register */
#define RAL_PCI_BAR0 PCI_BAR(0)

int	ral_pci_match(device_t, cfdata_t, void *);
void	ral_pci_attach(device_t, device_t, void *);
int	ral_pci_detach(device_t, int);

CFATTACH_DECL_NEW(ral_pci, sizeof (struct ral_pci_softc),
	ral_pci_match, ral_pci_attach, ral_pci_detach, NULL);

static const struct ral_pci_matchid {
	pci_vendor_id_t         ral_vendor;
	pci_product_id_t        ral_product;
} ral_pci_devices[] = {
	{ PCI_VENDOR_AWT,    PCI_PRODUCT_AWT_RT2890 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT2860_1 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT2860_2 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT2860_3 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT2860_4 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT2860_5 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT2860_6 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT2860_7 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT3591_1 },
	{ PCI_VENDOR_EDIMAX, PCI_PRODUCT_EDIMAX_RT3591_2 },
	{ PCI_VENDOR_MSI,    PCI_PRODUCT_AWT_RT2890 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2560 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2561 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2561S },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2661 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2760 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2790 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2860 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2890 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3060 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3062 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3090 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3091 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3092 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3562 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3592 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3593 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT5360 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT5362 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT5390_1 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT5390_2 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT5390_3 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT5390_4 },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT5390_5 },
};

int
ral_pci_match(device_t parent, cfdata_t cfdata,
    void *aux)
{
	struct pci_attach_args *pa = aux;

	for (size_t i = 0; i < __arraycount(ral_pci_devices); i++) {
		const struct ral_pci_matchid *ral = &ral_pci_devices[i];
		if (PCI_VENDOR(pa->pa_id) == ral->ral_vendor &&
		    PCI_PRODUCT(pa->pa_id) == ral->ral_product)
			return 1;
	}

	return 0;
}

void
ral_pci_attach(device_t parent, device_t self, void *aux)
{
	struct ral_pci_softc *psc = device_private(self);
	struct rt2560_softc *sc = &psc->sc_sc;
	const struct pci_attach_args *pa = aux;
	const char *intrstr;
	bus_addr_t base;
	pci_intr_handle_t ih;
	pcireg_t memtype, reg;
	int error;
	char intrbuf[PCI_INTRSTR_LEN];

	pci_aprint_devinfo(pa, NULL);

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_RALINK) {
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_RALINK_RT2560:
			psc->sc_opns = &ral_rt2560_opns;
			break;
		case PCI_PRODUCT_RALINK_RT2561:
		case PCI_PRODUCT_RALINK_RT2561S:
		case PCI_PRODUCT_RALINK_RT2661:
			psc->sc_opns = &ral_rt2661_opns;
			break;
		default:
			psc->sc_opns = &ral_rt2860_opns;
			break;
		}
	} else {
		/* all other vendors are RT2860 only */
		psc->sc_opns = &ral_rt2860_opns;
	}

	sc->sc_dev = self;
	sc->sc_dmat = pa->pa_dmat;
	psc->sc_pc = pa->pa_pc;

	/* enable the appropriate bits in the PCI CSR */
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	reg |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, reg);

	/* map control/status registers */
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, RAL_PCI_BAR0);
	error = pci_mapreg_map(pa, RAL_PCI_BAR0, memtype, 0, &sc->sc_st,
	    &sc->sc_sh, &base, &psc->sc_mapsize);

	if (error != 0) {
		aprint_error(": could not map memory space\n");
		return;
	}

	if (pci_intr_map(pa, &ih) != 0) {
		aprint_error(": could not map interrupt\n");
		return;
	}

	intrstr = pci_intr_string(psc->sc_pc, ih, intrbuf, sizeof(intrbuf));
	psc->sc_ih = pci_intr_establish(psc->sc_pc, ih, IPL_NET,
	    psc->sc_opns->intr, sc);

	if (psc->sc_ih == NULL) {
		aprint_error(": could not establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting at %s\n", intrstr);

	(*psc->sc_opns->attach)(sc, PCI_PRODUCT(pa->pa_id));
}

int
ral_pci_detach(device_t self, int flags)
{
	struct ral_pci_softc *psc = device_private(self);
	struct rt2560_softc *sc = &psc->sc_sc;
	int error;

	if (psc->sc_ih != NULL) {
		pci_intr_disestablish(psc->sc_pc, psc->sc_ih);

		error = (*psc->sc_opns->detach)(sc);
		if (error != 0)
			return error;
	}

	if (psc->sc_mapsize > 0)
		bus_space_unmap(sc->sc_st, sc->sc_sh, psc->sc_mapsize);

	return 0;
}
