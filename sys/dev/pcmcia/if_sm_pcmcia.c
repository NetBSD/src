/*	$NetBSD: if_sm_pcmcia.c,v 1.26.8.1 2002/06/20 16:33:55 gehenna Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sm_pcmcia.c,v 1.26.8.1 2002/06/20 16:33:55 gehenna Exp $");

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

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/smc91cxxreg.h>
#include <dev/ic/smc91cxxvar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

int	sm_pcmcia_match __P((struct device *, struct cfdata *, void *));
void	sm_pcmcia_attach __P((struct device *, struct device *, void *));
int	sm_pcmcia_detach __P((struct device *, int));

struct sm_pcmcia_softc {
	struct	smc91cxx_softc sc_smc;		/* real "smc" softc */

	/* PCMCIA-specific goo. */
	struct	pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int	sc_io_window;			/* our i/o window */
	void	*sc_ih;				/* interrupt cookie */
	struct	pcmcia_function *sc_pf;		/* our PCMCIA function */
};

struct cfattach sm_pcmcia_ca = {
	sizeof(struct sm_pcmcia_softc), sm_pcmcia_match, sm_pcmcia_attach,
	    sm_pcmcia_detach, smc91cxx_activate
};

int	sm_pcmcia_enable __P((struct smc91cxx_softc *));
void	sm_pcmcia_disable __P((struct smc91cxx_softc *));

int	sm_pcmcia_ascii_enaddr __P((const char *, u_int8_t *));
int	sm_pcmcia_funce_enaddr __P((struct device *, u_int8_t *));

int	sm_pcmcia_lannid_ciscallback __P((struct pcmcia_tuple *, void *));

const struct pcmcia_product sm_pcmcia_products[] = {
	{ PCMCIA_STR_MEGAHERTZ2_XJACK,		PCMCIA_VENDOR_MEGAHERTZ2,
	  PCMCIA_PRODUCT_MEGAHERTZ2_XJACK,	 0, },

	{ PCMCIA_STR_NEWMEDIA_BASICS,		PCMCIA_VENDOR_NEWMEDIA,
	  PCMCIA_PRODUCT_NEWMEDIA_BASICS,	0, },

#if 0
	{ PCMCIA_STR_SMC_8020BT,		PCMCIA_VENDOR_SMC,
	  PCMCIA_PRODUCT_SMC_8020BT,		0, },
#endif
	{ PCMCIA_STR_PSION_GOLDCARD,		PCMCIA_VENDOR_PSION,
	  PCMCIA_PRODUCT_PSION_GOLDCARD,	0, },

	{ NULL }
};

int
sm_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (pcmcia_product_lookup(pa, sm_pcmcia_products,
	    sizeof sm_pcmcia_products[0], NULL) != NULL)
		return (1);
	return (0);
}

void
sm_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sm_pcmcia_softc *psc = (struct sm_pcmcia_softc *)self;
	struct smc91cxx_softc *sc = &psc->sc_smc;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	u_int8_t myla[ETHER_ADDR_LEN], *enaddr = NULL;
	const struct pcmcia_product *pp;

	psc->sc_pf = pa->pf;
	cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head);

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (pcmcia_function_enable(pa->pf)) {
		printf(": function enable failed\n");
		goto enable_failed;
	}

	/* XXX sanity check number of mem and i/o spaces */

	/* Allocate and map i/o space for the card. */
	if (pcmcia_io_alloc(pa->pf, 0, cfe->iospace[0].length,
	    cfe->iospace[0].length, &psc->sc_pcioh)) {
		printf(": can't allocate i/o space\n");
		goto ioalloc_failed;
	}

	sc->sc_bst = psc->sc_pcioh.iot;
	sc->sc_bsh = psc->sc_pcioh.ioh;

	sc->sc_enable = sm_pcmcia_enable;
	sc->sc_disable = sm_pcmcia_disable;

	if (pcmcia_io_map(pa->pf, (cfe->flags & PCMCIA_CFE_IO16) ?
	    PCMCIA_WIDTH_IO16 : PCMCIA_WIDTH_IO8, 0, cfe->iospace[0].length,
	    &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space\n");
		goto iomap_failed;
	}

	pp = pcmcia_product_lookup(pa, sm_pcmcia_products,
	    sizeof sm_pcmcia_products[0], NULL);
	if (pp == NULL)
		panic("sm_pcmcia_attach: impossible");

	printf(": %s\n", pp->pp_name);

	/*
	 * First try to get the Ethernet address from FUNCE/LANNID tuple.
	 */
	if (sm_pcmcia_funce_enaddr(parent, myla))
		enaddr = myla;

	/*
	 * If that failed, try one of the CIS info strings.
	 */
	if (enaddr == NULL) {
		char *cisstr = NULL;

		switch (pa->manufacturer) {
		case PCMCIA_VENDOR_MEGAHERTZ:
		case PCMCIA_VENDOR_MEGAHERTZ2:
			cisstr = pa->pf->sc->card.cis1_info[3];
			break;

		case PCMCIA_VENDOR_SMC:
			cisstr = pa->pf->sc->card.cis1_info[2];
			break;
		}

		if (cisstr != NULL && sm_pcmcia_ascii_enaddr(cisstr, myla))
			enaddr = myla;
	}

	if (enaddr == NULL)
		printf("%s: unable to get Ethernet address\n",
		    sc->sc_dev.dv_xname);

	/* Perform generic intialization. */
	smc91cxx_attach(sc, enaddr);

	pcmcia_function_disable(pa->pf);
	return;

 iomap_failed:
	/* Free our i/o space. */
	pcmcia_io_free(pa->pf, &psc->sc_pcioh);

 ioalloc_failed:
	/* Disable the device */
	pcmcia_function_disable(pa->pf);

 enable_failed:
	psc->sc_io_window = -1;
}

