/*	$NetBSD: gem.c,v 1.28 2003/08/24 18:07:03 chs Exp $ */

/*
 * 
 * Copyright (C) 2001 Eduardo Horvath.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Driver for Sun GEM ethernet controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gem.c,v 1.28 2003/08/24 18:07:03 chs Exp $");

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/callout.h>
#include <sys/mbuf.h>   
#include <sys/syslog.h>
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

#include <dev/ic/gemreg.h>
#include <dev/ic/gemvar.h>

#define TRIES	10000

void		gem_start __P((struct ifnet *));
void		gem_stop __P((struct ifnet *, int));
int		gem_ioctl __P((struct ifnet *, u_long, caddr_t));
void		gem_tick __P((void *));
void		gem_watchdog __P((struct ifnet *));
void		gem_shutdown __P((void *));
int		gem_init __P((struct ifnet *));
void		gem_init_regs(struct gem_softc *sc);
static int	gem_ringsize(int sz);
int		gem_meminit __P((struct gem_softc *));
void		gem_mifinit __P((struct gem_softc *));
void		gem_reset __P((struct gem_softc *));
int		gem_reset_rx(struct gem_softc *sc);
int		gem_reset_tx(struct gem_softc *sc);
int		gem_disable_rx(struct gem_softc *sc);
int		gem_disable_tx(struct gem_softc *sc);
void		gem_rxdrain(struct gem_softc *sc);
int		gem_add_rxbuf(struct gem_softc *sc, int idx);
void		gem_setladrf __P((struct gem_softc *));

/* MII methods & callbacks */
static int	gem_mii_readreg __P((struct device *, int, int));
static void	gem_mii_writereg __P((struct device *, int, int, int));
static void	gem_mii_statchg __P((struct device *));

int		gem_mediachange __P((struct ifnet *));
void		gem_mediastatus __P((struct ifnet *, struct ifmediareq *));

struct mbuf	*gem_get __P((struct gem_softc *, int, int));
int		gem_put __P((struct gem_softc *, int, struct mbuf *));
void		gem_read __P((struct gem_softc *, int, int));
int		gem_eint __P((struct gem_softc *, u_int));
int		gem_rint __P((struct gem_softc *));
int		gem_tint __P((struct gem_softc *));
void		gem_power __P((int, void *));

#ifdef GEM_DEBUG
#define	DPRINTF(sc, x)	if ((sc)->sc_ethercom.ec_if.if_flags & IFF_DEBUG) \
				printf x
#else
#define	DPRINTF(sc, x)	/* nothing */
#endif


/*
 * gem_attach:
 *
 *	Attach a Gem interface to the system.
 */
void
gem_attach(sc, enaddr)
	struct gem_softc *sc;
	const uint8_t *enaddr;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mii_data *mii = &sc->sc_mii;
	struct mii_softc *child;
	struct ifmedia_entry *ifm;
	int i, error;
	u_int32_t v;

	/* Make sure the chip is stopped. */
	ifp->if_softc = sc;
	gem_reset(sc);

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmatag,
	    sizeof(struct gem_control_data), PAGE_SIZE, 0, &sc->sc_cdseg,
	    1, &sc->sc_cdnseg, 0)) != 0) {
		aprint_error(
		   "%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

/* XXX should map this in with correct endianness */
	if ((error = bus_dmamem_map(sc->sc_dmatag, &sc->sc_cdseg, sc->sc_cdnseg,
	    sizeof(struct gem_control_data), (caddr_t *)&sc->sc_control_data,
	    BUS_DMA_COHERENT)) != 0) {
		aprint_error("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->sc_dmatag,
	    sizeof(struct gem_control_data), 1,
	    sizeof(struct gem_control_data), 0, 0, &sc->sc_cddmamap)) != 0) {
		aprint_error("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmatag, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct gem_control_data), NULL,
	    0)) != 0) {
		aprint_error(
		    "%s: unable to load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_3;
	}

	/*
	 * Initialize the transmit job descriptors.
	 */
	SIMPLEQ_INIT(&sc->sc_txfreeq);
	SIMPLEQ_INIT(&sc->sc_txdirtyq);

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < GEM_TXQUEUELEN; i++) {
		struct gem_txsoft *txs;

		txs = &sc->sc_txsoft[i];
		txs->txs_mbuf = NULL;
		if ((error = bus_dmamap_create(sc->sc_dmatag,
		    ETHER_MAX_LEN_JUMBO, GEM_NTXSEGS,
		    ETHER_MAX_LEN_JUMBO, 0, 0,
		    &txs->txs_dmamap)) != 0) {
			aprint_error("%s: unable to create tx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_4;
		}
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < GEM_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmatag, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			aprint_error("%s: unable to create rx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	/*
	 * From this point forward, the attachment cannot fail.  A failure
	 * before this point releases all resources that may have been
	 * allocated.
	 */

	/* Announce ourselves. */
	aprint_normal("%s: Ethernet address %s", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr));

	/* Get RX FIFO size */
	sc->sc_rxfifosize = 64 *
	    bus_space_read_4(sc->sc_bustag, sc->sc_h, GEM_RX_FIFO_SIZE); 
	aprint_normal(", %uKB RX fifo", sc->sc_rxfifosize / 1024);

	/* Get TX FIFO size */
	v = bus_space_read_4(sc->sc_bustag, sc->sc_h, GEM_TX_FIFO_SIZE); 
	aprint_normal(", %uKB TX fifo\n", v / 16);

	/* Initialize ifnet structure. */
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;
	ifp->if_start = gem_start;
	ifp->if_ioctl = gem_ioctl;
	ifp->if_watchdog = gem_watchdog;
	ifp->if_stop = gem_stop;
	ifp->if_init = gem_init;
	IFQ_SET_READY(&ifp->if_snd);

	/* Initialize ifmedia structures and MII info */
	mii->mii_ifp = ifp;
	mii->mii_readreg = gem_mii_readreg; 
	mii->mii_writereg = gem_mii_writereg;
	mii->mii_statchg = gem_mii_statchg;

	ifmedia_init(&mii->mii_media, IFM_IMASK, gem_mediachange, gem_mediastatus);

	gem_mifinit(sc);

	mii_attach(&sc->sc_dev, mii, 0xffffffff,
			MII_PHY_ANY, MII_OFFSET_ANY, MIIF_FORCEANEG);

	child = LIST_FIRST(&mii->mii_phys);
	if (child == NULL) {
		/* No PHY attached */
		ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_MANUAL);
	} else {
		/*
		 * Walk along the list of attached MII devices and
		 * establish an `MII instance' to `phy number'
		 * mapping. We'll use this mapping in media change
		 * requests to determine which phy to use to program
		 * the MIF configuration register.
		 */
		for (; child != NULL; child = LIST_NEXT(child, mii_list)) {
			/*
			 * Note: we support just two PHYs: the built-in
			 * internal device and an external on the MII
			 * connector.
			 */
			if (child->mii_phy > 1 || child->mii_inst > 1) {
				aprint_error(
				    "%s: cannot accomodate MII device %s"
				       " at phy %d, instance %d\n",
				       sc->sc_dev.dv_xname,
				       child->mii_dev.dv_xname,
				       child->mii_phy, child->mii_inst);
				continue;
			}

			sc->sc_phys[child->mii_inst] = child->mii_phy;
			
		}

		/*
		 * Now select and activate the PHY we will use.
		 *
		 * The order of preference is External (MDI1),
		 * Internal (MDI0), Serial Link (no MII).
		 */
		if (sc->sc_phys[1]) {
#ifdef DEBUG
			aprint_debug("using external phy\n");
#endif
			sc->sc_mif_config |= GEM_MIF_CONFIG_PHY_SEL;
		} else {
#ifdef DEBUG
			aprint_debug("using internal phy\n");
#endif
			sc->sc_mif_config &= ~GEM_MIF_CONFIG_PHY_SEL;
		}
		bus_space_write_4(sc->sc_bustag, sc->sc_h, GEM_MIF_CONFIG, 
			sc->sc_mif_config);

		/*
		 * XXX - we can really do the following ONLY if the
		 * phy indeed has the auto negotiation capability!!
		 */
		ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_AUTO);
	}

	/*
	 * If we support GigE media, we support jumbo frames too.
	 * Unless we are Apple.
	 */
	TAILQ_FOREACH(ifm, &sc->sc_media.ifm_list, ifm_list) {
		if (IFM_SUBTYPE(ifm->ifm_media) == IFM_1000_T ||
		    IFM_SUBTYPE(ifm->ifm_media) == IFM_1000_SX ||
		    IFM_SUBTYPE(ifm->ifm_media) == IFM_1000_LX ||
		    IFM_SUBTYPE(ifm->ifm_media) == IFM_1000_CX) {
			if (sc->sc_variant != GEM_APPLE_GMAC)
				sc->sc_ethercom.ec_capabilities
				    |= ETHERCAP_JUMBO_MTU;

			sc->sc_flags |= GEM_GIGABIT;
			break;
		}
	}

	/* claim 802.1q capability */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	sc->sc_sh = shutdownhook_establish(gem_shutdown, sc);
	if (sc->sc_sh == NULL)
		panic("gem_config: can't establish shutdownhook");

