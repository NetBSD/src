/*	$NetBSD: if_malo_cardbus.c,v 1.1 2018/12/14 21:49:22 jakllsch Exp $ */
/*	$OpenBSD: if_malo_cardbus.c,v 1.12 2013/12/06 21:03:02 deraadt Exp $ */

/*
 * Copyright (c) 2006 Claudio Jeker <claudio@openbsd.org>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_malo_cardbus.c,v 1.1 2018/12/14 21:49:22 jakllsch Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <netinet/in.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/cardbusvar.h>

#include <dev/ic/malovar.h>

struct malo_cardbus_softc {
	struct malo_softc	sc_malo;

	/* cardbus specific goo */
	cardbus_devfunc_t	sc_ct;
	pcitag_t		sc_tag;
	void			*sc_ih;
	bus_size_t		sc_mapsize1;
	bus_size_t		sc_mapsize2;
	pcireg_t		sc_bar1_val;
	pcireg_t		sc_bar2_val;
};

int	malo_cardbus_match(device_t, cfdata_t, void *);
void	malo_cardbus_attach(device_t, device_t, void *);
int	malo_cardbus_detach(device_t, int);

CFATTACH_DECL_NEW(malo_cardbus, sizeof (struct malo_cardbus_softc),
    malo_cardbus_match, malo_cardbus_attach, malo_cardbus_detach, NULL);

void	malo_cardbus_setup(struct malo_cardbus_softc *csc);
int	malo_cardbus_enable(struct malo_softc *sc);
void	malo_cardbus_disable(struct malo_softc *sc);

int
malo_cardbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct cardbus_attach_args *ca = aux;

	if (PCI_VENDOR(ca->ca_id) == PCI_VENDOR_MARVELL) {
		switch (PCI_PRODUCT(ca->ca_id)) {
			case PCI_PRODUCT_MARVELL_88W8310:
			case PCI_PRODUCT_MARVELL_88W8335_1:
			case PCI_PRODUCT_MARVELL_88W8335_2:
				return 1;
			default:
				return 0;
		}
	}

	return 0;
}

void
malo_cardbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct malo_cardbus_softc *csc = device_private(self);
	struct malo_softc *sc = &csc->sc_malo;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	char devinfo[256];
	bus_addr_t base;
	int error;

	pci_devinfo(ca->ca_id, ca->ca_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s\n", devinfo);

	sc->sc_dev = self;
	sc->sc_dmat = ca->ca_dmat;
	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;

	/* power management hooks */
	sc->sc_enable = malo_cardbus_enable;
	sc->sc_disable = malo_cardbus_disable;

	/* map control/status registers */
	error = Cardbus_mapreg_map(ct, PCI_BAR0,
	    PCI_MAPREG_TYPE_MEM, 0, &sc->sc_mem1_bt, &sc->sc_mem1_bh,
	    &base, &csc->sc_mapsize1);
	if (error != 0) {
		aprint_error(": can't map mem1 space\n");
		return;
	}
	csc->sc_bar1_val = base | PCI_MAPREG_TYPE_MEM;

	error = Cardbus_mapreg_map(ct, PCI_BAR1,
	    PCI_MAPREG_TYPE_MEM, 0, &sc->sc_mem2_bt, &sc->sc_mem2_bh,
	    &base, &csc->sc_mapsize2);
	if (error != 0) {
		aprint_error(": can't map mem2 space\n");
		Cardbus_mapreg_unmap(ct, PCI_BAR0, sc->sc_mem1_bt,
		    sc->sc_mem1_bh, csc->sc_mapsize1);
		return;
	}
	csc->sc_bar2_val = base | PCI_MAPREG_TYPE_MEM;

	/* set up the PCI configuration registers */
	malo_cardbus_setup(csc);

	error = malo_attach(sc);
#if notyet
	if (error != 0)
		malo_cardbus_detach(sc->sc_dev, 0);
#endif

	Cardbus_function_disable(ct);
}

int
malo_cardbus_detach(struct device *self, int flags)
{
	struct malo_cardbus_softc *csc = device_private(self);
	struct malo_softc *sc = &csc->sc_malo;
	cardbus_devfunc_t ct = csc->sc_ct;
	int error;

	error = malo_detach(sc);
	if (error != 0)
		return error;

	/* unhook the interrupt handler */
	if (csc->sc_ih != NULL) {
		Cardbus_intr_disestablish(ct, csc->sc_ih);
		csc->sc_ih = NULL;
	}

	/* release bus space and close window */
	Cardbus_mapreg_unmap(ct, PCI_BAR0, sc->sc_mem1_bt,
	    sc->sc_mem1_bh, csc->sc_mapsize1);
	Cardbus_mapreg_unmap(ct, PCI_BAR1, sc->sc_mem2_bt,
	    sc->sc_mem2_bh, csc->sc_mapsize2);

	return 0;
}

void
malo_cardbus_setup(struct malo_cardbus_softc *csc)
{
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	pcireg_t reg;

	/* program the BAR */
	Cardbus_conf_write(ct, csc->sc_tag, PCI_BAR0, csc->sc_bar1_val);
	Cardbus_conf_write(ct, csc->sc_tag, PCI_BAR1, csc->sc_bar2_val);

	/* make sure the right access type is on the cardbus bridge */
	(*cf->cardbus_ctrl)(cc, CARDBUS_MEM_ENABLE);
	(*cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* enable the appropriate bits in the PCI CSR */
	reg = Cardbus_conf_read(ct, csc->sc_tag, PCI_COMMAND_STATUS_REG);
	reg |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE;
	Cardbus_conf_write(ct, csc->sc_tag, PCI_COMMAND_STATUS_REG, reg);
}

int
malo_cardbus_enable(struct malo_softc *sc)
{
	struct malo_cardbus_softc *csc = (struct malo_cardbus_softc *)sc;
	cardbus_devfunc_t ct = csc->sc_ct;

	/* power on the socket */
	Cardbus_function_enable(ct);

	/* setup the PCI configuration registers */
	malo_cardbus_setup(csc);

	/* map and establish the interrupt handler */
	csc->sc_ih = Cardbus_intr_establish(ct, IPL_NET,
	    malo_intr, sc);
	if (csc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "could not establish interrupt\n");
		Cardbus_function_disable(ct);
		return 1;
	}

	return 0;
}

void
malo_cardbus_disable(struct malo_softc *sc)
{
	struct malo_cardbus_softc *csc = (struct malo_cardbus_softc *)sc;
	cardbus_devfunc_t ct = csc->sc_ct;

	/* unhook the interrupt handler */
	Cardbus_intr_disestablish(ct, csc->sc_ih);
	csc->sc_ih = NULL;

	/* power down the socket */
	Cardbus_function_disable(ct);
}
