/* $Id: if_ae.c,v 1.4 2006/09/24 03:53:08 jmcneill Exp $ */
/*-
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1998, 1999, 2000, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center; and by Charles M. Hannum.
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

/*
 * Device driver for the onboard ethernet MAC found on the AR5312
 * chip's AHB bus.
 *
 * This device is very simliar to the tulip in most regards, and
 * the code is directly derived from NetBSD's tulip.c.  However, it
 * is different enough that it did not seem to be a good idea to
 * add further complexity to the tulip driver, so we have our own.
 *
 * Also tulip has a lot of complexity in it for various parts/options
 * that we don't need, and on these little boxes with only ~8MB RAM, we
 * don't want any extra bloat.
 */

/*
 * TODO:
 *
 * 1) Find out about BUS_MODE_ALIGN16B.  This chip can apparently align
 *    inbound packets on a half-word boundary, which would make life easier
 *    for TCP/IP.  (Aligning IP headers on a word.)
 *
 * 2) There is stuff in original tulip to shut down the device when reacting
 *    to a a change in link status.  Is that needed.
 *
 * 3) Test with variety of 10/100 HDX/FDX scenarios.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ae.c,v 1.4 2006/09/24 03:53:08 jmcneill Exp $");

#include "bpfilter.h"

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

#include <machine/endian.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>

#include <mips/atheros/include/arbusvar.h>
#include <mips/atheros/dev/aereg.h>
#include <mips/atheros/dev/aevar.h>

static const struct {
	u_int32_t txth_opmode;		/* OPMODE bits */
	const char *txth_name;		/* name of mode */
} ae_txthresh[] = {
	{ OPMODE_TR_32,		"32 words" },
	{ OPMODE_TR_64,		"64 words" },
	{ OPMODE_TR_128,	"128 words" },
	{ OPMODE_TR_256,	"256 words" },
	{ OPMODE_SF,		"store and forward mode" },
	{ 0,			NULL },
};

static int 	ae_match(struct device *, struct cfdata *, void *);
static void	ae_attach(struct device *, struct device *, void *);
static int	ae_detach(struct device *, int);
static int	ae_activate(struct device *, enum devact);

static void	ae_reset(struct ae_softc *);
static void	ae_idle(struct ae_softc *, u_int32_t);

static int	ae_mediachange(struct ifnet *);
static void	ae_mediastatus(struct ifnet *, struct ifmediareq *);

static void	ae_start(struct ifnet *);
static void	ae_watchdog(struct ifnet *);
static int	ae_ioctl(struct ifnet *, u_long, caddr_t);
static int	ae_init(struct ifnet *);
static void	ae_stop(struct ifnet *, int);

static void	ae_shutdown(void *);

static void	ae_rxdrain(struct ae_softc *);
static int	ae_add_rxbuf(struct ae_softc *, int);

static int	ae_enable(struct ae_softc *);
static void	ae_disable(struct ae_softc *);
static void	ae_power(int, void *);

static void	ae_filter_setup(struct ae_softc *);

static int	ae_intr(void *);
static void	ae_rxintr(struct ae_softc *);
static void	ae_txintr(struct ae_softc *);

static void	ae_mii_tick(void *);
static void	ae_mii_statchg(struct device *);

static int	ae_mii_readreg(struct device *, int, int);
static void	ae_mii_writereg(struct device *, int, int, int);

#ifdef AE_DEBUG
#define	DPRINTF(sc, x)	if ((sc)->sc_ethercom.ec_if.if_flags & IFF_DEBUG) \
				printf x
#else
#define	DPRINTF(sc, x)	/* nothing */
#endif

#ifdef AE_STATS
static void	ae_print_stats(struct ae_softc *);
#endif

CFATTACH_DECL(ae, sizeof(struct ae_softc),
    ae_match, ae_attach, ae_detach, ae_activate);

/*
 * ae_match:
 *
 *	Check for a device match.
 */
int
ae_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct arbus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, cf->cf_name) == 0)
		return 1;

	return 0;

}

/*
 * ae_attach:
 *
 *	Attach an ae interface to the system.
 */
void
ae_attach(struct device *parent, struct device *self, void *aux)
{
	const uint8_t *enaddr;
	prop_data_t ea;
	struct ae_softc *sc = (void *)self;
	struct arbus_attach_args *aa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int i, error;

	callout_init(&sc->sc_tick_callout);

	printf(": Atheros AR531X 10/100 Ethernet\n");

	/*
	 * Try to get MAC address.
	 */
	ea = prop_dictionary_get(device_properties(&sc->sc_dev), "mac-addr");
	if (ea == NULL) {
		printf("%s: unable to get mac-addr property\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
	KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
	enaddr = prop_data_data_nocopy(ea);

	/* Announce ourselves. */
	printf("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr));

	sc->sc_cirq = aa->aa_cirq;
	sc->sc_mirq = aa->aa_mirq;
	sc->sc_st = aa->aa_bst;
	sc->sc_dmat = aa->aa_dmat;

	SIMPLEQ_INIT(&sc->sc_txfreeq);
	SIMPLEQ_INIT(&sc->sc_txdirtyq);

	/*
	 * Map registers.
	 */
	sc->sc_size = aa->aa_size;
	if ((error = bus_space_map(sc->sc_st, aa->aa_addr, sc->sc_size, 0,
	    &sc->sc_sh)) != 0) {
		printf("%s: unable to map registers, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct ae_control_data), PAGE_SIZE, 0, &sc->sc_cdseg,
	    1, &sc->sc_cdnseg, 0)) != 0) {
		printf("%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &sc->sc_cdseg, sc->sc_cdnseg,
	    sizeof(struct ae_control_data), (caddr_t *)&sc->sc_control_data,
	    BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct ae_control_data), 1,
	    sizeof(struct ae_control_data), 0, 0, &sc->sc_cddmamap)) != 0) {
		printf("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_3;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct ae_control_data), NULL,
	    0)) != 0) {
		printf("%s: unable to load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_4;
	}

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < AE_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    AE_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			printf("%s: unable to create tx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_5;
		}
	}

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < AE_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			printf("%s: unable to create rx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_6;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	/*
	 * Reset the chip to a known state.
	 */
	ae_reset(sc);

	/*
	 * From this point forward, the attachment cannot fail.  A failure
	 * before this point releases all resources that may have been
	 * allocated.
	 */
	sc->sc_flags |= AE_ATTACHED;

	/*
	 * Initialize our media structures.  This may probe the MII, if
	 * present.
	 */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = ae_mii_readreg;
	sc->sc_mii.mii_writereg = ae_mii_writereg;
	sc->sc_mii.mii_statchg = ae_mii_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, ae_mediachange,
	    ae_mediastatus);
	mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);

	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

	sc->sc_tick = ae_mii_tick;

	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	sc->sc_if_flags = ifp->if_flags;
	ifp->if_ioctl = ae_ioctl;
	ifp->if_start = ae_start;
	ifp->if_watchdog = ae_watchdog;
	ifp->if_init = ae_init;
	ifp->if_stop = ae_stop;
	IFQ_SET_READY(&ifp->if_snd);

	/*
	 * We can support 802.1Q VLAN-sized frames.
	 */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