#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_NET, 0);
#endif

	evcnt_attach_dynamic(&sc->sc_ev_intr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "interrupts");
#ifdef GEM_COUNTERS
	evcnt_attach_dynamic(&sc->sc_ev_txint, EVCNT_TYPE_INTR,
	    &sc->sc_ev_intr, sc->sc_dev.dv_xname, "tx interrupts");
	evcnt_attach_dynamic(&sc->sc_ev_rxint, EVCNT_TYPE_INTR,
	    &sc->sc_ev_intr, sc->sc_dev.dv_xname, "rx interrupts");
	evcnt_attach_dynamic(&sc->sc_ev_rxfull, EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx ring full");
	evcnt_attach_dynamic(&sc->sc_ev_rxnobuf, EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx malloc failure");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[0], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx 0desc");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[1], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx 1desc");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[2], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx 2desc");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[3], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx 3desc");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[4], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx >3desc");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[5], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx >7desc");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[6], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx >15desc");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[7], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx >31desc");
	evcnt_attach_dynamic(&sc->sc_ev_rxhist[8], EVCNT_TYPE_INTR,
	    &sc->sc_ev_rxint, sc->sc_dev.dv_xname, "rx >63desc");
#endif

#if notyet
	/*
	 * Add a suspend hook to make sure we come back up after a
	 * resume.
	 */
	sc->sc_powerhook = powerhook_establish(gem_power, sc);
	if (sc->sc_powerhook == NULL)
		aprint_error("%s: WARNING: unable to establish power hook\n",
		    sc->sc_dev.dv_xname);
#endif

	callout_init(&sc->sc_tick_ch);
	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
 fail_5:
	for (i = 0; i < GEM_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmatag,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
 fail_4:
	for (i = 0; i < GEM_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmatag,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmatag, sc->sc_cddmamap);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmatag, sc->sc_cddmamap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmatag, (caddr_t)sc->sc_control_data,
	    sizeof(struct gem_control_data));
 fail_1:
	bus_dmamem_free(sc->sc_dmatag, &sc->sc_cdseg, sc->sc_cdnseg);
 fail_0:
	return;
}


void
gem_tick(arg)
	void *arg;
{
	struct gem_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_ch, hz, gem_tick, sc);
	
}

void
gem_reset(sc)
	struct gem_softc *sc;
{
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	int i;
	int s;

	s = splnet();
	DPRINTF(sc, ("%s: gem_reset\n", sc->sc_dev.dv_xname));
	gem_reset_rx(sc);
	gem_reset_tx(sc);

	/* Do a full reset */
	bus_space_write_4(t, h, GEM_RESET, GEM_RESET_RX|GEM_RESET_TX);
	for (i=TRIES; i--; delay(100))
		if ((bus_space_read_4(t, h, GEM_RESET) & 
			(GEM_RESET_RX|GEM_RESET_TX)) == 0)
			break;
	if ((bus_space_read_4(t, h, GEM_RESET) &
		(GEM_RESET_RX|GEM_RESET_TX)) != 0) {
		printf("%s: cannot reset device\n",
			sc->sc_dev.dv_xname);
	}
	splx(s);
}


/*
 * gem_rxdrain:
 *
 *	Drain the receive queue.
 */
