/*	$NetBSD: if_sm_pcmcia.c,v 1.1.2.3 1997/09/29 21:17:38 thorpej Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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

#include "bpfilter.h"

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

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/smc91cxxreg.h>
#include <dev/ic/smc91cxxvar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>

#define	PCMCIA_MANUFACTURER_MEGAHERTZ	0x128
#define	PCMCIA_PRODUCT_MEGAHERTZ_XJACK	0x103

#ifdef __BROKEN_INDIRECT_CONFIG
int	sm_pcmcia_match __P((struct device *, void *, void *));
#else
int	sm_pcmcia_match __P((struct device *, struct cfdata *, void *));
#endif
void	sm_pcmcia_attach __P((struct device *, struct device *, void *));

struct sm_pcmcia_softc {
	struct	smc91cxx_softc sc_smc;		/* real "smc" softc */

	/* PCMCIA-specific goo. */
	struct	pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int	sc_io_window;			/* our i/o window */
	void	*sc_ih;				/* interrupt cookie */
	struct	pcmcia_function *sc_pf;		/* our PCMCIA function */
};

struct cfattach sm_pcmcia_ca = {
	sizeof(struct sm_pcmcia_softc), sm_pcmcia_match, sm_pcmcia_attach
};

int	sm_pcmcia_enable __P((struct smc91cxx_softc *));
void	sm_pcmcia_disable __P((struct smc91cxx_softc *));

int
sm_pcmcia_match(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (pa->manufacturer == PCMCIA_MANUFACTURER_MEGAHERTZ) {
		switch (pa->product) {
		case PCMCIA_PRODUCT_MEGAHERTZ_XJACK:
			if (pa->pf->number == 0)
				return (1);
		}
	}

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
	const char *model;

	psc->sc_pf = pa->pf;
	cfe = pa->pf->cfe_head.sqh_first;

	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (pcmcia_function_enable(pa->pf)) {
		printf(": function enable failed\n");
		return;
	}

	/* XXX sanity check number of mem and i/o spaces */

	/* Allocate and map i/o space for the card. */
	if (pcmcia_io_alloc(pa->pf, 0, cfe->iospace[0].length,
	    &psc->sc_pcioh)) {
		printf(": can't allocate i/o space\n");
		return;
	}

	sc->sc_bst = psc->sc_pcioh.iot;
	sc->sc_bsh = psc->sc_pcioh.ioh;

	sc->sc_enable = sm_pcmcia_enable;
	sc->sc_disable = sm_pcmcia_disable;

	if (pcmcia_io_map(pa->pf, (cfe->flags & PCMCIA_CFE_IO16) ?
	    PCMCIA_WIDTH_IO16 : PCMCIA_WIDTH_IO8, 0, cfe->iospace[0].length,
	    &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space\n");
		return;
	}

	switch (pa->product) {
	case PCMCIA_PRODUCT_MEGAHERTZ_XJACK:
		model = "Megahertz X-JACK Ethernet";
		break;

	default:
		model = "Unknown SMC91Cxx Ethernet";
	}
	printf(": %s\n", model);

	if (pa->product == PCMCIA_PRODUCT_MEGAHERTZ_XJACK) {
		char enaddr_str[12];
		int i = 0, j;

		/*
		 * The X-JACK's Ethernet address is stored in the fourth
		 * CIS info string.  We need to parse it and pass it to
		 * the generic layer.
		 */
		if (strlen(pa->pf->sc->card.cis1_info[3]) != 12) {
			/* Bogus address! */
			goto out;
		}
		bcopy(pa->pf->sc->card.cis1_info[3], enaddr_str, 12);
		bzero(myla, sizeof(myla));
		for (i = 0; i < 6; i++) {
			for (j = 0; j < 2; j++) {
				/* Convert to upper case. */
				if (enaddr_str[(i * 2) + j] >= 'a' &&
				    enaddr_str[(i * 2) + j] <= 'z')
					enaddr_str[(i * 2) + j] -= 'a' - 'A';

				/* Parse the digit. */
				if (enaddr_str[(i * 2) + j] >= '0' &&
				    enaddr_str[(i * 2) + j] <= '9')
					myla[i] |= enaddr_str[(i * 2) + j]
					    - '0';
				else if (enaddr_str[(i * 2) + j] >= 'A' &&
					 enaddr_str[(i * 2) + j] <= 'F')
					myla[i] |= enaddr_str[(i * 2) + j]
					    - 'A' + 10;
				else {
					/* Bogus digit!! */
					goto out;
				}

				/* Compensate for ordering of digits. */
				if (j == 0)
					myla[i] <<= 4;
			}
		}

 out:
		if (i >= 6) {
			/* Successfully parsed. */
			enaddr = myla;
		} else {
			printf("%s: unable to read MAC address from CIS\n",
			    sc->sc_dev.dv_xname);
		}
	}

	/* Perform generic intialization. */
	smc91cxx_attach(sc, enaddr);

	/* Establish the interrupt handler. */
	psc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_NET, smc91cxx_intr, sc);
	if (psc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);

	pcmcia_function_disable(pa->pf);
}

int
sm_pcmcia_enable(sc)
	struct smc91cxx_softc *sc;
{
	struct sm_pcmcia_softc *psc = (struct sm_pcmcia_softc *)sc;

	return (pcmcia_function_enable(psc->sc_pf));
}

void
sm_pcmcia_disable(sc)
	struct smc91cxx_softc *sc;
{
	struct sm_pcmcia_softc *psc = (struct sm_pcmcia_softc *)sc;

	pcmcia_function_disable(psc->sc_pf);
}