#if NRND > 0
	rnd_attach_source(&sc->sc_rnd_source, sc->sc_dev.dv_xname,
	    RND_TYPE_NET, 0);
#endif

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	sc->sc_sdhook = shutdownhook_establish(ae_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);

	/*
	 * Add a suspend hook to make sure we come back up after a
	 * resume.
	 */
	sc->sc_powerhook = powerhook_establish(sc->sc_dev.dv_xname,
	    ae_power, sc);
	if (sc->sc_powerhook == NULL)
		printf("%s: WARNING: unable to establish power hook\n",
		    sc->sc_dev.dv_xname);
	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
 fail_6:
	for (i = 0; i < AE_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
 fail_5:
	for (i = 0; i < AE_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
 fail_4:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
 fail_3:
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_control_data,
	    sizeof(struct ae_control_data));
 fail_2:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_cdseg, sc->sc_cdnseg);
 fail_1:
	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_size);
 fail_0:
	return;
}

/*
 * ae_activate:
 *
 *	Handle device activation/deactivation requests.
 */
int
ae_activate(struct device *self, enum devact act)
{
	struct ae_softc *sc = (void *) self;
	int s, error = 0;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		mii_activate(&sc->sc_mii, act, MII_PHY_ANY, MII_OFFSET_ANY);
		if_deactivate(&sc->sc_ethercom.ec_if);
		break;
	}
	splx(s);

	return (error);
}

/*
 * ae_detach:
 *
 *	Detach a device interface.
 */
int
ae_detach(struct device *self, int flags)
{
	struct ae_softc *sc = (void *)self;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ae_rxsoft *rxs;
	struct ae_txsoft *txs;
	int i;

	/*
	 * Succeed now if there isn't any work to do.
	 */
	if ((sc->sc_flags & AE_ATTACHED) == 0)
		return (0);

	/* Unhook our tick handler. */
	if (sc->sc_tick)
		callout_stop(&sc->sc_tick_callout);

	/* Detach all PHYs */
	mii_detach(&sc->sc_mii, MII_PHY_ANY, MII_OFFSET_ANY);

	/* Delete all remaining media. */
	ifmedia_delete_instance(&sc->sc_mii.mii_media, IFM_INST_ANY);

#if NRND > 0
	rnd_detach_source(&sc->sc_rnd_source);
#endif
	ether_ifdetach(ifp);
	if_detach(ifp);

	for (i = 0; i < AE_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
		bus_dmamap_destroy(sc->sc_dmat, rxs->rxs_dmamap);
	}
	for (i = 0; i < AE_TXQUEUELEN; i++) {
		txs = &sc->sc_txsoft[i];
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
		bus_dmamap_destroy(sc->sc_dmat, txs->txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_control_data,
	    sizeof(struct ae_control_data));
	bus_dmamem_free(sc->sc_dmat, &sc->sc_cdseg, sc->sc_cdnseg);

	shutdownhook_disestablish(sc->sc_sdhook);
	powerhook_disestablish(sc->sc_powerhook);

	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_size);


	return (0);
}

/*
 * ae_shutdown:
 *
 *	Make sure the interface is stopped at reboot time.
 */
static void
ae_shutdown(void *arg)
{
	struct ae_softc *sc = arg;

	ae_stop(&sc->sc_ethercom.ec_if, 1);
}

/*
 * ae_start:		[ifnet interface function]
 *
 *	Start packet transmission on the interface.
 */
