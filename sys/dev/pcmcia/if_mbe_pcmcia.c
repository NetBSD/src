/*	$NetBSD: if_mbe_pcmcia.c,v 1.9 1999/10/27 19:10:02 is Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Enami Tsugutomo.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/mb86960reg.h>
#include <dev/ic/mb86960var.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

int	mbe_pcmcia_match __P((struct device *, struct cfdata *, void *));
void	mbe_pcmcia_attach __P((struct device *, struct device *, void *));
int	mbe_pcmcia_detach __P((struct device *, int));

struct mbe_pcmcia_softc {
	struct	mb86960_softc sc_mb86960;	/* real "mb" softc */

	/* PCMCIA-specific goo. */
	struct	pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int	sc_io_window;			/* our i/o window */
	void	*sc_ih;				/* interrupt cookie */
	struct	pcmcia_function *sc_pf;		/* our PCMCIA function */
};

struct cfattach mbe_pcmcia_ca = {
	sizeof(struct mbe_pcmcia_softc), mbe_pcmcia_match, mbe_pcmcia_attach,
	    mbe_pcmcia_detach, mb86960_activate
};

#if NetBSD <= 199712
struct cfdriver mbe_cd = {			/* XXX shouldn't be here */
	NULL, "mbe", DV_IFNET
};
#endif

int	mbe_pcmcia_enable __P((struct mb86960_softc *));
void	mbe_pcmcia_disable __P((struct mb86960_softc *));

struct mbe_pcmcia_get_enaddr_args {
	int got_enaddr;
	u_int8_t enaddr[ETHER_ADDR_LEN];
};
int	mbe_pcmcia_get_enaddr __P((struct pcmcia_tuple *, void *));

struct mbe_pcmcia_product {
	u_int32_t	mpp_vendor;	/* vendor ID */
	u_int32_t	mpp_product;	/* product ID */
	int		mpp_expfunc;	/* exptected function */
	u_int32_t	mpp_ioalign;	/* required alignment */
	const char	*mpp_name;	/* product name */
} mbe_pcmcia_products[] = {
	{ PCMCIA_VENDOR_TDK,		PCMCIA_PRODUCT_TDK_LAK_CD021BX,
	  0,				0,
	  PCMCIA_STR_TDK_LAK_CD021BX },
#if 0 /* XXX 86960-based? */
	{ PCMCIA_VENDOR_TDK,		PCMCIA_PRODUCT_TDK_LAK_DFL9610,
	  1,				0,
	  PCMCIA_STR_TDK_LAK_DFL9610 }
#endif

	{ PCMCIA_VENDOR_CONTEC,		PCMCIA_PRODUCT_CONTEC_CNETPC,
	  0,				0,
	  PCMCIA_STR_CONTEC_CNETPC },

	{ PCMCIA_VENDOR_FUJITSU,	PCMCIA_PRODUCT_FUJITSU_LA501,
	  0,				0x20,
	  PCMCIA_STR_FUJITSU_LA501 },

	{ PCMCIA_VENDOR_FUJITSU,	PCMCIA_PRODUCT_FUJITSU_LA10S,
	  0,				0,
	  PCMCIA_STR_FUJITSU_LA10S },

	{ 0,				0,
	  0,				0,
	  NULL },
};

const struct mbe_pcmcia_product *mbe_pcmcia_lookup
    __P((const struct pcmcia_attach_args *pa));

const struct mbe_pcmcia_product *
mbe_pcmcia_lookup(pa)
	const struct pcmcia_attach_args *pa;
{
	const struct mbe_pcmcia_product *mpp;

	for (mpp = mbe_pcmcia_products; mpp->mpp_name != NULL; mpp++) {
		if (pa->manufacturer == mpp->mpp_vendor &&
		    pa->product == mpp->mpp_product &&
		    pa->pf->number == mpp->mpp_expfunc)
			return (mpp);
	}
	return (NULL);
}

int
mbe_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (mbe_pcmcia_lookup(pa) != NULL)
		return (1);

	return (0);
}