int
sm_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct sm_pcmcia_softc *psc = (struct sm_pcmcia_softc *)self;
	int rv;

	if (psc->sc_io_window == -1)
		/* Nothing to detach. */
		return (0);

	rv = smc91cxx_detach((struct device *)&psc->sc_smc, flags);
	if (rv != 0)
		return (rv);

	/* Unmap our i/o window. */
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);

	/* Free our i/o space. */
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
	return (0);
}

int
sm_pcmcia_ascii_enaddr(cisstr, myla)
	const char *cisstr;
	u_int8_t *myla;
{
	u_int8_t digit;
	int i;

	memset(myla, 0, ETHER_ADDR_LEN);

	for (i = 0, digit = 0; i < (ETHER_ADDR_LEN * 2); i++) {
		if (cisstr[i] >= '0' && cisstr[i] <= '9')
			digit |= cisstr[i] - '0';
		else if (cisstr[i] >= 'a' && cisstr[i] <= 'f')
			digit |= (cisstr[i] - 'a') + 10;
		else if (cisstr[i] >= 'A' && cisstr[i] <= 'F')
			digit |= (cisstr[i] - 'A') + 10;
		else {
			/* Bogus digit!! */
			return (0);
		}

		/* Compensate for ordering of digits. */
		if (i & 1) {
			myla[i >> 1] = digit;
			digit = 0;
		} else
			digit <<= 4;
	}

	return (1);
}

int
sm_pcmcia_funce_enaddr(parent, myla)
	struct device *parent;
	u_int8_t *myla;
{

	return (pcmcia_scan_cis(parent, sm_pcmcia_lannid_ciscallback, myla));
}

int
sm_pcmcia_lannid_ciscallback(tuple, arg)
	struct pcmcia_tuple *tuple;
	void *arg;
{
	u_int8_t *myla = arg;
	int i;

	if (tuple->code == PCMCIA_CISTPL_FUNCE
	    || tuple->code == PCMCIA_CISTPL_SPCL) {
		/* subcode, length */
		if (tuple->length < 2)
			return (0);

		if ((pcmcia_tuple_read_1(tuple, 0) !=
		     PCMCIA_TPLFE_TYPE_LAN_NID) ||
		    (pcmcia_tuple_read_1(tuple, 1) != ETHER_ADDR_LEN))
			return (0);

		for (i = 0; i < ETHER_ADDR_LEN; i++)
			myla[i] = pcmcia_tuple_read_1(tuple, i + 2);
		return (1);
	}
	return (0);
}

int
sm_pcmcia_enable(sc)
	struct smc91cxx_softc *sc;
{
	struct sm_pcmcia_softc *psc = (struct sm_pcmcia_softc *)sc;
	int rv;

	rv = pcmcia_function_enable(psc->sc_pf);
	if (rv != 0)
		return (rv);

	/* Establish the interrupt handler. */
	psc->sc_ih = pcmcia_intr_establish(psc->sc_pf, IPL_NET, smc91cxx_intr,
	    sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}
	return (0);
}

void
sm_pcmcia_disable(sc)
	struct smc91cxx_softc *sc;
{
	struct sm_pcmcia_softc *psc = (struct sm_pcmcia_softc *)sc;

	pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);
	pcmcia_function_disable(psc->sc_pf);
}