static void
ae_start(struct ifnet *ifp)
{
	struct ae_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct ae_txsoft *txs, *last_txs = NULL;
	bus_dmamap_t dmamap;
	int error, firsttx, nexttx, lasttx = 1, ofree, seg;

	DPRINTF(sc, ("%s: ae_start: sc_flags 0x%08x, if_flags 0x%08x\n",
	    sc->sc_dev.dv_xname, sc->sc_flags, ifp->if_flags));


	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous number of free descriptors and
	 * the first descriptor we'll use.
	 */
	ofree = sc->sc_txfree;
	firsttx = sc->sc_txnext;

	DPRINTF(sc, ("%s: ae_start: txfree %d, txnext %d\n",
	    sc->sc_dev.dv_xname, ofree, firsttx));

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txfreeq)) != NULL &&
	       sc->sc_txfree != 0) {
		/*
		 * Grab a packet off the queue.
		 */
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
		m = NULL;

		dmamap = txs->txs_dmamap;

		/*
		 * Load the DMA map.  If this fails, the packet either
		 * didn't fit in the alloted number of segments, or we were
		 * short on resources.  In this case, we'll copy and try
		 * again.
		 */
		if (((mtod(m0, uintptr_t) & 3) != 0) ||
		    bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		      BUS_DMA_WRITE|BUS_DMA_NOWAIT) != 0) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				printf("%s: unable to allocate Tx mbuf\n",
				    sc->sc_dev.dv_xname);
				break;
			}
			MCLAIM(m, &sc->sc_ethercom.ec_tx_mowner);
			if (m0->m_pkthdr.len > MHLEN) {
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0) {
					printf("%s: unable to allocate Tx "
					    "cluster\n", sc->sc_dev.dv_xname);
					m_freem(m);
					break;
				}
			}
			m_copydata(m0, 0, m0->m_pkthdr.len, mtod(m, caddr_t));
			m->m_pkthdr.len = m->m_len = m0->m_pkthdr.len;
			error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap,
			    m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
			if (error) {
				printf("%s: unable to load Tx buffer, "
				    "error = %d\n", sc->sc_dev.dv_xname,
				    error);
				break;
			}
		}

		/*
		 * Ensure we have enough descriptors free to describe
		 * the packet.
		 */
		if (dmamap->dm_nsegs > sc->sc_txfree) {
			/*
			 * Not enough free descriptors to transmit this
			 * packet.  We haven't committed to anything yet,
			 * so just unload the DMA map, put the packet
			 * back on the queue, and punt.  Notify the upper
			 * layer that there are no more slots left.
			 *
			 * XXX We could allocate an mbuf and copy, but
			 * XXX it is worth it?
			 */
			ifp->if_flags |= IFF_OACTIVE;
			bus_dmamap_unload(sc->sc_dmat, dmamap);
			if (m != NULL)
				m_freem(m);
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m != NULL) {
			m_freem(m0);
			m0 = m;
		}

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		/* Sync the DMA map. */
		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Initialize the transmit descriptors.
		 */
		for (nexttx = sc->sc_txnext, seg = 0;
		     seg < dmamap->dm_nsegs;
		     seg++, nexttx = AE_NEXTTX(nexttx)) {
			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the OWN bit just
			 * yet.  That could cause a race condition.
			 * We'll do it below.
			 */
			sc->sc_txdescs[nexttx].ad_status =
			    (nexttx == firsttx) ? 0 : ADSTAT_OWN;
			sc->sc_txdescs[nexttx].ad_bufaddr1 =
			    dmamap->dm_segs[seg].ds_addr;
			sc->sc_txdescs[nexttx].ad_ctl =
			    (dmamap->dm_segs[seg].ds_len <<
				ADCTL_SIZE1_SHIFT) |
				(nexttx == (AE_NTXDESC - 1) ?
				    ADCTL_ER : 0);
			lasttx = nexttx;
		}

		KASSERT(lasttx != -1);

		/* Set `first segment' and `last segment' appropriately. */
		sc->sc_txdescs[sc->sc_txnext].ad_ctl |= ADCTL_Tx_FS;
		sc->sc_txdescs[lasttx].ad_ctl |= ADCTL_Tx_LS;

#ifdef AE_DEBUG
		if (ifp->if_flags & IFF_DEBUG) {
			printf("     txsoft %p transmit chain:\n", txs);
			for (seg = sc->sc_txnext;; seg = AE_NEXTTX(seg)) {
				printf("     descriptor %d:\n", seg);
				printf("       ad_status:   0x%08x\n",
				    sc->sc_txdescs[seg].ad_status);
				printf("       ad_ctl:      0x%08x\n",
				    sc->sc_txdescs[seg].ad_ctl);
				printf("       ad_bufaddr1: 0x%08x\n",
				    sc->sc_txdescs[seg].ad_bufaddr1);
				printf("       ad_bufaddr2: 0x%08x\n",
				    sc->sc_txdescs[seg].ad_bufaddr2);
				if (seg == lasttx)
					break;
			}
		}
#endif

		/* Sync the descriptors we're using. */
		AE_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * Store a pointer to the packet so we can free it later,
		 * and remember what txdirty will be once the packet is
		 * done.
		 */
		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_lastdesc = lasttx;
		txs->txs_ndescs = dmamap->dm_nsegs;

		/* Advance the tx pointer. */
		sc->sc_txfree -= dmamap->dm_nsegs;
		sc->sc_txnext = nexttx;

		SIMPLEQ_REMOVE_HEAD(&sc->sc_txfreeq, txs_q);
		SIMPLEQ_INSERT_TAIL(&sc->sc_txdirtyq, txs, txs_q);

		last_txs = txs;

#if NBPFILTER > 0
		/*
		 * Pass the packet to any BPF listeners.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif /* NBPFILTER > 0 */
	}

	if (txs == NULL || sc->sc_txfree == 0) {
		/* No more slots left; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}

	if (sc->sc_txfree != ofree) {
		DPRINTF(sc, ("%s: packets enqueued, IC on %d, OWN on %d\n",
		    sc->sc_dev.dv_xname, lasttx, firsttx));
		/*
		 * Cause a transmit interrupt to happen on the
		 * last packet we enqueued.
		 */
		sc->sc_txdescs[lasttx].ad_ctl |= ADCTL_Tx_IC;
		AE_CDTXSYNC(sc, lasttx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * The entire packet chain is set up.  Give the
		 * first descriptor to the chip now.
		 */
		sc->sc_txdescs[firsttx].ad_status |= ADSTAT_OWN;
		AE_CDTXSYNC(sc, firsttx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* Wake up the transmitter. */
		/* XXX USE AUTOPOLLING? */
		AE_WRITE(sc, CSR_TXPOLL, TXPOLL_TPD);
		AE_BARRIER(sc);

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

/*
 * ae_watchdog:	[ifnet interface function]
 *
 *	Watchdog timer handler.
 */
static void
ae_watchdog(struct ifnet *ifp)
{
	struct ae_softc *sc = ifp->if_softc;
	int doing_transmit;

	doing_transmit = (! SIMPLEQ_EMPTY(&sc->sc_txdirtyq));

	if (doing_transmit) {
		printf("%s: transmit timeout\n", sc->sc_dev.dv_xname);
		ifp->if_oerrors++;
	}
	else
		printf("%s: spurious watchdog timeout\n", sc->sc_dev.dv_xname);

	(void) ae_init(ifp);

	/* Try to get more packets going. */
	ae_start(ifp);
}

/*
 * ae_ioctl:		[ifnet interface function]
 *
 *	Handle control requests from the operator.
 */
static int
ae_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ae_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;
	case SIOCSIFFLAGS:
		/* If the interface is up and running, only modify the receive
		 * filter when setting promiscuous or debug mode.  Otherwise
		 * fall through to ether_ioctl, which will reset the chip.
		 */
#define RESETIGN (IFF_CANTCHANGE|IFF_DEBUG)
		if (((ifp->if_flags & (IFF_UP|IFF_RUNNING))
		    == (IFF_UP|IFF_RUNNING))
		    && ((ifp->if_flags & (~RESETIGN))
		    == (sc->sc_if_flags & (~RESETIGN)))) {
			/* Set up the receive filter. */
			ae_filter_setup(sc);
			error = 0;
			break;
#undef RESETIGN
		}
		/* FALLTHROUGH */
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING) {
				/*
				 * Multicast list has changed.  Set the
				 * hardware filter accordingly.
				 */
				ae_filter_setup(sc);
			}
			error = 0;
		}
		break;
	}

	/* Try to get more packets going. */
	if (AE_IS_ENABLED(sc))
		ae_start(ifp);

	sc->sc_if_flags = ifp->if_flags;
	splx(s);
	return (error);
}

