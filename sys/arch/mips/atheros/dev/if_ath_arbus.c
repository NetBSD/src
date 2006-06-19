/* $NetBSD: if_ath_arbus.c,v 1.3.8.1 2006/06/19 03:44:52 chap Exp $ */

/*-
 * Copyright (c) 2006 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: if_ath_arbus.c,v 1.3.8.1 2006/06/19 03:44:52 chap Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_llc.h>
#include <net/if_arp.h>

#include <netinet/in.h>

#include <net80211/ieee80211_netbsd.h>
#include <net80211/ieee80211_var.h>

#include <mips/atheros/include/ar531xreg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>

#include <dev/pci/pcidevs.h>
#include <dev/ic/ath_netbsd.h>
#include <dev/ic/athvar.h>
#include <contrib/dev/ath/ah.h>

struct ath_arbus_softc {
	struct ath_softc	sc_ath;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void			*sc_ih;
	struct ar531x_config	sc_config;
};

static int	ath_arbus_match(struct device *, struct cfdata *, void *);
static void	ath_arbus_attach(struct device *, struct device *, void *);
static int	ath_arbus_detach(struct device *, int);
static void	ath_arbus_shutdown(void *);

CFATTACH_DECL(ath_arbus, sizeof(struct ath_arbus_softc),
    ath_arbus_match, ath_arbus_attach, ath_arbus_detach, NULL);

static int
ath_arbus_match(struct device *parent, struct cfdata *cf, void *opaque)
{
	struct arbus_attach_args *aa;

	aa = (struct arbus_attach_args *)opaque;
	if (strcmp(aa->aa_name, "ath") == 0)
		return 1;

	return 0;
}

static void
ath_arbus_attach(struct device *parent, struct device *self, void *opaque)
{
	struct ath_arbus_softc *asc;
	struct ath_softc *sc;
	struct arbus_attach_args *aa;
	const char *name;
	void *hook;
	int rv;
	uint16_t devid;
	uint32_t rev;

	asc = (struct ath_arbus_softc *)self;
	sc = &asc->sc_ath;
	aa = (struct arbus_attach_args *)opaque;

	rev = GETSYSREG(AR531X_SYSREG_REVISION);
	devid = AR531X_REVISION_WMAC(rev);
	name = ath_hal_probe(PCI_VENDOR_ATHEROS, devid);

	printf(": %s\n", name ? name : "Unknown AR531X WLAN");

	asc->sc_iot = aa->aa_bst;
	rv = bus_space_map(asc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &asc->sc_ioh);
	if (rv) {
		aprint_error("%s: unable to map registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	/*
	 * Setup HAL configuration state for use by the driver.
	 */
	rv = ar531x_board_config(&asc->sc_config);
	if (rv) {
		aprint_error("%s: unable to locate board configuration\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	asc->sc_config.unit = sc->sc_dev.dv_unit;	/* XXX? */
	asc->sc_config.tag = asc->sc_iot;

	/* NB: the HAL expects the config state passed as the tag */
	sc->sc_st = (HAL_BUS_TAG) &asc->sc_config;
	sc->sc_sh = (HAL_BUS_HANDLE) asc->sc_ioh;
	sc->sc_dmat = aa->aa_dmat;

	sc->sc_invalid = 1;

	asc->sc_ih = arbus_intr_establish(aa->aa_irq, ath_intr, sc);
	if (asc->sc_ih == NULL) {
		aprint_error("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (ath_attach(devid, sc) != 0) {
		aprint_error("%s: ath_attach failed\n", sc->sc_dev.dv_xname);
		goto err;
	}

	hook = shutdownhook_establish(ath_arbus_shutdown, asc);
	if (hook == NULL) {
		aprint_error("%s: couldn't establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
		goto err;
	}

	return;

err:
	arbus_intr_disestablish(asc->sc_ih);
}

static int
ath_arbus_detach(struct device *self, int flags)
{
	struct ath_arbus_softc *asc = (struct ath_arbus_softc *)self;

	ath_detach(&asc->sc_ath);
	arbus_intr_disestablish(asc->sc_ih);

	return (0);
}

static void
ath_arbus_shutdown(void *opaque)
{
	struct ath_arbus_softc *asc = opaque;

	ath_shutdown(&asc->sc_ath);

	return;
}