void
gem_rxdrain(struct gem_softc *sc)
{
	struct gem_rxsoft *rxs;
	int i;

	for (i = 0; i < GEM_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmatag, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

/* 
 * Reset the whole thing.
 */
void
gem_stop(struct ifnet *ifp, int disable)
{
	struct gem_softc *sc = (struct gem_softc *)ifp->if_softc;
	struct gem_txsoft *txs;

	DPRINTF(sc, ("%s: gem_stop\n", sc->sc_dev.dv_xname));

	callout_stop(&sc->sc_tick_ch);
	mii_down(&sc->sc_mii);

	/* XXX - Should we reset these instead? */
	gem_disable_rx(sc);
	gem_disable_tx(sc);

	/*
	 * Release any queued transmit buffers.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs_q);
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmatag, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	if (disable) {
		gem_rxdrain(sc);
	}

	/*
	 * Mark the interface down and cancel the watchdog timer.
	 */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}


/*
 * Reset the receiver
 */
int
gem_reset_rx(struct gem_softc *sc)
{
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	int i;


	/*
	 * Resetting while DMA is in progress can cause a bus hang, so we
	 * disable DMA first.
	 */
	gem_disable_rx(sc);
	bus_space_write_4(t, h, GEM_RX_CONFIG, 0);
	/* Wait till it finishes */
	for (i=TRIES; i--; delay(100))
		if ((bus_space_read_4(t, h, GEM_RX_CONFIG) & 1) == 0)
			break;
	if ((bus_space_read_4(t, h, GEM_RX_CONFIG) & 1) != 0)
		printf("%s: cannot disable read DMA\n",
			sc->sc_dev.dv_xname);

	/* Wait 5ms extra. */
	delay(5000);

	/* Finally, reset the ERX */
	bus_space_write_4(t, h, GEM_RESET, GEM_RESET_RX);
	/* Wait till it finishes */
	for (i=TRIES; i--; delay(100))
		if ((bus_space_read_4(t, h, GEM_RESET) & GEM_RESET_RX) == 0)
			break;
	if ((bus_space_read_4(t, h, GEM_RESET) & GEM_RESET_RX) != 0) {
		printf("%s: cannot reset receiver\n",
			sc->sc_dev.dv_xname);
		return (1);
	}
	return (0);
}


/*
 * Reset the transmitter
 */
int
gem_reset_tx(struct gem_softc *sc)
{
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	int i;

	/*
	 * Resetting while DMA is in progress can cause a bus hang, so we
	 * disable DMA first.
	 */
	gem_disable_tx(sc);
	bus_space_write_4(t, h, GEM_TX_CONFIG, 0);
	/* Wait till it finishes */
	for (i=TRIES; i--; delay(100))
		if ((bus_space_read_4(t, h, GEM_TX_CONFIG) & 1) == 0)
			break;
	if ((bus_space_read_4(t, h, GEM_TX_CONFIG) & 1) != 0)
		printf("%s: cannot disable read DMA\n",
			sc->sc_dev.dv_xname);

	/* Wait 5ms extra. */
	delay(5000);

	/* Finally, reset the ETX */
	bus_space_write_4(t, h, GEM_RESET, GEM_RESET_TX);
	/* Wait till it finishes */
	for (i=TRIES; i--; delay(100))
		if ((bus_space_read_4(t, h, GEM_RESET) & GEM_RESET_TX) == 0)
			break;
	if ((bus_space_read_4(t, h, GEM_RESET) & GEM_RESET_TX) != 0) {
		printf("%s: cannot reset receiver\n",
			sc->sc_dev.dv_xname);
		return (1);
	}
	return (0);
}

/*
 * disable receiver.
 */
int
gem_disable_rx(struct gem_softc *sc)
{
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	int i;
	u_int32_t cfg;

	/* Flip the enable bit */
	cfg = bus_space_read_4(t, h, GEM_MAC_RX_CONFIG);
	cfg &= ~GEM_MAC_RX_ENABLE;
	bus_space_write_4(t, h, GEM_MAC_RX_CONFIG, cfg);

	/* Wait for it to finish */
	for (i=TRIES; i--; delay(100)) 
		if ((bus_space_read_4(t, h, GEM_MAC_RX_CONFIG) &
			GEM_MAC_RX_ENABLE) == 0)
			return (0);
	return (1);
}

/*
 * disable transmitter.
 */
int
gem_disable_tx(struct gem_softc *sc)
{
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	int i;
	u_int32_t cfg;

	/* Flip the enable bit */
	cfg = bus_space_read_4(t, h, GEM_MAC_TX_CONFIG);
	cfg &= ~GEM_MAC_TX_ENABLE;
	bus_space_write_4(t, h, GEM_MAC_TX_CONFIG, cfg);

	/* Wait for it to finish */
	for (i=TRIES; i--; delay(100)) 
		if ((bus_space_read_4(t, h, GEM_MAC_TX_CONFIG) &
			GEM_MAC_TX_ENABLE) == 0)
			return (0);
	return (1);
}

/*
 * Initialize interface.
 */
int
gem_meminit(struct gem_softc *sc)
{
	struct gem_rxsoft *rxs;
	int i, error;

	/*
	 * Initialize the transmit descriptor ring.
	 */
	memset((void *)sc->sc_txdescs, 0, sizeof(sc->sc_txdescs));
	for (i = 0; i < GEM_NTXDESC; i++) {
		sc->sc_txdescs[i].gd_flags = 0;
		sc->sc_txdescs[i].gd_addr = 0;
	}
	GEM_CDTXSYNC(sc, 0, GEM_NTXDESC,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	sc->sc_txfree = GEM_NTXDESC-1;
	sc->sc_txnext = 0;
	sc->sc_txwin = 0;

	/*
	 * Initialize the receive descriptor and receive job
	 * descriptor rings.
	 */
	for (i = 0; i < GEM_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf == NULL) {
			if ((error = gem_add_rxbuf(sc, i)) != 0) {
				printf("%s: unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    sc->sc_dev.dv_xname, i, error);
				/*
				 * XXX Should attempt to run with fewer receive
				 * XXX buffers instead of just failing.
				 */
				gem_rxdrain(sc);
				return (1);
			}
		} else
			GEM_INIT_RXDESC(sc, i);
	}
	sc->sc_rxptr = 0;

	return (0);
}

static int
gem_ringsize(int sz)
{
	int v;

	switch (sz) {
	case 32:
		v = GEM_RING_SZ_32;
		break;
	case 64:
		v = GEM_RING_SZ_64;
		break;
	case 128:
		v = GEM_RING_SZ_128;
		break;
	case 256:
		v = GEM_RING_SZ_256;
		break;
	case 512:
		v = GEM_RING_SZ_512;
		break;
	case 1024:
		v = GEM_RING_SZ_1024;
		break;
	case 2048:
		v = GEM_RING_SZ_2048;
		break;
	case 4096:
		v = GEM_RING_SZ_4096;
		break;
	case 8192:
		v = GEM_RING_SZ_8192;
		break;
	default:
		printf("gem: invalid Receive Descriptor ring size\n");
		break;
	}
	return (v);
}

/*
 * Initialization of interface; set up initialization block
 * and transmit/receive descriptor rings.
 */
int
gem_init(struct ifnet *ifp)
{
	struct gem_softc *sc = (struct gem_softc *)ifp->if_softc;
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	int s;
	u_int max_frame_size;
	u_int32_t v;

	s = splnet();

	DPRINTF(sc, ("%s: gem_init: calling stop\n", sc->sc_dev.dv_xname));
	/*
	 * Initialization sequence. The numbered steps below correspond
	 * to the sequence outlined in section 6.3.5.1 in the Ethernet
	 * Channel Engine manual (part of the PCIO manual).
	 * See also the STP2002-STQ document from Sun Microsystems.
	 */

	/* step 1 & 2. Reset the Ethernet Channel */
	gem_stop(ifp, 0);
	gem_reset(sc);
	DPRINTF(sc, ("%s: gem_init: restarting\n", sc->sc_dev.dv_xname));

	/* Re-initialize the MIF */
	gem_mifinit(sc);

	/* Call MI reset function if any */
	if (sc->sc_hwreset)
		(*sc->sc_hwreset)(sc);

	/* step 3. Setup data structures in host memory */
	gem_meminit(sc);

	/* step 4. TX MAC registers & counters */
	gem_init_regs(sc);
	max_frame_size = max(sc->sc_ethercom.ec_if.if_mtu, ETHERMTU);
	max_frame_size += ETHER_HDR_LEN + ETHER_CRC_LEN;
	if (sc->sc_ethercom.ec_capenable & ETHERCAP_VLAN_MTU)
		max_frame_size += ETHER_VLAN_ENCAP_LEN;
	bus_space_write_4(t, h, GEM_MAC_MAC_MAX_FRAME,
	    max_frame_size|/* burst size */(0x2000<<16));

	/* step 5. RX MAC registers & counters */
	gem_setladrf(sc);

	/* step 6 & 7. Program Descriptor Ring Base Addresses */
	/* NOTE: we use only 32-bit DMA addresses here. */
	bus_space_write_4(t, h, GEM_TX_RING_PTR_HI, 0);
	bus_space_write_4(t, h, GEM_TX_RING_PTR_LO, GEM_CDTXADDR(sc, 0));

	bus_space_write_4(t, h, GEM_RX_RING_PTR_HI, 0);
	bus_space_write_4(t, h, GEM_RX_RING_PTR_LO, GEM_CDRXADDR(sc, 0));

	/* step 8. Global Configuration & Interrupt Mask */
	bus_space_write_4(t, h, GEM_INTMASK,
		      ~(GEM_INTR_TX_INTME|
			GEM_INTR_TX_EMPTY|
			GEM_INTR_RX_DONE|GEM_INTR_RX_NOBUF|
			GEM_INTR_RX_TAG_ERR|GEM_INTR_PCS|
			GEM_INTR_MAC_CONTROL|GEM_INTR_MIF|
			GEM_INTR_BERR));
	bus_space_write_4(t, h, GEM_MAC_RX_MASK,
			GEM_MAC_RX_DONE|GEM_MAC_RX_FRAME_CNT);
	bus_space_write_4(t, h, GEM_MAC_TX_MASK, 0xffff); /* XXXX */
	bus_space_write_4(t, h, GEM_MAC_CONTROL_MASK, 0); /* XXXX */

	/* step 9. ETX Configuration: use mostly default values */

	/* Enable DMA */
	v = gem_ringsize(GEM_NTXDESC /*XXX*/);
	bus_space_write_4(t, h, GEM_TX_CONFIG, 
		v|GEM_TX_CONFIG_TXDMA_EN|
		((0x400<<10)&GEM_TX_CONFIG_TXFIFO_TH));
	bus_space_write_4(t, h, GEM_TX_KICK, sc->sc_txnext);

	/* step 10. ERX Configuration */

	/* Encode Receive Descriptor ring size: four possible values */
	v = gem_ringsize(GEM_NRXDESC /*XXX*/);

	/* Enable DMA */
	bus_space_write_4(t, h, GEM_RX_CONFIG, 
		v|(GEM_THRSH_1024<<GEM_RX_CONFIG_FIFO_THRS_SHIFT)|
		(2<<GEM_RX_CONFIG_FBOFF_SHFT)|GEM_RX_CONFIG_RXDMA_EN|
		(0<<GEM_RX_CONFIG_CXM_START_SHFT));
	/*
	 * The following value is for an OFF Threshold of about 3/4 full
	 * and an ON Threshold of 1/4 full.
	 */
	bus_space_write_4(t, h, GEM_RX_PAUSE_THRESH,
	     (3 * sc->sc_rxfifosize / 256) |
	     (   (sc->sc_rxfifosize / 256) << 12));
	bus_space_write_4(t, h, GEM_RX_BLANKING, (6<<12)|6);

	/* step 11. Configure Media */
	mii_mediachg(&sc->sc_mii);

/* XXXX Serial link needs a whole different setup. */


	/* step 12. RX_MAC Configuration Register */
	v = bus_space_read_4(t, h, GEM_MAC_RX_CONFIG);
	v |= GEM_MAC_RX_ENABLE;
	bus_space_write_4(t, h, GEM_MAC_RX_CONFIG, v);

	/* step 14. Issue Transmit Pending command */

	/* Call MI initialization function if any */
	if (sc->sc_hwinit)
		(*sc->sc_hwinit)(sc);


	/* step 15.  Give the reciever a swift kick */
	bus_space_write_4(t, h, GEM_RX_KICK, GEM_NRXDESC-4);

	/* Start the one second timer. */
	callout_reset(&sc->sc_tick_ch, hz, gem_tick, sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;
	splx(s);

	return (0);
}

void
gem_init_regs(struct gem_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	const u_char *laddr = LLADDR(ifp->if_sadl);
	u_int32_t v;

	/* These regs are not cleared on reset */
	if (!sc->sc_inited) {

		/* Wooo.  Magic values. */
		bus_space_write_4(t, h, GEM_MAC_IPG0, 0);
		bus_space_write_4(t, h, GEM_MAC_IPG1, 8);
		bus_space_write_4(t, h, GEM_MAC_IPG2, 4);

		bus_space_write_4(t, h, GEM_MAC_MAC_MIN_FRAME, ETHER_MIN_LEN);
		/* Max frame and max burst size */
		bus_space_write_4(t, h, GEM_MAC_MAC_MAX_FRAME,
		     ETHER_MAX_LEN | (0x2000<<16));

		bus_space_write_4(t, h, GEM_MAC_PREAMBLE_LEN, 0x7);
		bus_space_write_4(t, h, GEM_MAC_JAM_SIZE, 0x4);
		bus_space_write_4(t, h, GEM_MAC_ATTEMPT_LIMIT, 0x10);
		/* Dunno.... */
		bus_space_write_4(t, h, GEM_MAC_CONTROL_TYPE, 0x8088);
		bus_space_write_4(t, h, GEM_MAC_RANDOM_SEED,
		    ((laddr[5]<<8)|laddr[4])&0x3ff);

		/* Secondary MAC addr set to 0:0:0:0:0:0 */
		bus_space_write_4(t, h, GEM_MAC_ADDR3, 0);
		bus_space_write_4(t, h, GEM_MAC_ADDR4, 0);
		bus_space_write_4(t, h, GEM_MAC_ADDR5, 0);

		/* MAC control addr set to 01:80:c2:00:00:01 */
		bus_space_write_4(t, h, GEM_MAC_ADDR6, 0x0001);
		bus_space_write_4(t, h, GEM_MAC_ADDR7, 0xc200);
		bus_space_write_4(t, h, GEM_MAC_ADDR8, 0x0180);

		/* MAC filter addr set to 0:0:0:0:0:0 */
		bus_space_write_4(t, h, GEM_MAC_ADDR_FILTER0, 0);
		bus_space_write_4(t, h, GEM_MAC_ADDR_FILTER1, 0);
		bus_space_write_4(t, h, GEM_MAC_ADDR_FILTER2, 0);

		bus_space_write_4(t, h, GEM_MAC_ADR_FLT_MASK1_2, 0);
		bus_space_write_4(t, h, GEM_MAC_ADR_FLT_MASK0, 0);

		sc->sc_inited = 1;
	}

	/* Counters need to be zeroed */
	bus_space_write_4(t, h, GEM_MAC_NORM_COLL_CNT, 0);
	bus_space_write_4(t, h, GEM_MAC_FIRST_COLL_CNT, 0);
	bus_space_write_4(t, h, GEM_MAC_EXCESS_COLL_CNT, 0);
	bus_space_write_4(t, h, GEM_MAC_LATE_COLL_CNT, 0);
	bus_space_write_4(t, h, GEM_MAC_DEFER_TMR_CNT, 0);
	bus_space_write_4(t, h, GEM_MAC_PEAK_ATTEMPTS, 0);
	bus_space_write_4(t, h, GEM_MAC_RX_FRAME_COUNT, 0);
	bus_space_write_4(t, h, GEM_MAC_RX_LEN_ERR_CNT, 0);
	bus_space_write_4(t, h, GEM_MAC_RX_ALIGN_ERR, 0);
	bus_space_write_4(t, h, GEM_MAC_RX_CRC_ERR_CNT, 0);
	bus_space_write_4(t, h, GEM_MAC_RX_CODE_VIOL, 0);

	/* Un-pause stuff */
#if 0
	bus_space_write_4(t, h, GEM_MAC_SEND_PAUSE_CMD, 0x1BF0);
#else
	bus_space_write_4(t, h, GEM_MAC_SEND_PAUSE_CMD, 0);
#endif

	/*
	 * Set the station address.
	 */
	bus_space_write_4(t, h, GEM_MAC_ADDR0, (laddr[4]<<8)|laddr[5]);
	bus_space_write_4(t, h, GEM_MAC_ADDR1, (laddr[2]<<8)|laddr[3]);
	bus_space_write_4(t, h, GEM_MAC_ADDR2, (laddr[0]<<8)|laddr[1]);

#if 0
	if (sc->sc_variant != APPLE_GMAC)
		return;
#endif

	/*
	 * Enable MII outputs.  Enable GMII if there is a gigabit PHY.
	 */
	sc->sc_mif_config = bus_space_read_4(t, h, GEM_MIF_CONFIG);
	v = GEM_MAC_XIF_TX_MII_ENA;
	if (sc->sc_mif_config & GEM_MIF_CONFIG_MDI1) {
		v |= GEM_MAC_XIF_FDPLX_LED;
		if (sc->sc_flags & GEM_GIGABIT)
			v |= GEM_MAC_XIF_GMII_MODE;
	}
	bus_space_write_4(t, h, GEM_MAC_XIF_CONFIG, v);
}

void
gem_start(ifp)
	struct ifnet *ifp;
{
	struct gem_softc *sc = (struct gem_softc *)ifp->if_softc;
	struct mbuf *m0, *m;
	struct gem_txsoft *txs, *last_txs;
	bus_dmamap_t dmamap;
	int error, firsttx, nexttx, lasttx, ofree, seg;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous number of free descriptors and
	 * the first descriptor we'll use.
	 */
	ofree = sc->sc_txfree;
	firsttx = sc->sc_txnext;

	DPRINTF(sc, ("%s: gem_start: txfree %d, txnext %d\n",
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
		if (bus_dmamap_load_mbuf(sc->sc_dmatag, dmamap, m0,
		      BUS_DMA_WRITE|BUS_DMA_NOWAIT) != 0) {
			if (m0->m_pkthdr.len > MCLBYTES) {
				printf("%s: unable to allocate jumbo Tx "
				    "cluster\n", sc->sc_dev.dv_xname);
				IFQ_DEQUEUE(&ifp->if_snd, m0);
				m_freem(m0);
				continue;
			}
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
			error = bus_dmamap_load_mbuf(sc->sc_dmatag, dmamap,
			    m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
			if (error) {
				printf("%s: unable to load Tx buffer, "
				    "error = %d\n", sc->sc_dev.dv_xname, error);
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
			bus_dmamap_unload(sc->sc_dmatag, dmamap);
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
		bus_dmamap_sync(sc->sc_dmatag, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Initialize the transmit descriptors.
		 */
		for (nexttx = sc->sc_txnext, seg = 0;
		     seg < dmamap->dm_nsegs;
		     seg++, nexttx = GEM_NEXTTX(nexttx)) {
			uint64_t flags;

			/*
			 * If this is the first descriptor we're
			 * enqueueing, set the start of packet flag,
			 * and the checksum stuff if we want the hardware
			 * to do it.
			 */
			sc->sc_txdescs[nexttx].gd_addr =
			    GEM_DMA_WRITE(sc, dmamap->dm_segs[seg].ds_addr);
			flags = dmamap->dm_segs[seg].ds_len & GEM_TD_BUFSIZE;
			if (nexttx == firsttx) {
				flags |= GEM_TD_START_OF_PACKET;
				if (++sc->sc_txwin > GEM_NTXSEGS * 2 / 3) {
					sc->sc_txwin = 0;
					flags |= GEM_TD_INTERRUPT_ME;
				}
			}
			if (seg == dmamap->dm_nsegs - 1) {
				flags |= GEM_TD_END_OF_PACKET;
			}
			sc->sc_txdescs[nexttx].gd_flags =
				GEM_DMA_WRITE(sc, flags);
			lasttx = nexttx;
		}

#ifdef GEM_DEBUG
		if (ifp->if_flags & IFF_DEBUG) {
			printf("     gem_start %p transmit chain:\n", txs);
			for (seg = sc->sc_txnext;; seg = GEM_NEXTTX(seg)) {
				printf("descriptor %d:\t", seg);
				printf("gd_flags:   0x%016llx\t", (long long)
					GEM_DMA_READ(sc, sc->sc_txdescs[seg].gd_flags));
				printf("gd_addr: 0x%016llx\n", (long long)
					GEM_DMA_READ(sc, sc->sc_txdescs[seg].gd_addr));
				if (seg == lasttx)
					break;
			}
		}
#endif

		/* Sync the descriptors we're using. */
		GEM_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
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
		 * The entire packet chain is set up.  
		 * Kick the transmitter.
		 */
		DPRINTF(sc, ("%s: gem_start: kicking tx %d\n",
			sc->sc_dev.dv_xname, nexttx));
		bus_space_write_4(sc->sc_bustag, sc->sc_h, GEM_TX_KICK,
			sc->sc_txnext);

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
		DPRINTF(sc, ("%s: gem_start: watchdog %d\n",
			sc->sc_dev.dv_xname, ifp->if_timer));
	}
}

/*
 * Transmit interrupt.
 */
int
gem_tint(sc)
	struct gem_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t mac = sc->sc_h;
	struct gem_txsoft *txs;
	int txlast;
	int progress = 0;


	DPRINTF(sc, ("%s: gem_tint\n", sc->sc_dev.dv_xname));

	/*
	 * Unload collision counters
	 */
	ifp->if_collisions +=
		bus_space_read_4(t, mac, GEM_MAC_NORM_COLL_CNT) +
		bus_space_read_4(t, mac, GEM_MAC_FIRST_COLL_CNT) +
		bus_space_read_4(t, mac, GEM_MAC_EXCESS_COLL_CNT) +
		bus_space_read_4(t, mac, GEM_MAC_LATE_COLL_CNT);

	/*
	 * then clear the hardware counters.
	 */
	bus_space_write_4(t, mac, GEM_MAC_NORM_COLL_CNT, 0);
	bus_space_write_4(t, mac, GEM_MAC_FIRST_COLL_CNT, 0);
	bus_space_write_4(t, mac, GEM_MAC_EXCESS_COLL_CNT, 0);
	bus_space_write_4(t, mac, GEM_MAC_LATE_COLL_CNT, 0);

	/*
	 * Go through our Tx list and free mbufs for those
	 * frames that have been transmitted.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		GEM_CDTXSYNC(sc, txs->txs_lastdesc,
		    txs->txs_ndescs,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

#ifdef GEM_DEBUG
		if (ifp->if_flags & IFF_DEBUG) {
			int i;
			printf("    txsoft %p transmit chain:\n", txs);
			for (i = txs->txs_firstdesc;; i = GEM_NEXTTX(i)) {
				printf("descriptor %d: ", i);
				printf("gd_flags: 0x%016llx\t", (long long)
					GEM_DMA_READ(sc, sc->sc_txdescs[i].gd_flags));
				printf("gd_addr: 0x%016llx\n", (long long)
					GEM_DMA_READ(sc, sc->sc_txdescs[i].gd_addr));
				if (i == txs->txs_lastdesc)
					break;
			}
		}
#endif

		/*
		 * In theory, we could harveast some descriptors before
		 * the ring is empty, but that's a bit complicated.
		 *
		 * GEM_TX_COMPLETION points to the last descriptor
		 * processed +1.
		 */
		txlast = bus_space_read_4(t, mac, GEM_TX_COMPLETION);
		DPRINTF(sc,
			("gem_tint: txs->txs_lastdesc = %d, txlast = %d\n",
				txs->txs_lastdesc, txlast));
		if (txs->txs_firstdesc <= txs->txs_lastdesc) {
			if ((txlast >= txs->txs_firstdesc) &&
				(txlast <= txs->txs_lastdesc))
				break;
		} else {
			/* Ick -- this command wraps */
			if ((txlast >= txs->txs_firstdesc) ||
				(txlast <= txs->txs_lastdesc))
				break;
		}

		DPRINTF(sc, ("gem_tint: releasing a desc\n"));
		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs_q);

		sc->sc_txfree += txs->txs_ndescs;

		if (txs->txs_mbuf == NULL) {
#ifdef DIAGNOSTIC
				panic("gem_txintr: null mbuf");
#endif
		}

		bus_dmamap_sync(sc->sc_dmatag, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmatag, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;

		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);

		ifp->if_opackets++;
		progress = 1;
	}

#if 0
	DPRINTF(sc, ("gem_tint: GEM_TX_STATE_MACHINE %x "
		"GEM_TX_DATA_PTR %llx "
		"GEM_TX_COMPLETION %x\n",
		bus_space_read_4(sc->sc_bustag, sc->sc_h, GEM_TX_STATE_MACHINE),
		((long long) bus_space_read_4(sc->sc_bustag, sc->sc_h,
			GEM_TX_DATA_PTR_HI) << 32) |
			     bus_space_read_4(sc->sc_bustag, sc->sc_h,
			GEM_TX_DATA_PTR_LO),
		bus_space_read_4(sc->sc_bustag, sc->sc_h, GEM_TX_COMPLETION)));
#endif

	if (progress) {
		if (sc->sc_txfree == GEM_NTXDESC - 1)
			sc->sc_txwin = 0;

		ifp->if_flags &= ~IFF_OACTIVE;
		gem_start(ifp);

		if (SIMPLEQ_EMPTY(&sc->sc_txdirtyq))
			ifp->if_timer = 0;
	}
	DPRINTF(sc, ("%s: gem_tint: watchdog %d\n",
		sc->sc_dev.dv_xname, ifp->if_timer));

	return (1);
}

/*
 * Receive interrupt.
 */
int
gem_rint(sc)
	struct gem_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	struct ether_header *eh;
	struct gem_rxsoft *rxs;
	struct mbuf *m;
	u_int64_t rxstat;
	u_int32_t rxcomp;
	int i, len, progress = 0;

	DPRINTF(sc, ("%s: gem_rint\n", sc->sc_dev.dv_xname));

	/*
	 * Read the completion register once.  This limits
	 * how long the following loop can execute.
	 */
	rxcomp = bus_space_read_4(t, h, GEM_RX_COMPLETION);

	/*
	 * XXXX Read the lastrx only once at the top for speed.
	 */
	DPRINTF(sc, ("gem_rint: sc->rxptr %d, complete %d\n",
		sc->sc_rxptr, rxcomp));

	/*
	 * Go into the loop at least once.
	 */
	for (i = sc->sc_rxptr; i == sc->sc_rxptr || i != rxcomp;
	     i = GEM_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		GEM_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		rxstat = GEM_DMA_READ(sc, sc->sc_rxdescs[i].gd_flags);

		if (rxstat & GEM_RD_OWN) {
			/*
			 * We have processed all of the receive buffers.
			 */
			break;
		}

		progress++;
		ifp->if_ipackets++;

		if (rxstat & GEM_RD_BAD_CRC) {
			ifp->if_ierrors++;
			printf("%s: receive error: CRC error\n",
				sc->sc_dev.dv_xname);
			GEM_INIT_RXDESC(sc, i);
			continue;
		}

		bus_dmamap_sync(sc->sc_dmatag, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);
#ifdef GEM_DEBUG
		if (ifp->if_flags & IFF_DEBUG) {
			printf("    rxsoft %p descriptor %d: ", rxs, i);
			printf("gd_flags: 0x%016llx\t", (long long)
				GEM_DMA_READ(sc, sc->sc_rxdescs[i].gd_flags));
			printf("gd_addr: 0x%016llx\n", (long long)
				GEM_DMA_READ(sc, sc->sc_rxdescs[i].gd_addr));
		}
#endif

		/*
		 * No errors; receive the packet.  Note the Gem
		 * includes the CRC with every packet.
		 */
		len = GEM_RD_BUFLEN(rxstat);

		/*
		 * Allocate a new mbuf cluster.  If that fails, we are
		 * out of memory, and must drop the packet and recycle
		 * the buffer that's already attached to this descriptor.
		 */
		m = rxs->rxs_mbuf;
		if (gem_add_rxbuf(sc, i) != 0) {
			GEM_COUNTER_INCR(sc, sc_ev_rxnobuf);
			ifp->if_ierrors++;
			GEM_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmatag, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
			continue;
		}
		m->m_data += 2; /* We're already off by two */

		eh = mtod(m, struct ether_header *);
		m->m_flags |= M_HASFCS;
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

	if (progress) {
		/* Update the receive pointer. */
		if (i == sc->sc_rxptr) {
			GEM_COUNTER_INCR(sc, sc_ev_rxfull);
#ifdef GEM_DEBUG
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: rint: ring wrap\n",
				    sc->sc_dev.dv_xname);
#endif
		}
		sc->sc_rxptr = i;
		bus_space_write_4(t, h, GEM_RX_KICK, GEM_PREVRX(i));
	}
#ifdef GEM_COUNTERS
	if (progress <= 4) {
		GEM_COUNTER_INCR(sc, sc_ev_rxhist[progress]);
	} else if (progress < 32) {
		if (progress < 16)
			GEM_COUNTER_INCR(sc, sc_ev_rxhist[5]);
		else
			GEM_COUNTER_INCR(sc, sc_ev_rxhist[6]);
			
	} else {
		if (progress < 64)
			GEM_COUNTER_INCR(sc, sc_ev_rxhist[7]);
		else
			GEM_COUNTER_INCR(sc, sc_ev_rxhist[8]);
	}
#endif

	DPRINTF(sc, ("gem_rint: done sc->rxptr %d, complete %d\n",
		sc->sc_rxptr, bus_space_read_4(t, h, GEM_RX_COMPLETION)));

	return (1);
}


/*
 * gem_add_rxbuf:
 *
 *	Add a receive buffer to the indicated descriptor.
 */
int
gem_add_rxbuf(struct gem_softc *sc, int idx)
{
	struct gem_rxsoft *rxs = &sc->sc_rxsoft[idx];
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

#ifdef GEM_DEBUG
/* bzero the packet to check DMA */
	memset(m->m_ext.ext_buf, 0, m->m_ext.ext_size);
#endif

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmatag, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmatag, rxs->rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL,
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load rx DMA map %d, error = %d\n",
		    sc->sc_dev.dv_xname, idx, error);
		panic("gem_add_rxbuf");	/* XXX */
	}

	bus_dmamap_sync(sc->sc_dmatag, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	GEM_INIT_RXDESC(sc, idx);

	return (0);
}


int
gem_eint(sc, status)
	struct gem_softc *sc;
	u_int status;
{
	char bits[128];

	if ((status & GEM_INTR_MIF) != 0) {
		printf("%s: XXXlink status changed\n", sc->sc_dev.dv_xname);
		return (1);
	}

	printf("%s: status=%s\n", sc->sc_dev.dv_xname,
		bitmask_snprintf(status, GEM_INTR_BITS, bits, sizeof(bits)));
	return (1);
}


int
gem_intr(v)
	void *v;
{
	struct gem_softc *sc = (struct gem_softc *)v;
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t seb = sc->sc_h;
	u_int32_t status;
	int r = 0;
#ifdef GEM_DEBUG
	char bits[128];
#endif

	sc->sc_ev_intr.ev_count++;

	status = bus_space_read_4(t, seb, GEM_STATUS);
	DPRINTF(sc, ("%s: gem_intr: cplt 0x%x status %s\n",
		sc->sc_dev.dv_xname, (status >> 19),
		bitmask_snprintf(status, GEM_INTR_BITS, bits, sizeof(bits))));

	if ((status & (GEM_INTR_RX_TAG_ERR | GEM_INTR_BERR)) != 0)
		r |= gem_eint(sc, status);

	if ((status & (GEM_INTR_TX_EMPTY | GEM_INTR_TX_INTME)) != 0) {
		GEM_COUNTER_INCR(sc, sc_ev_txint);
		r |= gem_tint(sc);
	}

	if ((status & (GEM_INTR_RX_DONE | GEM_INTR_RX_NOBUF)) != 0) {
		GEM_COUNTER_INCR(sc, sc_ev_rxint);
		r |= gem_rint(sc);
	}

	/* We should eventually do more than just print out error stats. */
	if (status & GEM_INTR_TX_MAC) {
		int txstat = bus_space_read_4(t, seb, GEM_MAC_TX_STATUS);
		if (txstat & ~GEM_MAC_TX_XMIT_DONE)
			printf("%s: MAC tx fault, status %x\n",
			    sc->sc_dev.dv_xname, txstat);
	}
	if (status & GEM_INTR_RX_MAC) {
		int rxstat = bus_space_read_4(t, seb, GEM_MAC_RX_STATUS);
		if (rxstat & ~GEM_MAC_RX_DONE)
			printf("%s: MAC rx fault, status %x\n",
			    sc->sc_dev.dv_xname, rxstat);
	}
	return (r);
}


void
gem_watchdog(ifp)
	struct ifnet *ifp;
{
	struct gem_softc *sc = ifp->if_softc;

	DPRINTF(sc, ("gem_watchdog: GEM_RX_CONFIG %x GEM_MAC_RX_STATUS %x "
		"GEM_MAC_RX_CONFIG %x\n",
		bus_space_read_4(sc->sc_bustag, sc->sc_h, GEM_RX_CONFIG),
		bus_space_read_4(sc->sc_bustag, sc->sc_h, GEM_MAC_RX_STATUS),
		bus_space_read_4(sc->sc_bustag, sc->sc_h, GEM_MAC_RX_CONFIG)));

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++ifp->if_oerrors;

	/* Try to get more packets going. */
	gem_start(ifp);
}

/*
 * Initialize the MII Management Interface
 */
void
gem_mifinit(sc)
	struct gem_softc *sc;
{
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t mif = sc->sc_h;

	/* Configure the MIF in frame mode */
	sc->sc_mif_config = bus_space_read_4(t, mif, GEM_MIF_CONFIG);
	sc->sc_mif_config &= ~GEM_MIF_CONFIG_BB_ENA;
	bus_space_write_4(t, mif, GEM_MIF_CONFIG, sc->sc_mif_config);
}

/*
 * MII interface
 *
 * The GEM MII interface supports at least three different operating modes:
 *
 * Bitbang mode is implemented using data, clock and output enable registers.
 *
 * Frame mode is implemented by loading a complete frame into the frame
 * register and polling the valid bit for completion.
 *
 * Polling mode uses the frame register but completion is indicated by
 * an interrupt.
 *
 */
static int
gem_mii_readreg(self, phy, reg)
	struct device *self;
	int phy, reg;
{
	struct gem_softc *sc = (void *)self;
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t mif = sc->sc_h;
	int n;
	u_int32_t v;

#ifdef GEM_DEBUG1
	if (sc->sc_debug)
		printf("gem_mii_readreg: phy %d reg %d\n", phy, reg);
#endif

#if 0
	/* Select the desired PHY in the MIF configuration register */
	v = bus_space_read_4(t, mif, GEM_MIF_CONFIG);
	/* Clear PHY select bit */
	v &= ~GEM_MIF_CONFIG_PHY_SEL;
	if (phy == GEM_PHYAD_EXTERNAL)
		/* Set PHY select bit to get at external device */
		v |= GEM_MIF_CONFIG_PHY_SEL;
	bus_space_write_4(t, mif, GEM_MIF_CONFIG, v);
#endif

	/* Construct the frame command */
	v = (reg << GEM_MIF_REG_SHIFT)	| (phy << GEM_MIF_PHY_SHIFT) |
		GEM_MIF_FRAME_READ;

	bus_space_write_4(t, mif, GEM_MIF_FRAME, v);
	for (n = 0; n < 100; n++) {
		DELAY(1);
		v = bus_space_read_4(t, mif, GEM_MIF_FRAME);
		if (v & GEM_MIF_FRAME_TA0)
			return (v & GEM_MIF_FRAME_DATA);
	}

	printf("%s: mii_read timeout\n", sc->sc_dev.dv_xname);
	return (0);
}

static void
gem_mii_writereg(self, phy, reg, val)
	struct device *self;
	int phy, reg, val;
{
	struct gem_softc *sc = (void *)self;
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t mif = sc->sc_h;
	int n;
	u_int32_t v;

#ifdef GEM_DEBUG1
	if (sc->sc_debug)
		printf("gem_mii_writereg: phy %d reg %d val %x\n", 
			phy, reg, val);
#endif

#if 0
	/* Select the desired PHY in the MIF configuration register */
	v = bus_space_read_4(t, mif, GEM_MIF_CONFIG);
	/* Clear PHY select bit */
	v &= ~GEM_MIF_CONFIG_PHY_SEL;
	if (phy == GEM_PHYAD_EXTERNAL)
		/* Set PHY select bit to get at external device */
		v |= GEM_MIF_CONFIG_PHY_SEL;
	bus_space_write_4(t, mif, GEM_MIF_CONFIG, v);
#endif
	/* Construct the frame command */
	v = GEM_MIF_FRAME_WRITE			|
	    (phy << GEM_MIF_PHY_SHIFT)		|
	    (reg << GEM_MIF_REG_SHIFT)		|
	    (val & GEM_MIF_FRAME_DATA);

	bus_space_write_4(t, mif, GEM_MIF_FRAME, v);
	for (n = 0; n < 100; n++) {
		DELAY(1);
		v = bus_space_read_4(t, mif, GEM_MIF_FRAME);
		if (v & GEM_MIF_FRAME_TA0)
			return;
	}

	printf("%s: mii_write timeout\n", sc->sc_dev.dv_xname);
}

static void
gem_mii_statchg(dev)
	struct device *dev;
{
	struct gem_softc *sc = (void *)dev;
#ifdef GEM_DEBUG
	int instance = IFM_INST(sc->sc_mii.mii_media.ifm_cur->ifm_media);
#endif
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t mac = sc->sc_h;
	u_int32_t v;

#ifdef GEM_DEBUG
	if (sc->sc_debug)
		printf("gem_mii_statchg: status change: phy = %d\n", 
			sc->sc_phys[instance]);
#endif


	/* Set tx full duplex options */
	bus_space_write_4(t, mac, GEM_MAC_TX_CONFIG, 0);
	delay(10000); /* reg must be cleared and delay before changing. */
	v = GEM_MAC_TX_ENA_IPG0|GEM_MAC_TX_NGU|GEM_MAC_TX_NGU_LIMIT|
		GEM_MAC_TX_ENABLE;
	if ((IFM_OPTIONS(sc->sc_mii.mii_media_active) & IFM_FDX) != 0) {
		v |= GEM_MAC_TX_IGN_CARRIER|GEM_MAC_TX_IGN_COLLIS;
	}
	bus_space_write_4(t, mac, GEM_MAC_TX_CONFIG, v);

	/* XIF Configuration */
 /* We should really calculate all this rather than rely on defaults */
	v = bus_space_read_4(t, mac, GEM_MAC_XIF_CONFIG);
	v = GEM_MAC_XIF_LINK_LED;
	v |= GEM_MAC_XIF_TX_MII_ENA;

	/* If an external transceiver is connected, enable its MII drivers */
	sc->sc_mif_config = bus_space_read_4(t, mac, GEM_MIF_CONFIG);
	if ((sc->sc_mif_config & GEM_MIF_CONFIG_MDI1) != 0) {
		/* External MII needs echo disable if half duplex. */
		if ((IFM_OPTIONS(sc->sc_mii.mii_media_active) & IFM_FDX) != 0)
			/* turn on full duplex LED */
			v |= GEM_MAC_XIF_FDPLX_LED;
		else
	 		/* half duplex -- disable echo */
		 	v |= GEM_MAC_XIF_ECHO_DISABL;

		if (sc->sc_ethercom.ec_if.if_baudrate == IF_Mbps(1000))
			v |= GEM_MAC_XIF_GMII_MODE;
		else
			v &= ~GEM_MAC_XIF_GMII_MODE;
	} else 
		/* Internal MII needs buf enable */
		v |= GEM_MAC_XIF_MII_BUF_ENA;
	bus_space_write_4(t, mac, GEM_MAC_XIF_CONFIG, v);
}

int
gem_mediachange(ifp)
	struct ifnet *ifp;
{
	struct gem_softc *sc = ifp->if_softc;

	if (IFM_TYPE(sc->sc_media.ifm_media) != IFM_ETHER)
		return (EINVAL);

	return (mii_mediachg(&sc->sc_mii));
}

void
gem_mediastatus(ifp, ifmr)
	struct ifnet *ifp;
	struct ifmediareq *ifmr;
{
	struct gem_softc *sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_UP) == 0)
		return;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
}

int gem_ioctldebug = 0;
/*
 * Process an ioctl request.
 */
int
gem_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct gem_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) { 
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
if (gem_ioctldebug) printf("reset1\n");
			gem_init(ifp);
			delay(50000);
			error = 0;
		}
		break;
	}

	/* Try to get things going again */
	if (ifp->if_flags & IFF_UP) {
if (gem_ioctldebug) printf("start\n");
		gem_start(ifp);
	}
	splx(s);
	return (error);
}