/*
 * ae_intr:
 *
 *	Interrupt service routine.
 */
int
ae_intr(void *arg)
{
	struct ae_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int32_t status, rxstatus, txstatus;
	int handled = 0, txthresh;

	DPRINTF(sc, ("%s: ae_intr\n", sc->sc_dev.dv_xname));

#ifdef DEBUG
	if (AE_IS_ENABLED(sc) == 0)
		panic("%s: ae_intr: not enabled", sc->sc_dev.dv_xname);
#endif

	/*
	 * If the interface isn't running, the interrupt couldn't
	 * possibly have come from us.
	 */
	if ((ifp->if_flags & IFF_RUNNING) == 0 ||
	    !device_is_active(&sc->sc_dev)) {
		printf("spurious?!?\n");
		return (0);
	}

	for (;;) {
		status = AE_READ(sc, CSR_STATUS);
		if (status) {
			AE_WRITE(sc, CSR_STATUS, status);
			AE_BARRIER(sc);
		}

		if ((status & sc->sc_inten) == 0)
			break;

		handled = 1;

		rxstatus = status & sc->sc_rxint_mask;
		txstatus = status & sc->sc_txint_mask;

		if (rxstatus) {
			/* Grab new any new packets. */
			ae_rxintr(sc);

			if (rxstatus & STATUS_RU) {
				printf("%s: receive ring overrun\n",
				    sc->sc_dev.dv_xname);
				/* Get the receive process going again. */
				AE_WRITE(sc, CSR_RXPOLL, RXPOLL_RPD);
				AE_BARRIER(sc);
				break;
			}
		}

		if (txstatus) {
			/* Sweep up transmit descriptors. */
			ae_txintr(sc);

			if (txstatus & STATUS_TJT)
				printf("%s: transmit jabber timeout\n",
				    sc->sc_dev.dv_xname);

			if (txstatus & STATUS_UNF) {
				/*
				 * Increase our transmit threshold if
				 * another is available.
				 */
				txthresh = sc->sc_txthresh + 1;
				if (ae_txthresh[txthresh].txth_name != NULL) {
					uint32_t opmode;
					/* Idle the transmit process. */
					opmode = AE_READ(sc, CSR_OPMODE);
					ae_idle(sc, OPMODE_ST);

					sc->sc_txthresh = txthresh;
					opmode &=
					    ~(OPMODE_TR|OPMODE_SF);
					opmode |=
					    ae_txthresh[txthresh].txth_opmode;
					printf("%s: transmit underrun; new "
					    "threshold: %s\n",
					    sc->sc_dev.dv_xname,
					    ae_txthresh[txthresh].txth_name);

					/*
					 * Set the new threshold and restart
					 * the transmit process.
					 */
					AE_WRITE(sc, CSR_OPMODE, opmode);
					AE_BARRIER(sc);
				}
					/*
					 * XXX Log every Nth underrun from
					 * XXX now on?
					 */
			}
		}

		if (status & (STATUS_TPS|STATUS_RPS)) {
			if (status & STATUS_TPS)
				printf("%s: transmit process stopped\n",
				    sc->sc_dev.dv_xname);
			if (status & STATUS_RPS)
				printf("%s: receive process stopped\n",
				    sc->sc_dev.dv_xname);
			(void) ae_init(ifp);
			break;
		}

		if (status & STATUS_SE) {
			const char *str;

			if (status & STATUS_TX_ABORT)
				str = "tx abort";
			else if (status & STATUS_RX_ABORT)
				str = "rx abort";
			else
				str = "unknown error";

			printf("%s: fatal system error: %s\n",
			    sc->sc_dev.dv_xname, str);
			(void) ae_init(ifp);
			break;
		}

		/*
		 * Not handled:
		 *
		 *	Transmit buffer unavailable -- normal
		 *	condition, nothing to do, really.
		 *
		 *	General purpose timer experied -- we don't
		 *	use the general purpose timer.
		 *
		 *	Early receive interrupt -- not available on
		 *	all chips, we just use RI.  We also only
		 *	use single-segment receive DMA, so this
		 *	is mostly useless.
		 */
	}

	/* Try to get more packets going. */
	ae_start(ifp);

#if NRND > 0
	if (handled)
		rnd_add_uint32(&sc->sc_rnd_source, status);
#endif
	return (handled);
}

/*
 * ae_rxintr:
 *
 *	Helper; handle receive interrupts.
 */
