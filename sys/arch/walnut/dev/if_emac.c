/*	$NetBSD: if_emac.c,v 1.1 2001/06/13 06:01:52 simonb Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
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
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/walnut.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

struct emac_softc {
	struct device sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* ethernet common data */
	void *sc_sdhook;		/* shutdown hook */
};

static int	emac_match(struct device *, struct cfdata *, void *);
static void	emac_attach(struct device *, struct device *, void *);
static int	emac_intr(void *);

struct cfattach emac_ca = {
	sizeof(struct emac_softc), emac_match, emac_attach
};

static int probe_done = 0;

static int
emac_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (probe_done)
		return 0;

	/* XXX probe! */
	probe_done = 1;
	return 1;
}

static void
emac_attach(struct device *parent, struct device *self, void *aux)
{
	union mainbus_attach_args *maa = aux;
	struct emac_softc *sc = (struct emac_softc *)self;

	sc->sc_st = galaxy_make_bus_space_tag(0, 0);
	sc->sc_sh = maa->mba_rmb.rmb_addr;

	printf(": 405GP EMAC\n");
	printf("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(board_data.mac_address_local));

	intr_establish(maa->mba_rmb.rmb_irq, IST_LEVEL, IPL_NET, emac_intr, sc);
	printf("%s: interrupting at irq %d\n", sc->sc_dev.dv_xname, maa->mba_rmb.rmb_irq);
}

static int
emac_intr(void *arg)
{

	printf("emac_intr: arg = %p\n", arg);	/* XXX */
	return 0;
}