void
gem_shutdown(arg)
	void *arg;
{
	struct gem_softc *sc = (struct gem_softc *)arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	gem_stop(ifp, 1);
}

/*
 * Set up the logical address filter.
 */
void
gem_setladrf(sc)
	struct gem_softc *sc;
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t h = sc->sc_h;
	u_int32_t crc;
	u_int32_t hash[16];
	u_int32_t v;
	int i;

	/* Get current RX configuration */
	v = bus_space_read_4(t, h, GEM_MAC_RX_CONFIG);

	/*
	 * Turn off promiscuous mode, promiscuous group mode (all multicast),
	 * and hash filter.  Depending on the case, the right bit will be
	 * enabled.
	 */
	v &= ~(GEM_MAC_RX_PROMISCUOUS|GEM_MAC_RX_HASH_FILTER|
	    GEM_MAC_RX_PROMISC_GRP);

	if ((ifp->if_flags & IFF_PROMISC) != 0) {
		/* Turn on promiscuous mode */
		v |= GEM_MAC_RX_PROMISCUOUS;
		ifp->if_flags |= IFF_ALLMULTI;
		goto chipit;
	}

	/*
	 * Set up multicast address filter by passing all multicast addresses
	 * through a crc generator, and then using the high order 8 bits as an
	 * index into the 256 bit logical address filter.  The high order 4
	 * bits select the word, while the other 4 bits select the bit within
	 * the word (where bit 0 is the MSB).
	 */

	/* Clear hash table */
	memset(hash, 0, sizeof(hash));

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
			 * XXX use the addr filter for this
			 */
			ifp->if_flags |= IFF_ALLMULTI;
			v |= GEM_MAC_RX_PROMISC_GRP;
			goto chipit;
		}

		/* Get the LE CRC32 of the address */
		crc = ether_crc32_le(enm->enm_addrlo, sizeof(enm->enm_addrlo));

		/* Just want the 8 most significant bits. */
		crc >>= 24;

		/* Set the corresponding bit in the filter. */
		hash[crc >> 4] |= 1 << (15 - (crc & 15));

		ETHER_NEXT_MULTI(step, enm);
	}

	v |= GEM_MAC_RX_HASH_FILTER;
	ifp->if_flags &= ~IFF_ALLMULTI;

	/* Now load the hash table into the chip (if we are using it) */
	for (i = 0; i < 16; i++) {
		bus_space_write_4(t, h,
		    GEM_MAC_HASH0 + i * (GEM_MAC_HASH1-GEM_MAC_HASH0),
		    hash[i]);
	}

chipit:
	bus_space_write_4(t, h, GEM_MAC_RX_CONFIG, v);
}

#if notyet

/*
 * gem_power:
 *
 *	Power management (suspend/resume) hook.
 */
void
gem_power(why, arg)
	int why;
	void *arg;
{
	struct gem_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s;

	s = splnet();
	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		gem_stop(ifp, 1);
		if (sc->sc_power != NULL)
			(*sc->sc_power)(sc, why);
		break;
	case PWR_RESUME:
		if (ifp->if_flags & IFF_UP) {
			if (sc->sc_power != NULL)
				(*sc->sc_power)(sc, why);
			gem_init(ifp);
		}
		break;
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
	splx(s);
}
#endif