static void
ae_rxintr(struct ae_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_header *eh;
	struct ae_rxsoft *rxs;
	struct mbuf *m;
	u_int32_t rxstat;
	int i, len;

	for (i = sc->sc_rxptr;; i = AE_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		AE_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		rxstat = sc->sc_rxdescs[i].ad_status;

		if (rxstat & ADSTAT_OWN) {
			/*
			 * We have processed all of the receive buffers.
			 */
			break;
		}

		/*
		 * If any collisions were seen on the wire, count one.
		 */
		if (rxstat & ADSTAT_Rx_CS)
			ifp->if_collisions++;

		/*
		 * If an error occurred, update stats, clear the status
		 * word, and leave the packet buffer in place.  It will
		 * simply be reused the next time the ring comes around.
	 	 * If 802.1Q VLAN MTU is enabled, ignore the Frame Too Long
		 * error.
		 */
		if (rxstat & ADSTAT_ES &&
		    ((sc->sc_ethercom.ec_capenable & ETHERCAP_VLAN_MTU) == 0 ||
		     (rxstat & (ADSTAT_Rx_DE | ADSTAT_Rx_RF |
				ADSTAT_Rx_DB | ADSTAT_Rx_CE)) != 0)) {
#define	PRINTERR(bit, str)						\
			if (rxstat & (bit))				\
				printf("%s: receive error: %s\n",	\
				    sc->sc_dev.dv_xname, str)
			ifp->if_ierrors++;
			PRINTERR(ADSTAT_Rx_DE, "descriptor error");
			PRINTERR(ADSTAT_Rx_RF, "runt frame");
			PRINTERR(ADSTAT_Rx_TL, "frame too long");
			PRINTERR(ADSTAT_Rx_RE, "MII error");
			PRINTERR(ADSTAT_Rx_DB, "dribbling bit");
			PRINTERR(ADSTAT_Rx_CE, "CRC error");
#undef PRINTERR
			AE_INIT_RXDESC(sc, i);
			continue;
		}

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		/*
		 * No errors; receive the packet.  Note the chip
		 * includes the CRC with every packet.
		 */
		len = ADSTAT_Rx_LENGTH(rxstat) - ETHER_CRC_LEN;

		/*
		 * XXX: the Atheros part can align on half words.  what
		 * is the performance implication of this?  Probably
		 * minimal, and we should use it...
		 */
#ifdef __NO_STRICT_ALIGNMENT
		/*
		 * Allocate a new mbuf cluster.  If that fails, we are
		 * out of memory, and must drop the packet and recycle
		 * the buffer that's already attached to this descriptor.
		 */
		m = rxs->rxs_mbuf;
		if (ae_add_rxbuf(sc, i) != 0) {
			ifp->if_ierrors++;
			AE_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
			continue;
		}
#else
		/*
		 * The chip's receive buffers must be 4-byte aligned.
		 * But this means that the data after the Ethernet header
		 * is misaligned.  We must allocate a new buffer and
		 * copy the data, shifted forward 2 bytes.
		 */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
 dropit:
			ifp->if_ierrors++;
			AE_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
			continue;
		}
		MCLAIM(m, &sc->sc_ethercom.ec_rx_mowner);
		if (len > (MHLEN - 2)) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(m);
				goto dropit;
			}
		}
		m->m_data += 2;

		/*
		 * Note that we use clusters for incoming frames, so the
		 * buffer is virtually contiguous.
		 */
		memcpy(mtod(m, caddr_t), mtod(rxs->rxs_mbuf, caddr_t), len);

		/* Allow the receive descriptor to continue using its mbuf. */
		AE_INIT_RXDESC(sc, i);
		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
#endif /* __NO_STRICT_ALIGNMENT */

		ifp->if_ipackets++;
		eh = mtod(m, struct ether_header *);
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

#if NBPFILTER > 0
		/*
		 * Pass this up to any BPF listeners, but only
		 * pass it up the stack if its for us.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif /* NPBFILTER > 0 */

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* Update the receive pointer. */
	sc->sc_rxptr = i;
}

/*
 * ae_txintr:
 *
 *	Helper; handle transmit interrupts.
 */
static void
ae_txintr(struct ae_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ae_txsoft *txs;
	u_int32_t txstat;

	DPRINTF(sc, ("%s: ae_txintr: sc_flags 0x%08x\n",
	    sc->sc_dev.dv_xname, sc->sc_flags));

	ifp->if_flags &= ~IFF_OACTIVE;

	/*
	 * Go through our Tx list and free mbufs for those
	 * frames that have been transmitted.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		AE_CDTXSYNC(sc, txs->txs_lastdesc,
		    txs->txs_ndescs,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

#ifdef AE_DEBUG
		if (ifp->if_flags & IFF_DEBUG) {
			int i;
			printf("    txsoft %p transmit chain:\n", txs);
			for (i = txs->txs_firstdesc;; i = AE_NEXTTX(i)) {
				printf("     descriptor %d:\n", i);
				printf("       ad_status:   0x%08x\n",
				    sc->sc_txdescs[i].ad_status);
				printf("       ad_ctl:      0x%08x\n",
				    sc->sc_txdescs[i].ad_ctl);
				printf("       ad_bufaddr1: 0x%08x\n",
				    sc->sc_txdescs[i].ad_bufaddr1);
				printf("       ad_bufaddr2: 0x%08x\n",
				    sc->sc_txdescs[i].ad_bufaddr2);
				if (i == txs->txs_lastdesc)
					break;
			}
		}
#endif

		txstat = sc->sc_txdescs[txs->txs_lastdesc].ad_status;
		if (txstat & ADSTAT_OWN)
			break;

		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs_q);

		sc->sc_txfree += txs->txs_ndescs;

		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;

		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);

		/*
		 * Check for errors and collisions.
		 */
#ifdef AE_STATS
		if (txstat & ADSTAT_Tx_UF)
			sc->sc_stats.ts_tx_uf++;
		if (txstat & ADSTAT_Tx_TO)
			sc->sc_stats.ts_tx_to++;
		if (txstat & ADSTAT_Tx_EC)
			sc->sc_stats.ts_tx_ec++;
		if (txstat & ADSTAT_Tx_LC)
			sc->sc_stats.ts_tx_lc++;
#endif

		if (txstat & (ADSTAT_Tx_UF|ADSTAT_Tx_TO))
			ifp->if_oerrors++;

		if (txstat & ADSTAT_Tx_EC)
			ifp->if_collisions += 16;
		else
			ifp->if_collisions += ADSTAT_Tx_COLLISIONS(txstat);
		if (txstat & ADSTAT_Tx_LC)
			ifp->if_collisions++;

		ifp->if_opackets++;
	}

	/*
	 * If there are no more pending transmissions, cancel the watchdog
	 * timer.
	 */
	if (txs == NULL)
		ifp->if_timer = 0;
}