void
mbe_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mbe_pcmcia_softc *psc = (struct mbe_pcmcia_softc *)self;
	struct mb86960_softc *sc = &psc->sc_mb86960;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct mbe_pcmcia_get_enaddr_args pgea;
	const struct mbe_pcmcia_product *mpp;

	mpp = mbe_pcmcia_lookup(pa);
	if (mpp == NULL) {
		printf("\n");
		panic("mbe_pcmcia_attach: impossible");
	}

	psc->sc_pf = pa->pf;
	cfe = pa->pf->cfe_head.sqh_first;

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (pcmcia_function_enable(pa->pf)) {
		printf(": function enable failed\n");
		return;
	}

	/* Allocate and map i/o space for the card. */
	if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start,
	    cfe->iospace[0].length,
	    mpp->mpp_ioalign ? mpp->mpp_ioalign : cfe->iospace[0].length,
	    &psc->sc_pcioh)) {
		printf(": can't allocate i/o space\n");
		return;
	}

	sc->sc_bst = psc->sc_pcioh.iot;
	sc->sc_bsh = psc->sc_pcioh.ioh;

	sc->sc_enable = mbe_pcmcia_enable;
	sc->sc_disable = mbe_pcmcia_disable;

	/*
	 * Don't bother checking flags; the back-end sets the chip
	 * into 16-bit mode.
	 */
	if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_IO16, 0, cfe->iospace[0].length,
	    &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space\n");
		return;
	}

	printf(": %s\n", mpp->mpp_name);

	/* Read station address. */
	pgea.got_enaddr = 0;
	if (pcmcia_scan_cis(parent, mbe_pcmcia_get_enaddr, &pgea) == -1) {
		printf("%s: Couldn't read CIS to get ethernet address\n",
		    sc->sc_dev.dv_xname);
		return;
	} else if (!pgea.got_enaddr) {
		printf("%s: Couldn't get ethernet address from CIS\n",
		    sc->sc_dev.dv_xname);
		return;
	} else
#ifdef DIAGNOSTIC
		printf("%s: Ethernet address from CIS: %s\n",
		    sc->sc_dev.dv_xname, ether_sprintf(pgea.enaddr))
#endif
		;

	/* Perform generic initialization. */
	mb86960_attach(sc, MB86960_TYPE_86965, pgea.enaddr);

	mb86960_config(sc, NULL, 0, 0);

	pcmcia_function_disable(pa->pf);
}

int
mbe_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct mbe_pcmcia_softc *psc = (struct mbe_pcmcia_softc *)self;

	/* Unmap our i/o window. */
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);

	/* Free our i/o space. */
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);

#ifdef notyet
	/*
	 * Our softc is about to go away, so drop our reference
	 * to the ifnet.
	 */
	if_delref(psc->sc_mb86960.sc_ec.ec_if);
	return (0);
#else
	return (EBUSY);
#endif
}

int
mbe_pcmcia_enable(sc)
	struct mb86960_softc *sc;
{
	struct mbe_pcmcia_softc *psc = (struct mbe_pcmcia_softc *)sc;

	/* Establish the interrupt handler. */
	psc->sc_ih = pcmcia_intr_establish(psc->sc_pf, IPL_NET, mb86960_intr,
	    sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	return (pcmcia_function_enable(psc->sc_pf));
}

void
mbe_pcmcia_disable(sc)
	struct mb86960_softc *sc;
{
	struct mbe_pcmcia_softc *psc = (struct mbe_pcmcia_softc *)sc;

	pcmcia_function_disable(psc->sc_pf);

	pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);
}

int
mbe_pcmcia_get_enaddr(tuple, arg)
	struct pcmcia_tuple *tuple;
	void *arg;
{
	struct mbe_pcmcia_get_enaddr_args *p = arg;
	int i;

	if (tuple->code == PCMCIA_CISTPL_FUNCE) {
		if (tuple->length < 2) /* sub code and ether addr length */
			return (0);

		if ((pcmcia_tuple_read_1(tuple, 0) !=
			PCMCIA_TPLFE_TYPE_LAN_NID) ||
		    (pcmcia_tuple_read_1(tuple, 1) != ETHER_ADDR_LEN))
			return (0);

		for (i = 0; i < ETHER_ADDR_LEN; i++)
			p->enaddr[i] = pcmcia_tuple_read_1(tuple, i + 2);
		p->got_enaddr = 1;
		return (1);
	}
	return (0);
}
