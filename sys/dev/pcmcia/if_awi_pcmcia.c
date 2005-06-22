/* $NetBSD: if_awi_pcmcia.c,v 1.34 2005/06/22 06:16:02 dyoung Exp $ */

/*-
 * Copyright (c) 1999, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld
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

/*
 * PCMCIA attachment for BayStack 650 802.11FH PCMCIA card,
 * based on the AMD 79c930 802.11 controller chip.
 *
 * This attachment can probably be trivially adapted for other FH and
 * DS cards based on the same chipset.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_awi_pcmcia.c,v 1.34 2005/06/22 06:16:02 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net80211/ieee80211_netbsd.h>
#include <net80211/ieee80211_var.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/am79c930reg.h>
#include <dev/ic/am79c930var.h>
#include <dev/ic/awireg.h>
#include <dev/ic/awivar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

static int awi_pcmcia_match(struct device *, struct cfdata *, void *);
static int awi_pcmcia_validate_config(struct pcmcia_config_entry *);
static void awi_pcmcia_attach(struct device *, struct device *, void *);
static int awi_pcmcia_detach(struct device *, int);
static int awi_pcmcia_enable(struct awi_softc *);
static void awi_pcmcia_disable(struct awi_softc *);

struct awi_pcmcia_softc {
	struct awi_softc sc_awi;		/* real "awi" softc */

	/* PCMCIA-specific goo */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handler */

	int sc_state;
#define	AWI_PCMCIA_ATTACHED	3
};

CFATTACH_DECL(awi_pcmcia, sizeof(struct awi_pcmcia_softc),
    awi_pcmcia_match, awi_pcmcia_attach, awi_pcmcia_detach, awi_activate);

static const struct pcmcia_product awi_pcmcia_products[] = {
	{ PCMCIA_VENDOR_BAY,		PCMCIA_PRODUCT_BAY_STACK_650,
	  PCMCIA_CIS_BAY_STACK_650 },

	{ PCMCIA_VENDOR_BAY,		PCMCIA_PRODUCT_BAY_STACK_660,
	  PCMCIA_CIS_BAY_STACK_660 },

	{ PCMCIA_VENDOR_BAY,		PCMCIA_PRODUCT_BAY_SURFER_PRO,
	  PCMCIA_CIS_BAY_SURFER_PRO },

	{ PCMCIA_VENDOR_AMD,		PCMCIA_PRODUCT_AMD_AM79C930,
	  PCMCIA_CIS_AMD_AM79C930 },

	{ PCMCIA_VENDOR_ICOM,		PCMCIA_PRODUCT_ICOM_SL200,
	  PCMCIA_CIS_ICOM_SL200 },

	{ PCMCIA_VENDOR_NOKIA,		PCMCIA_PRODUCT_NOKIA_C020_WLAN,
	  PCMCIA_CIS_NOKIA_C020_WLAN },

	{ PCMCIA_VENDOR_FARALLON,	PCMCIA_PRODUCT_FARALLON_SKYLINE,
	  PCMCIA_CIS_FARALLON_SKYLINE },
};
static const size_t awi_pcmcia_nproducts =
    sizeof(awi_pcmcia_products) / sizeof(awi_pcmcia_products[0]);

static int
awi_pcmcia_enable(sc)
	struct awi_softc *sc;
{
	struct awi_pcmcia_softc *psc = (struct awi_pcmcia_softc *)sc;
	struct pcmcia_function *pf = psc->sc_pf;
	int error;

	/* establish the interrupt. */
	psc->sc_ih = pcmcia_intr_establish(pf, IPL_NET, awi_intr, sc);
	if (!psc->sc_ih)
		return (EIO);

	error = pcmcia_function_enable(pf);
	if (error) {
		pcmcia_intr_disestablish(pf, psc->sc_ih);
		psc->sc_ih = 0;
	}

	return (error);
}

static void
awi_pcmcia_disable(sc)
	struct awi_softc *sc;
{
	struct awi_pcmcia_softc *psc = (struct awi_pcmcia_softc *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	pcmcia_function_disable(pf);
	pcmcia_intr_disestablish(pf, psc->sc_ih);
	psc->sc_ih = 0;
}

static int
awi_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (pcmcia_product_lookup(pa, awi_pcmcia_products, awi_pcmcia_nproducts,
	    sizeof(awi_pcmcia_products[0]), NULL))
		return (1);
	return (0);
}

static int
awi_pcmcia_validate_config(cfe)
	struct pcmcia_config_entry *cfe;
{
	if (cfe->iftype != PCMCIA_IFTYPE_IO ||
	    cfe->num_iospace < 1 ||
	    cfe->iospace[0].length < AM79C930_IO_SIZE)
		return (EINVAL);
	if (cfe->num_memspace < 1) {
		cfe->memspace[0].length = AM79C930_MEM_SIZE;
		cfe->memspace[0].cardaddr = 0;
		cfe->memspace[0].hostaddr = 0;
	} else if (cfe->memspace[0].length < AM79C930_MEM_SIZE)
		return (EINVAL);
	return (0);
}

static void
awi_pcmcia_attach(parent, self, aux)
	struct device  *parent, *self;
	void           *aux;
{
	struct awi_pcmcia_softc *psc = (void *)self;
	struct awi_softc *sc = &psc->sc_awi;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	int error;

	psc->sc_pf = pa->pf;

	error = pcmcia_function_configure(pa->pf, awi_pcmcia_validate_config);
	if (error) {
		aprint_error("%s: configure failed, error=%d\n", self->dv_xname,
		    error);
		return;
	}

	cfe = pa->pf->cfe;
	sc->sc_chip.sc_bustype = AM79C930_BUS_PCMCIA;
	sc->sc_chip.sc_iot = cfe->iospace[0].handle.iot;
	sc->sc_chip.sc_ioh = cfe->iospace[0].handle.ioh;
	if (cfe->num_memspace > 0) {
		sc->sc_chip.sc_memt = cfe->memspace[0].handle.memt;
		sc->sc_chip.sc_memh = cfe->memspace[0].handle.memh;
		am79c930_chip_init(&sc->sc_chip, 1);
	} else
		am79c930_chip_init(&sc->sc_chip, 0);

	error = awi_pcmcia_enable(sc);
        if (error)
                goto fail;
	sc->sc_enabled = 1;

	awi_read_bytes(sc, AWI_BANNER, sc->sc_banner, AWI_BANNER_LEN);
	if (memcmp(sc->sc_banner, "PCnetMobile:", 12))
		goto fail2;

	sc->sc_enable = awi_pcmcia_enable;
	sc->sc_disable = awi_pcmcia_disable;

	sc->sc_cansleep = 1;

	if (awi_attach(sc) != 0) {
		printf("%s: failed to attach controller\n", self->dv_xname);
		goto fail2;
	}

	sc->sc_enabled = 0;
	awi_pcmcia_disable(sc);
	psc->sc_state = AWI_PCMCIA_ATTACHED;
	return;

fail2:
	sc->sc_enabled = 0;
	awi_pcmcia_disable(sc);
fail:
	pcmcia_function_unconfigure(pa->pf);
}

static int
awi_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct awi_pcmcia_softc *psc = (struct awi_pcmcia_softc *)self;
	int error;

	if (psc->sc_state != AWI_PCMCIA_ATTACHED)
		return (0);

	error = awi_detach(&psc->sc_awi);
	if (error)
		return (error);

	pcmcia_function_unconfigure(psc->sc_pf);

	return (0);
}