#ifdef AE_STATS
void
ae_print_stats(struct ae_softc *sc)
{

	printf("%s: tx_uf %lu, tx_to %lu, tx_ec %lu, tx_lc %lu\n",
	    sc->sc_dev.dv_xname,
	    sc->sc_stats.ts_tx_uf, sc->sc_stats.ts_tx_to,
	    sc->sc_stats.ts_tx_ec, sc->sc_stats.ts_tx_lc);
}
#endif

/*
 * ae_reset:
 *
 *	Perform a soft reset on the chip.
 */
void
ae_reset(struct ae_softc *sc)
{
	int i;

	AE_WRITE(sc, CSR_BUSMODE, BUSMODE_SWR);
	AE_BARRIER(sc);

	/*
	 * The chip doesn't take itself out of reset automatically.
	 * We need to do so after 2us.
	 */
	delay(10);
	AE_WRITE(sc, CSR_BUSMODE, 0);
	AE_BARRIER(sc);

	for (i = 0; i < 1000; i++) {
		/*
		 * Wait a bit for the reset to complete before peeking
		 * at the chip again.
		 */
		delay(10);
		if (AE_ISSET(sc, CSR_BUSMODE, BUSMODE_SWR) == 0)
			break;
	}

	if (AE_ISSET(sc, CSR_BUSMODE, BUSMODE_SWR))
		printf("%s: reset failed to complete\n", sc->sc_dev.dv_xname);

	delay(1000);
}

/*
 * ae_init:		[ ifnet interface function ]
 *
 *	Initialize the interface.  Must be called at splnet().
 */
static int
ae_init(struct ifnet *ifp)
{
	struct ae_softc *sc = ifp->if_softc;
	struct ae_txsoft *txs;
	struct ae_rxsoft *rxs;
	uint8_t *enaddr;
	int i, error = 0;

	if ((error = ae_enable(sc)) != 0)
		goto out;

	/*
	 * Cancel any pending I/O.
	 */
	ae_stop(ifp, 0);

	/*
	 * Reset the chip to a known state.
	 */
	ae_reset(sc);

	/*
	 * Initialize the BUSMODE register.
	 */
	AE_WRITE(sc, CSR_BUSMODE,
	    /* XXX: not sure if this is a good thing or not... */
	    //BUSMODE_ALIGN_16B |
	    BUSMODE_BAR | BUSMODE_BLE | BUSMODE_PBL_4LW);
	AE_BARRIER(sc);

	/*
	 * Initialize the transmit descriptor ring.
	 */
	memset(sc->sc_txdescs, 0, sizeof(sc->sc_txdescs));
	for (i = 0; i < AE_NTXDESC; i++) {
		sc->sc_txdescs[i].ad_ctl = 0;
		sc->sc_txdescs[i].ad_bufaddr2 =
		    AE_CDTXADDR(sc, AE_NEXTTX(i));
	}
	sc->sc_txdescs[AE_NTXDESC - 1].ad_ctl |= ADCTL_ER;
	AE_CDTXSYNC(sc, 0, AE_NTXDESC,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	sc->sc_txfree = AE_NTXDESC;
	sc->sc_txnext = 0;

	/*
	 * Initialize the transmit job descriptors.
	 */
	SIMPLEQ_INIT(&sc->sc_txfreeq);
	SIMPLEQ_INIT(&sc->sc_txdirtyq);
	for (i = 0; i < AE_TXQUEUELEN; i++) {
		txs = &sc->sc_txsoft[i];
		txs->txs_mbuf = NULL;
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	/*
	 * Initialize the receive descriptor and receive job
	 * descriptor rings.
	 */
	for (i = 0; i < AE_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf == NULL) {
			if ((error = ae_add_rxbuf(sc, i)) != 0) {
				printf("%s: unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    sc->sc_dev.dv_xname, i, error);
				/*
				 * XXX Should attempt to run with fewer receive
				 * XXX buffers instead of just failing.
				 */
				ae_rxdrain(sc);
				goto out;
			}
		} else
			AE_INIT_RXDESC(sc, i);
	}
	sc->sc_rxptr = 0;

	/*
	 * Initialize the interrupt mask and enable interrupts.
	 */
	/* normal interrupts */
	sc->sc_inten =  STATUS_TI | STATUS_TU | STATUS_RI | STATUS_NIS;

	/* abnormal interrupts */
	sc->sc_inten |= STATUS_TPS | STATUS_TJT | STATUS_UNF |
	    STATUS_RU | STATUS_RPS | STATUS_SE | STATUS_AIS;

	sc->sc_rxint_mask = STATUS_RI|STATUS_RU;
	sc->sc_txint_mask = STATUS_TI|STATUS_UNF|STATUS_TJT;

	sc->sc_rxint_mask &= sc->sc_inten;
	sc->sc_txint_mask &= sc->sc_inten;

	AE_WRITE(sc, CSR_INTEN, sc->sc_inten);
	AE_WRITE(sc, CSR_STATUS, 0xffffffff);

	/*
	 * Give the transmit and receive rings to the chip.
	 */
	AE_WRITE(sc, CSR_TXLIST, AE_CDTXADDR(sc, sc->sc_txnext));
	AE_WRITE(sc, CSR_RXLIST, AE_CDRXADDR(sc, sc->sc_rxptr));
	AE_BARRIER(sc);

	/*
	 * Set the station address.
	 */
	enaddr = LLADDR(ifp->if_sadl);
	AE_WRITE(sc, CSR_MACHI, enaddr[5] << 16 | enaddr[4]);
	AE_WRITE(sc, CSR_MACLO, enaddr[3] << 24 | enaddr[2] << 16 |
		enaddr[1] << 8 | enaddr[0]);
	AE_BARRIER(sc);

	/*
	 * Set the receive filter.  This will start the transmit and
	 * receive processes.
	 */
	ae_filter_setup(sc);

	/*
	 * Set the current media.
	 */
	ae_mediachange(ifp);

	/*
	 * Start the mac.
	 */
	AE_SET(sc, CSR_MACCTL, MACCTL_RE | MACCTL_TE);
	AE_BARRIER(sc);

	/*
	 * Write out the opmode.
	 */
	AE_WRITE(sc, CSR_OPMODE, OPMODE_SR | OPMODE_ST |
	    ae_txthresh[sc->sc_txthresh].txth_opmode);
	/*
	 * Start the receive process.
	 */
	AE_WRITE(sc, CSR_RXPOLL, RXPOLL_RPD);
	AE_BARRIER(sc);

	if (sc->sc_tick != NULL) {
		/* Start the one second clock. */
		callout_reset(&sc->sc_tick_callout, hz >> 3, sc->sc_tick, sc);
	}

	/*
	 * Note that the interface is now running.
	 */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	sc->sc_if_flags = ifp->if_flags;

 out:
	if (error) {
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		ifp->if_timer = 0;
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
	}
	return (error);
}

/*
 * ae_enable:
 *
 *	Enable the chip.
 */
static int
ae_enable(struct ae_softc *sc)
{

	if (AE_IS_ENABLED(sc) == 0) {
		sc->sc_ih = arbus_intr_establish(sc->sc_cirq, sc->sc_mirq,
		    ae_intr, sc);
		if (sc->sc_ih == NULL) {
			printf("%s: unable to establish interrupt\n",
			    sc->sc_dev.dv_xname);
			return (EIO);
		}
		sc->sc_flags |= AE_ENABLED;
	}
	return (0);
}

/*
 * ae_disable:
 *
 *	Disable the chip.
 */
static void
ae_disable(struct ae_softc *sc)
{

	if (AE_IS_ENABLED(sc)) {
		arbus_intr_disestablish(sc->sc_ih);
		sc->sc_flags &= ~AE_ENABLED;
	}
}

/*
 * ae_power:
 *
 *	Power management (suspend/resume) hook.
 */
static void
ae_power(int why, void *arg)
{
	struct ae_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s;

	printf("power called: %d, %x\n", why, (uint32_t)arg);
	s = splnet();
	switch (why) {
	case PWR_STANDBY:
		/* do nothing! */
		break;
	case PWR_SUSPEND:
		ae_stop(ifp, 0);
		ae_disable(sc);
		break;
	case PWR_RESUME:
		if (ifp->if_flags & IFF_UP) {
			ae_enable(sc);
			ae_init(ifp);
		}
		break;
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
	splx(s);
}

/*
 * ae_rxdrain:
 *
 *	Drain the receive queue.
 */
static void
ae_rxdrain(struct ae_softc *sc)
{
	struct ae_rxsoft *rxs;
	int i;

	for (i = 0; i < AE_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

/*
 * ae_stop:		[ ifnet interface function ]
 *
 *	Stop transmission on the interface.
 */
static void
ae_stop(struct ifnet *ifp, int disable)
{
	struct ae_softc *sc = ifp->if_softc;
	struct ae_txsoft *txs;

	if (sc->sc_tick != NULL) {
		/* Stop the one second clock. */
		callout_stop(&sc->sc_tick_callout);
	}

	/* Down the MII. */
	mii_down(&sc->sc_mii);

	/* Disable interrupts. */
	AE_WRITE(sc, CSR_INTEN, 0);

	/* Stop the transmit and receive processes. */
	AE_WRITE(sc, CSR_OPMODE, 0);
	AE_WRITE(sc, CSR_RXLIST, 0);
	AE_WRITE(sc, CSR_TXLIST, 0);
	AE_CLR(sc, CSR_MACCTL, MACCTL_TE | MACCTL_RE);
	AE_BARRIER(sc);

	/*
	 * Release any queued transmit buffers.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs_q);
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	if (disable) {
		ae_rxdrain(sc);
		ae_disable(sc);
	}

	/*
	 * Mark the interface down and cancel the watchdog timer.
	 */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	sc->sc_if_flags = ifp->if_flags;
	ifp->if_timer = 0;

	/*
	 * Reset the chip (needed on some flavors to actually disable it).
	 */
	ae_reset(sc);
}

/*
 * ae_add_rxbuf:
 *
 *	Add a receive buffer to the indicated descriptor.
 */
static int
ae_add_rxbuf(struct ae_softc *sc, int idx)
{
	struct ae_rxsoft *rxs = &sc->sc_rxsoft[idx];
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);

	MCLAIM(m, &sc->sc_ethercom.ec_rx_mowner);
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return (ENOBUFS);
	}

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, rxs->rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL,
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load rx DMA map %d, error = %d\n",
		    sc->sc_dev.dv_xname, idx, error);
		panic("ae_add_rxbuf");	/* XXX */
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	AE_INIT_RXDESC(sc, idx);

	return (0);
}

/*
 * ae_filter_setup:
 *
 *	Set the chip's receive filter.
 */
static void
ae_filter_setup(struct ae_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t hash, mchash[2];
	uint32_t macctl = 0;

	/*
	 * If the chip is running, we need to reset the interface,
	 * and will revisit here (with IFF_RUNNING) clear.  The
	 * chip seems to really not like to have its multicast
	 * filter programmed without a reset.
	 */
	if (ifp->if_flags & IFF_RUNNING) {
		(void) ae_init(ifp);
		return;
	}

	DPRINTF(sc, ("%s: ae_filter_setup: sc_flags 0x%08x\n",
	    sc->sc_dev.dv_xname, sc->sc_flags));

	macctl = AE_READ(sc, CSR_MACCTL);
	macctl &= ~(MACCTL_PR | MACCTL_PM);
	macctl |= MACCTL_HASH;
	macctl |= MACCTL_HBD;
	macctl |= MACCTL_PR;

	if (ifp->if_flags & IFF_PROMISC) {
		macctl |= MACCTL_PR;
		goto allmulti;
	}

	mchash[0] = mchash[1] = 0;

	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			goto allmulti;
		}

		/* Verify whether we use big or little endian hashes */
		hash = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN) & 0x3f;
		mchash[hash >> 5] |= 1 << (hash & 0x1f);
		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
	goto setit;

 allmulti:
	ifp->if_flags |= IFF_ALLMULTI;
	mchash[0] = mchash[1] = 0xffffffff;
	macctl |= MACCTL_PM;

 setit:
	AE_WRITE(sc, CSR_HTHI, mchash[0]);
	AE_WRITE(sc, CSR_HTHI, mchash[1]);

	AE_WRITE(sc, CSR_MACCTL, macctl);
	AE_BARRIER(sc);

	DPRINTF(sc, ("%s: ae_filter_setup: returning %x\n",
		    sc->sc_dev.dv_xname, macctl));
}

/*
 * ae_idle:
 *
 *	Cause the transmit and/or receive processes to go idle.
 */
void
ae_idle(struct ae_softc *sc, u_int32_t bits)
{
	static const char * const txstate_names[] = {
		"STOPPED",
		"RUNNING - FETCH",
		"RUNNING - WAIT",
		"RUNNING - READING",
		"-- RESERVED --",
		"RUNNING - SETUP",
		"SUSPENDED",
		"RUNNING - CLOSE",
	};
	static const char * const rxstate_names[] = {
		"STOPPED",
		"RUNNING - FETCH",
		"RUNNING - CHECK",
		"RUNNING - WAIT",
		"SUSPENDED",
		"RUNNING - CLOSE",
		"RUNNING - FLUSH",
		"RUNNING - QUEUE",
	};

	u_int32_t csr, ackmask = 0;
	int i;

	if (bits & OPMODE_ST)
		ackmask |= STATUS_TPS;

	if (bits & OPMODE_SR)
		ackmask |= STATUS_RPS;

	AE_CLR(sc, CSR_OPMODE, bits);

	for (i = 0; i < 1000; i++) {
		if (AE_ISSET(sc, CSR_STATUS, ackmask) == ackmask)
			break;
		delay(10);
	}

	csr = AE_READ(sc, CSR_STATUS);
	if ((csr & ackmask) != ackmask) {
		if ((bits & OPMODE_ST) != 0 && (csr & STATUS_TPS) == 0 &&
		    (csr & STATUS_TS) != STATUS_TS_STOPPED) {
			printf("%s: transmit process failed to idle: "
			    "state %s\n", sc->sc_dev.dv_xname,
			    txstate_names[(csr & STATUS_TS) >> 20]);
		}
		if ((bits & OPMODE_SR) != 0 && (csr & STATUS_RPS) == 0 &&
		    (csr & STATUS_RS) != STATUS_RS_STOPPED) {
			printf("%s: receive process failed to idle: "
			    "state %s\n", sc->sc_dev.dv_xname,
			    rxstate_names[(csr & STATUS_RS) >> 17]);
		}
	}
}

/*****************************************************************************
 * Generic media support functions.
 *****************************************************************************/

/*
 * ae_mediastatus:	[ifmedia interface function]
 *
 *	Query the current media.
 */
void
ae_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct ae_softc *sc = ifp->if_softc;

	if (AE_IS_ENABLED(sc) == 0) {
		ifmr->ifm_active = IFM_ETHER | IFM_NONE;
		ifmr->ifm_status = 0;
		return;
	}

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

/*
 * ae_mediachange:	[ifmedia interface function]
 *
 *	Update the current media.
 */
int
ae_mediachange(struct ifnet *ifp)
{
	struct ae_softc *sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_UP) == 0)
		return (0);

	mii_mediachg(&sc->sc_mii);
	return (0);
}

/*****************************************************************************
 * Support functions for MII-attached media.
 *****************************************************************************/

/*
 * ae_mii_tick:
 *
 *	One second timer, used to tick the MII.
 */
static void
ae_mii_tick(void *arg)
{
	struct ae_softc *sc = arg;
	int s;

	if (!device_is_active(&sc->sc_dev))
		return;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_callout, hz, sc->sc_tick, sc);
}

/*
 * ae_mii_statchg:	[mii interface function]
 *
 *	Callback from PHY when media changes.
 */
static void
ae_mii_statchg(struct device *self)
{
	struct ae_softc *sc = (struct ae_softc *)self;
	uint32_t	macctl, flowc;

	//opmode = AE_READ(sc, CSR_OPMODE);
	macctl = AE_READ(sc, CSR_MACCTL);

	/* XXX: do we need to do this? */
	/* Idle the transmit and receive processes. */
	//ae_idle(sc, OPMODE_ST|OPMODE_SR);

	if (sc->sc_mii.mii_media_active & IFM_FDX) {
		flowc = FLOWC_FCE;
		macctl &= ~MACCTL_DRO;
		macctl |= MACCTL_FDX;
	} else {
		flowc = 0;	/* cannot do flow control in HDX */
		macctl |= MACCTL_DRO;
		macctl &= ~MACCTL_FDX;
	}

	AE_WRITE(sc, CSR_FLOWC, flowc);
	AE_WRITE(sc, CSR_MACCTL, macctl);

	/* restore operational mode */
	//AE_WRITE(sc, CSR_OPMODE, opmode);
	AE_BARRIER(sc);
}

/*
 * ae_mii_readreg:
 *
 *	Read a PHY register.
 */
static int
ae_mii_readreg(struct device *self, int phy, int reg)
{
	struct ae_softc	*sc = (struct ae_softc *)self;
	uint32_t	addr;
	int		i;

	addr = (phy << MIIADDR_PHY_SHIFT) | (reg << MIIADDR_REG_SHIFT);
	AE_WRITE(sc, CSR_MIIADDR, addr);
	AE_BARRIER(sc);
	for (i = 0; i < 100000000; i++) {
		if ((AE_READ(sc, CSR_MIIADDR) & MIIADDR_BUSY) == 0)
			break;
	}

	return (AE_READ(sc, CSR_MIIDATA) & 0xffff);
}

/*
 * ae_mii_writereg:
 *
 *	Write a PHY register.
 */
static void
ae_mii_writereg(struct device *self, int phy, int reg, int val)
{
	struct ae_softc *sc = (struct ae_softc *)self;
	uint32_t	addr;
	int		i;

	/* write the data register */
	AE_WRITE(sc, CSR_MIIDATA, val);

	/* write the address to latch it in */
	addr = (phy << MIIADDR_PHY_SHIFT) | (reg << MIIADDR_REG_SHIFT) |
	    MIIADDR_WRITE;
	AE_WRITE(sc, CSR_MIIADDR, addr);
	AE_BARRIER(sc);

	for (i = 0; i < 100000000; i++) {
		if ((AE_READ(sc, CSR_MIIADDR) & MIIADDR_BUSY) == 0)
			break;
	}
}
