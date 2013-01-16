/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#define _ARM32_BUS_DMA_PRIVATE
#define GMAC_PRIVATE

#include "locators.h"
#include "opt_broadcom.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_eth.c,v 1.14.2.3 2013/01/16 05:32:45 yamt Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/if_dl.h>

#include <net/bpf.h>

#include <dev/mii/miivar.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

//#define BCMETH_MPSAFE

#ifdef BCMETH_COUNTERS
#define	BCMETH_EVCNT_ADD(a,b)	((void)((a).ev_count += (b)))
#else
#define	BCMETH_EVCNT_ADD(a,b)	do { } while (/*CONSTCOND*/0)
#endif
#define	BCMETH_EVCNT_INCR(a)	BCMETH_EVCNT_ADD((a), 1)

#define	BCMETH_RCVOFFSET	10
#define	BCMETH_MAXTXMBUFS	128
#define	BCMETH_NTXSEGS		30
#define	BCMETH_MAXRXMBUFS	255
#define	BCMETH_MINRXMBUFS	64
#define	BCMETH_NRXSEGS		1
#define	BCMETH_RINGSIZE		PAGE_SIZE

#if 0
#define	BCMETH_RCVMAGIC		0xfeedface
#endif

static int bcmeth_ccb_match(device_t, cfdata_t, void *);
static void bcmeth_ccb_attach(device_t, device_t, void *);

struct bcmeth_txqueue {
	bus_dmamap_t txq_descmap;
	struct gmac_txdb *txq_consumer;
	struct gmac_txdb *txq_producer;
	struct gmac_txdb *txq_first;
	struct gmac_txdb *txq_last;
	struct ifqueue txq_mbufs;
	struct mbuf *txq_next;
	size_t txq_free;
	size_t txq_threshold;
	size_t txq_lastintr;
	bus_size_t txq_reg_xmtaddrlo;
	bus_size_t txq_reg_xmtptr;
	bus_size_t txq_reg_xmtctl;
	bus_size_t txq_reg_xmtsts0;
	bus_size_t txq_reg_xmtsts1;
	bus_dma_segment_t txq_descmap_seg;
};

struct bcmeth_rxqueue {
	bus_dmamap_t rxq_descmap;
	struct gmac_rxdb *rxq_consumer;
	struct gmac_rxdb *rxq_producer;
	struct gmac_rxdb *rxq_first;
	struct gmac_rxdb *rxq_last;
	struct mbuf *rxq_mhead;
	struct mbuf **rxq_mtail;
	struct mbuf *rxq_mconsumer;
	size_t rxq_inuse;
	size_t rxq_threshold;
	bus_size_t rxq_reg_rcvaddrlo;
	bus_size_t rxq_reg_rcvptr;
	bus_size_t rxq_reg_rcvctl;
	bus_size_t rxq_reg_rcvsts0;
	bus_size_t rxq_reg_rcvsts1;
	bus_dma_segment_t rxq_descmap_seg;
};

struct bcmeth_mapcache {
	u_int dmc_nmaps;
	u_int dmc_maxseg;
	u_int dmc_maxmaps;
	u_int dmc_maxmapsize;
	bus_dmamap_t dmc_maps[0];
};

struct bcmeth_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	kmutex_t *sc_lock;
	kmutex_t *sc_hwlock;
	struct ethercom sc_ec;
#define	sc_if		sc_ec.ec_if
	struct ifmedia sc_media;
	void *sc_soft_ih;
	void *sc_ih;

	struct bcmeth_rxqueue sc_rxq;
	struct bcmeth_txqueue sc_txq;

	uint32_t sc_maxfrm;
	uint32_t sc_cmdcfg;
	uint32_t sc_intmask;
	uint32_t sc_rcvlazy;
	volatile uint32_t sc_soft_flags;
#define	SOFT_RXINTR		0x01
#define	SOFT_TXINTR		0x02

#ifdef BCMETH_COUNTERS
	struct evcnt sc_ev_intr;
	struct evcnt sc_ev_soft_intr;
	struct evcnt sc_ev_work;
	struct evcnt sc_ev_tx_stall;
	struct evcnt sc_ev_rx_badmagic_lo;
	struct evcnt sc_ev_rx_badmagic_hi;
#endif

	struct ifqueue sc_rx_bufcache;
	struct bcmeth_mapcache *sc_rx_mapcache;     
	struct bcmeth_mapcache *sc_tx_mapcache;

	struct workqueue *sc_workq;
	struct work sc_work;

	volatile uint32_t sc_work_flags;
#define	WORK_RXINTR		0x01
#define	WORK_RXUNDERFLOW	0x02
#define	WORK_REINIT		0x04

	uint8_t sc_enaddr[ETHER_ADDR_LEN];
};

static void bcmeth_ifstart(struct ifnet *);
static void bcmeth_ifwatchdog(struct ifnet *);
static int bcmeth_ifinit(struct ifnet *);
static void bcmeth_ifstop(struct ifnet *, int);
static int bcmeth_ifioctl(struct ifnet *, u_long, void *);

static int bcmeth_mapcache_create(struct bcmeth_softc *,
    struct bcmeth_mapcache **, size_t, size_t, size_t);
static void bcmeth_mapcache_destroy(struct bcmeth_softc *,
    struct bcmeth_mapcache *);
static bus_dmamap_t bcmeth_mapcache_get(struct bcmeth_softc *,
    struct bcmeth_mapcache *);
static void bcmeth_mapcache_put(struct bcmeth_softc *,
    struct bcmeth_mapcache *, bus_dmamap_t);

static int bcmeth_txq_attach(struct bcmeth_softc *,
    struct bcmeth_txqueue *, u_int);
static void bcmeth_txq_purge(struct bcmeth_softc *,
    struct bcmeth_txqueue *);
static void bcmeth_txq_reset(struct bcmeth_softc *,
    struct bcmeth_txqueue *);
static bool bcmeth_txq_consume(struct bcmeth_softc *,
    struct bcmeth_txqueue *);
static bool bcmeth_txq_produce(struct bcmeth_softc *,
    struct bcmeth_txqueue *, struct mbuf *m);
static bool bcmeth_txq_active_p(struct bcmeth_softc *,
    struct bcmeth_txqueue *);

static int bcmeth_rxq_attach(struct bcmeth_softc *,
    struct bcmeth_rxqueue *, u_int);
static bool bcmeth_rxq_produce(struct bcmeth_softc *,
    struct bcmeth_rxqueue *);
static void bcmeth_rxq_purge(struct bcmeth_softc *,
    struct bcmeth_rxqueue *, bool);
static void bcmeth_rxq_reset(struct bcmeth_softc *,
    struct bcmeth_rxqueue *);

static int bcmeth_intr(void *);
#ifdef BCMETH_MPSAFETX
static void bcmeth_soft_txintr(struct bcmeth_softc *);
#endif
static void bcmeth_soft_intr(void *);
static void bcmeth_worker(struct work *, void *);

static int bcmeth_mediachange(struct ifnet *);
static void bcmeth_mediastatus(struct ifnet *, struct ifmediareq *);

static inline uint32_t
bcmeth_read_4(struct bcmeth_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
bcmeth_write_4(struct bcmeth_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

CFATTACH_DECL_NEW(bcmeth_ccb, sizeof(struct bcmeth_softc),
	bcmeth_ccb_match, bcmeth_ccb_attach, NULL, NULL);

static int
bcmeth_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

#ifdef DIAGNOSTIC
	const int port = cf->cf_loc[BCMCCBCF_PORT];
#endif
	KASSERT(port == BCMCCBCF_PORT_DEFAULT || port == loc->loc_port);

	return 1;
}

static void
bcmeth_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmeth_softc * const sc = device_private(self);
	struct ethercom * const ec = &sc->sc_ec;
	struct ifnet * const ifp = &ec->ec_if;
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;
	const char * const xname = device_xname(self);
	prop_dictionary_t dict = device_properties(self);
	int error;

	sc->sc_bst = ccbaa->ccbaa_ccb_bst;
	sc->sc_dmat = ccbaa->ccbaa_dmat;
	bus_space_subregion(sc->sc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	/*
	 * We need to use the coherent dma tag for the GMAC.
	 */
	sc->sc_dmat = &bcm53xx_coherent_dma_tag;

	prop_data_t eaprop = prop_dictionary_get(dict, "mac-address");
        if (eaprop == NULL) {
		uint32_t mac0 = bcmeth_read_4(sc, UNIMAC_MAC_0);
		uint32_t mac1 = bcmeth_read_4(sc, UNIMAC_MAC_1);
		if ((mac0 == 0 && mac1 == 0) || (mac1 & 1)) {
			aprint_error(": mac-address property is missing\n");
			return;
		}
		sc->sc_enaddr[0] = (mac0 >> 0) & 0xff;
		sc->sc_enaddr[1] = (mac0 >> 8) & 0xff;
		sc->sc_enaddr[2] = (mac0 >> 16) & 0xff;
		sc->sc_enaddr[3] = (mac0 >> 24) & 0xff;
		sc->sc_enaddr[4] = (mac1 >> 0) & 0xff;
		sc->sc_enaddr[5] = (mac1 >> 8) & 0xff;
	} else {
		KASSERT(prop_object_type(eaprop) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(eaprop) == ETHER_ADDR_LEN);
		memcpy(sc->sc_enaddr, prop_data_data_nocopy(eaprop),
		    ETHER_ADDR_LEN);
	}
	sc->sc_dev = self;
	sc->sc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTNET);
	sc->sc_hwlock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);

	bcmeth_write_4(sc, GMAC_INTMASK, 0);	// disable interrupts

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	error = bcmeth_rxq_attach(sc, &sc->sc_rxq, 0);
	if (error) {
		aprint_error(": failed to init rxq: %d\n", error);
		return;
	}

	error = bcmeth_txq_attach(sc, &sc->sc_txq, 0);
	if (error) {
		aprint_error(": failed to init txq: %d\n", error);
		return;
	}

	error = bcmeth_mapcache_create(sc, &sc->sc_rx_mapcache, 
	    BCMETH_MAXRXMBUFS, MCLBYTES, BCMETH_NRXSEGS);
	if (error) {
		aprint_error(": failed to allocate rx dmamaps: %d\n", error);
		return;
	}

	error = bcmeth_mapcache_create(sc, &sc->sc_tx_mapcache, 
	    BCMETH_MAXTXMBUFS, MCLBYTES, BCMETH_NTXSEGS);
	if (error) {
		aprint_error(": failed to allocate tx dmamaps: %d\n", error);
		return;
	}

	error = workqueue_create(&sc->sc_workq, xname, bcmeth_worker, sc,
	    (PRI_USER + MAXPRI_USER) / 2, IPL_NET, WQ_MPSAFE|WQ_PERCPU);
	if (error) {
		aprint_error(": failed to create workqueue: %d\n", error);
		return;
	}

	sc->sc_soft_ih = softint_establish(SOFTINT_MPSAFE | SOFTINT_NET,
	    bcmeth_soft_intr, sc);

	sc->sc_ih = intr_establish(loc->loc_intrs[0], IPL_VM, IST_LEVEL,
	    bcmeth_intr, sc);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intrs[0]);
	} else {
		aprint_normal_dev(self, "interrupting on irq %d\n",
		     loc->loc_intrs[0]);
	}

	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	/*
	 * Since each port in plugged into the switch/flow-accelerator,
	 * we hard code at Gige Full-Duplex with Flow Control enabled.
	 */
	int ifmedia = IFM_ETHER|IFM_1000_T|IFM_FDX;
	//ifmedia |= IFM_FLOW|IFM_ETH_TXPAUSE|IFM_ETH_RXPAUSE;
	ifmedia_init(&sc->sc_media, IFM_IMASK, bcmeth_mediachange,
	    bcmeth_mediastatus);
	ifmedia_add(&sc->sc_media, ifmedia, 0, NULL);
	ifmedia_set(&sc->sc_media, ifmedia);

	ec->ec_capabilities = ETHERCAP_VLAN_MTU | ETHERCAP_JUMBO_MTU;

	strlcpy(ifp->if_xname, xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_baudrate = IF_Mbps(1000);
	ifp->if_capabilities = 0;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef BCMETH_MPSAFE
	ifp->if_flags2 = IFF2_MPSAFE;
#endif
	ifp->if_ioctl = bcmeth_ifioctl;
	ifp->if_start = bcmeth_ifstart;
	ifp->if_watchdog = bcmeth_ifwatchdog;
	ifp->if_init = bcmeth_ifinit;
	ifp->if_stop = bcmeth_ifstop;
	IFQ_SET_READY(&ifp->if_snd);

	bcmeth_ifstop(ifp, true);

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

#ifdef BCMETH_COUNTERS
	evcnt_attach_dynamic(&sc->sc_ev_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "intr");
	evcnt_attach_dynamic(&sc->sc_ev_soft_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "soft intr");
	evcnt_attach_dynamic(&sc->sc_ev_work, EVCNT_TYPE_MISC,
	    NULL, xname, "work items");
	evcnt_attach_dynamic(&sc->sc_ev_tx_stall, EVCNT_TYPE_MISC,
	    NULL, xname, "tx stalls");
	evcnt_attach_dynamic(&sc->sc_ev_rx_badmagic_lo, EVCNT_TYPE_MISC,
	    NULL, xname, "rx badmagic lo");
	evcnt_attach_dynamic(&sc->sc_ev_rx_badmagic_hi, EVCNT_TYPE_MISC,
	    NULL, xname, "rx badmagic hi");
#endif
}

static int
bcmeth_mediachange(struct ifnet *ifp)
{
	//struct bcmeth_softc * const sc = ifp->if_softc;
	return 0;
}

static void
bcmeth_mediastatus(struct ifnet *ifp, struct ifmediareq *ifm)
{
	//struct bcmeth_softc * const sc = ifp->if_softc;

	ifm->ifm_status = IFM_AVALID | IFM_ACTIVE;
	ifm->ifm_active = IFM_ETHER | IFM_FDX | IFM_1000_T;
}

static uint64_t
bcmeth_macaddr_create(const uint8_t *enaddr)
{
	return (enaddr[3] << 0)			// UNIMAC_MAC_0
	    |  (enaddr[2] << 8)			// UNIMAC_MAC_0
	    |  (enaddr[1] << 16)		// UNIMAC_MAC_0
	    |  (enaddr[0] << 24)		// UNIMAC_MAC_0
	    |  ((uint64_t)enaddr[5] << 32)	// UNIMAC_MAC_1
	    |  ((uint64_t)enaddr[4] << 40);	// UNIMAC_MAC_1
}

static int
bcmeth_ifinit(struct ifnet *ifp)
{
	struct bcmeth_softc * const sc = ifp->if_softc;
	int error = 0;

	sc->sc_maxfrm = max(ifp->if_mtu + 32, MCLBYTES);
	if (ifp->if_mtu > ETHERMTU_JUMBO)
		return error;

	KASSERT(ifp->if_flags & IFF_UP);

	/*
	 * Stop the interface
	 */
	bcmeth_ifstop(ifp, 0);

	/*
	 * If our frame size has changed (or it's our first time through)
	 * destroy the existing transmit mapcache.
	 */
	if (sc->sc_tx_mapcache != NULL
	    && sc->sc_maxfrm != sc->sc_tx_mapcache->dmc_maxmapsize) {
		bcmeth_mapcache_destroy(sc, sc->sc_tx_mapcache);
		sc->sc_tx_mapcache = NULL;
	}

	if (sc->sc_tx_mapcache == NULL) {
		error = bcmeth_mapcache_create(sc, &sc->sc_tx_mapcache,
		    BCMETH_MAXTXMBUFS, sc->sc_maxfrm, BCMETH_NTXSEGS);
		if (error)
			return error;
	}

	sc->sc_cmdcfg = NO_LENGTH_CHECK | PAUSE_IGNORE
	    | __SHIFTIN(ETH_SPEED_1000, ETH_SPEED)
	    | RX_ENA | TX_ENA;

	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_cmdcfg |= PROMISC_EN;
	} else {
		sc->sc_cmdcfg &= ~PROMISC_EN;
	}

	const uint64_t macstnaddr =
	    bcmeth_macaddr_create(CLLADDR(ifp->if_sadl));

	sc->sc_intmask = DESCPROTOERR|DATAERR|DESCERR;

	/* 5. Load RCVADDR_LO with new pointer */
	bcmeth_rxq_reset(sc, &sc->sc_rxq);

	bcmeth_write_4(sc, sc->sc_rxq.rxq_reg_rcvctl,
	    __SHIFTIN(BCMETH_RCVOFFSET, RCVCTL_RCVOFFSET)
	    | RCVCTL_PARITY_DIS
	    | RCVCTL_OFLOW_CONTINUE
	    | __SHIFTIN(3, RCVCTL_BURSTLEN));

	/* 6. Load XMTADDR_LO with new pointer */
	bcmeth_txq_reset(sc, &sc->sc_txq);

	bcmeth_write_4(sc, sc->sc_txq.txq_reg_xmtctl, XMTCTL_DMA_ACT_INDEX
	    | XMTCTL_PARITY_DIS
	    | __SHIFTIN(3, XMTCTL_BURSTLEN));

	/* 7. Setup other UNIMAC registers */
	bcmeth_write_4(sc, UNIMAC_FRAME_LEN, sc->sc_maxfrm);
	bcmeth_write_4(sc, UNIMAC_MAC_0, (uint32_t)(macstnaddr >>  0));
	bcmeth_write_4(sc, UNIMAC_MAC_1, (uint32_t)(macstnaddr >> 32));
	bcmeth_write_4(sc, UNIMAC_COMMAND_CONFIG, sc->sc_cmdcfg);

	uint32_t devctl = bcmeth_read_4(sc, GMAC_DEVCONTROL);
	devctl |= RGMII_LINK_STATUS_SEL | NWAY_AUTO_POLL_EN | TXARB_STRICT_MODE;
	devctl &= ~FLOW_CTRL_MODE;
	devctl &= ~MIB_RD_RESET_EN;
	devctl &= ~RXQ_OVERFLOW_CTRL_SEL;
	devctl &= ~CPU_FLOW_CTRL_ON;
	bcmeth_write_4(sc, GMAC_DEVCONTROL, devctl);

	/* Setup lazy receive (at most 1ms). */
	sc->sc_rcvlazy =  __SHIFTIN(4, INTRCVLAZY_FRAMECOUNT)
	     | __SHIFTIN(125000000 / 1000, INTRCVLAZY_TIMEOUT);
	bcmeth_write_4(sc, GMAC_INTRCVLAZY, sc->sc_rcvlazy);

	/* 11. Enable transmit queues in TQUEUE, and ensure that the transmit scheduling mode is correctly set in TCTRL. */
	sc->sc_intmask |= XMTINT_0|XMTUF;
	bcmeth_write_4(sc, sc->sc_txq.txq_reg_xmtctl,
	    bcmeth_read_4(sc, sc->sc_txq.txq_reg_xmtctl) | XMTCTL_ENABLE);


	/* 12. Enable receive queues in RQUEUE, */
	sc->sc_intmask |= RCVINT|RCVDESCUF|RCVFIFOOF;
	bcmeth_write_4(sc, sc->sc_rxq.rxq_reg_rcvctl,
	    bcmeth_read_4(sc, sc->sc_rxq.rxq_reg_rcvctl) | RCVCTL_ENABLE);

	bcmeth_rxq_produce(sc, &sc->sc_rxq);	/* fill with rx buffers */

#if 0
	aprint_normal_dev(sc->sc_dev,
	    "devctl=%#x ucmdcfg=%#x xmtctl=%#x rcvctl=%#x\n",
	    devctl, sc->sc_cmdcfg,
	    bcmeth_read_4(sc, sc->sc_txq.txq_reg_xmtctl),
	    bcmeth_read_4(sc, sc->sc_rxq.rxq_reg_rcvctl));
#endif

	sc->sc_soft_flags = 0;

	bcmeth_write_4(sc, GMAC_INTMASK, sc->sc_intmask);

	ifp->if_flags |= IFF_RUNNING;

	return error;
}

static void
bcmeth_ifstop(struct ifnet *ifp, int disable)
{
	struct bcmeth_softc * const sc = ifp->if_softc;
	struct bcmeth_txqueue * const txq = &sc->sc_txq;
	struct bcmeth_rxqueue * const rxq = &sc->sc_rxq;

	KASSERT(!cpu_intr_p());

	sc->sc_soft_flags = 0;
	sc->sc_work_flags = 0;

	/* Disable Rx processing */
	bcmeth_write_4(sc, rxq->rxq_reg_rcvctl,
	    bcmeth_read_4(sc, rxq->rxq_reg_rcvctl) & ~RCVCTL_ENABLE);

	/* Disable Tx processing */
	bcmeth_write_4(sc, txq->txq_reg_xmtctl,
	    bcmeth_read_4(sc, txq->txq_reg_xmtctl) & ~XMTCTL_ENABLE);

	/* Disable all interrupts */
	bcmeth_write_4(sc, GMAC_INTMASK, 0);

	for (;;) {
		uint32_t tx0 = bcmeth_read_4(sc, txq->txq_reg_xmtsts0);
		uint32_t rx0 = bcmeth_read_4(sc, rxq->rxq_reg_rcvsts0);
		if (__SHIFTOUT(tx0, XMTSTATE) == XMTSTATE_DIS
		    && __SHIFTOUT(rx0, RCVSTATE) == RCVSTATE_DIS)
			break;
		delay(50);
	}
	/*
	 * Now reset the controller.
	 *
	 * 3. Set SW_RESET bit in UNIMAC_COMMAND_CONFIG register
	 * 4. Clear SW_RESET bit in UNIMAC_COMMAND_CONFIG register
	 */
	bcmeth_write_4(sc, UNIMAC_COMMAND_CONFIG, SW_RESET);
	bcmeth_write_4(sc, GMAC_INTSTATUS, ~0);
	sc->sc_intmask = 0;
	ifp->if_flags &= ~IFF_RUNNING;

	/*
	 * Let's consume any remaining transmitted packets.  And if we are
	 * disabling the interface, purge ourselves of any untransmitted
	 * packets.  But don't consume any received packets, just drop them.
	 * If we aren't disabling the interface, save the mbufs in the
	 * receive queue for reuse.
	 */
	bcmeth_rxq_purge(sc, &sc->sc_rxq, disable);
	bcmeth_txq_consume(sc, &sc->sc_txq);
	if (disable) {
		bcmeth_txq_purge(sc, &sc->sc_txq);
		IF_PURGE(&ifp->if_snd);
	}

	bcmeth_write_4(sc, UNIMAC_COMMAND_CONFIG, 0);
}

static void
bcmeth_ifwatchdog(struct ifnet *ifp)
{
}

static int
bcmeth_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct bcmeth_softc *sc  = ifp->if_softc;
	struct ifreq * const ifr = data;
	const int s = splnet();
	int error;

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error != ENETRESET)
			break;

		if (cmd == SIOCADDMULTI || cmd == SIOCDELMULTI) {
			error = 0;
			break;
		}
		error = bcmeth_ifinit(ifp);
		break;
	}

	splx(s);
	return error;
}

static void
bcmeth_rxq_desc_presync(
	struct bcmeth_softc *sc,
	struct bcmeth_rxqueue *rxq,
	struct gmac_rxdb *rxdb,
	size_t count)
{
	bus_dmamap_sync(sc->sc_dmat, rxq->rxq_descmap, 
	    (rxdb - rxq->rxq_first) * sizeof(*rxdb), count * sizeof(*rxdb),
	    BUS_DMASYNC_PREWRITE);
}

static void
bcmeth_rxq_desc_postsync(
	struct bcmeth_softc *sc,
	struct bcmeth_rxqueue *rxq,
	struct gmac_rxdb *rxdb,
	size_t count)
{
	bus_dmamap_sync(sc->sc_dmat, rxq->rxq_descmap, 
	    (rxdb - rxq->rxq_first) * sizeof(*rxdb), count * sizeof(*rxdb),
	    BUS_DMASYNC_POSTWRITE);
}

static void
bcmeth_txq_desc_presync(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq,
	struct gmac_txdb *txdb,
	size_t count)
{
	bus_dmamap_sync(sc->sc_dmat, txq->txq_descmap, 
	    (txdb - txq->txq_first) * sizeof(*txdb), count * sizeof(*txdb),
	    BUS_DMASYNC_PREWRITE);
}

static void
bcmeth_txq_desc_postsync(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq,
	struct gmac_txdb *txdb,
	size_t count)
{
	bus_dmamap_sync(sc->sc_dmat, txq->txq_descmap, 
	    (txdb - txq->txq_first) * sizeof(*txdb), count * sizeof(*txdb),
	    BUS_DMASYNC_POSTWRITE);
}

static bus_dmamap_t
bcmeth_mapcache_get(
	struct bcmeth_softc *sc,
	struct bcmeth_mapcache *dmc)
{
	KASSERT(dmc->dmc_nmaps > 0);
	KASSERT(dmc->dmc_maps[dmc->dmc_nmaps-1] != NULL);
	return dmc->dmc_maps[--dmc->dmc_nmaps];
}

static void
bcmeth_mapcache_put(
	struct bcmeth_softc *sc,
	struct bcmeth_mapcache *dmc,
	bus_dmamap_t map)
{
	KASSERT(map != NULL);
	KASSERT(dmc->dmc_nmaps < dmc->dmc_maxmaps);
	dmc->dmc_maps[dmc->dmc_nmaps++] = map;
}

static void
bcmeth_mapcache_destroy(
	struct bcmeth_softc *sc,
	struct bcmeth_mapcache *dmc)
{
	const size_t dmc_size =
	    offsetof(struct bcmeth_mapcache, dmc_maps[dmc->dmc_maxmaps]);

	for (u_int i = 0; i < dmc->dmc_maxmaps; i++) {
		bus_dmamap_destroy(sc->sc_dmat, dmc->dmc_maps[i]);
	}
	kmem_intr_free(dmc, dmc_size);
}

static int
bcmeth_mapcache_create(
	struct bcmeth_softc *sc,
	struct bcmeth_mapcache **dmc_p,
	size_t maxmaps,
	size_t maxmapsize,
	size_t maxseg)
{
	const size_t dmc_size =
	    offsetof(struct bcmeth_mapcache, dmc_maps[maxmaps]);
	struct bcmeth_mapcache * const dmc =
		kmem_intr_zalloc(dmc_size, KM_NOSLEEP);

	dmc->dmc_maxmaps = maxmaps;
	dmc->dmc_nmaps = maxmaps;
	dmc->dmc_maxmapsize = maxmapsize;
	dmc->dmc_maxseg = maxseg;

	for (u_int i = 0; i < maxmaps; i++) {
		int error = bus_dmamap_create(sc->sc_dmat, dmc->dmc_maxmapsize,
		     dmc->dmc_maxseg, dmc->dmc_maxmapsize, 0,
		     BUS_DMA_WAITOK|BUS_DMA_ALLOCNOW, &dmc->dmc_maps[i]);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "failed to creat dma map cache "
			    "entry %u of %zu: %d\n",
			    i, maxmaps, error);
			while (i-- > 0) {
				bus_dmamap_destroy(sc->sc_dmat,
				    dmc->dmc_maps[i]);
			}
			kmem_intr_free(dmc, dmc_size);
			return error;
		}
		KASSERT(dmc->dmc_maps[i] != NULL);
	}

	*dmc_p = dmc;

	return 0;
}

#if 0
static void
bcmeth_dmamem_free(
	bus_dma_tag_t dmat,
	size_t map_size,
	bus_dma_segment_t *seg,
	bus_dmamap_t map,
	void *kvap)
{
	bus_dmamap_destroy(dmat, map);
	bus_dmamem_unmap(dmat, kvap, map_size);
	bus_dmamem_free(dmat, seg, 1);
}
#endif

static int
bcmeth_dmamem_alloc(
	bus_dma_tag_t dmat,
	size_t map_size,
	bus_dma_segment_t *seg,
	bus_dmamap_t *map,
	void **kvap)
{
	int error;
	int nseg;

	*kvap = NULL;
	*map = NULL;

	error = bus_dmamem_alloc(dmat, map_size, 2*PAGE_SIZE, 0,
	   seg, 1, &nseg, 0);
	if (error)
		return error;

	KASSERT(nseg == 1);

	error = bus_dmamem_map(dmat, seg, nseg, map_size, (void **)kvap, 0);
	if (error == 0) {
		error = bus_dmamap_create(dmat, map_size, 1, map_size, 0, 0,
		    map);
		if (error == 0) {
			error = bus_dmamap_load(dmat, *map, *kvap, map_size,
			    NULL, 0);
			if (error == 0)
				return 0;
			bus_dmamap_destroy(dmat, *map);
			*map = NULL;
		}
		bus_dmamem_unmap(dmat, *kvap, map_size);
		*kvap = NULL;
	}
	bus_dmamem_free(dmat, seg, nseg);
	return 0;
}

static struct mbuf *
bcmeth_rx_buf_alloc(
	struct bcmeth_softc *sc)
{
	struct mbuf *m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		printf("%s:%d: %s\n", __func__, __LINE__, "m_gethdr");
		return NULL;
	}
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		printf("%s:%d: %s\n", __func__, __LINE__, "MCLGET");
		m_freem(m);
		return NULL;
	}
	m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;

	bus_dmamap_t map = bcmeth_mapcache_get(sc, sc->sc_rx_mapcache);
	if (map == NULL) {
		printf("%s:%d: %s\n", __func__, __LINE__, "map get");
		m_freem(m);
		return NULL;
	}
	M_SETCTX(m, map);
	m->m_len = m->m_pkthdr.len = MCLBYTES;
	int error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "fail to load rx dmamap: %d\n",
		    error);
		M_SETCTX(m, NULL);
		m_freem(m);
		bcmeth_mapcache_put(sc, sc->sc_rx_mapcache, map);
		return NULL;
	}
	KASSERT(((map->_dm_flags ^ sc->sc_dmat->_ranges[0].dr_flags) & _BUS_DMAMAP_COHERENT) == 0);
	KASSERT(map->dm_mapsize == MCLBYTES);
#ifdef BCMETH_RCVMAGIC
	*mtod(m, uint32_t *) = BCMETH_RCVMAGIC;
	bus_dmamap_sync(sc->sc_dmat, map, 0, sizeof(uint32_t),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, map, sizeof(uint32_t),
	    map->dm_mapsize - sizeof(uint32_t), BUS_DMASYNC_PREREAD);
#else
	bus_dmamap_sync(sc->sc_dmat, map, 0, sizeof(uint32_t),
	    BUS_DMASYNC_PREREAD);
#endif

	return m;
}

static void
bcmeth_rx_map_unload(
	struct bcmeth_softc *sc,
	struct mbuf *m)
{
	KASSERT(m);
	for (; m != NULL; m = m->m_next) {
		bus_dmamap_t map = M_GETCTX(m, bus_dmamap_t);
		KASSERT(map);
		KASSERT(map->dm_mapsize == MCLBYTES);
		bus_dmamap_sync(sc->sc_dmat, map, 0, m->m_len,
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, map);
		bcmeth_mapcache_put(sc, sc->sc_rx_mapcache, map);
		M_SETCTX(m, NULL);
	}
}

static bool
bcmeth_rxq_produce(
	struct bcmeth_softc *sc,
	struct bcmeth_rxqueue *rxq)
{
	struct gmac_rxdb *producer = rxq->rxq_producer;
	bool produced = false;

	while (rxq->rxq_inuse < rxq->rxq_threshold) {
		struct mbuf *m;
		IF_DEQUEUE(&sc->sc_rx_bufcache, m);
		if (m == NULL) {
			m = bcmeth_rx_buf_alloc(sc);
			if (m == NULL) {
				printf("%s: bcmeth_rx_buf_alloc failed\n", __func__);
				break;
			}
		}
		bus_dmamap_t map = M_GETCTX(m, bus_dmamap_t);
		KASSERT(map);

		producer->rxdb_buflen = MCLBYTES;
		producer->rxdb_addrlo = map->dm_segs[0].ds_addr;
		producer->rxdb_flags &= RXDB_FLAG_ET;
		*rxq->rxq_mtail = m;
		rxq->rxq_mtail = &m->m_next;
		m->m_len = MCLBYTES;
		m->m_next = NULL;
		rxq->rxq_inuse++;
		if (++producer == rxq->rxq_last) {
			membar_producer();
			bcmeth_rxq_desc_presync(sc, rxq, rxq->rxq_producer,
			    rxq->rxq_last - rxq->rxq_producer);
			producer = rxq->rxq_producer = rxq->rxq_first;
		}
		produced = true;
	}
	if (produced) {
		membar_producer();
		if (producer != rxq->rxq_producer) {
			bcmeth_rxq_desc_presync(sc, rxq, rxq->rxq_producer,
			    producer - rxq->rxq_producer);
			rxq->rxq_producer = producer;
		}
		bcmeth_write_4(sc, rxq->rxq_reg_rcvptr,
		    rxq->rxq_descmap->dm_segs[0].ds_addr
		    + ((uintptr_t)producer & RCVPTR));
	}
	return true;
}

static void
bcmeth_rx_input(
	struct bcmeth_softc *sc,
	struct mbuf *m,
	uint32_t rxdb_flags)
{
	struct ifnet * const ifp = &sc->sc_if;

	bcmeth_rx_map_unload(sc, m);

	m_adj(m, BCMETH_RCVOFFSET);

	switch (__SHIFTOUT(rxdb_flags, RXSTS_PKTTYPE)) {
	case RXSTS_PKTTYPE_UC:
		break;
	case RXSTS_PKTTYPE_MC:
		m->m_flags |= M_MCAST;
		break;
	case RXSTS_PKTTYPE_BC:
		m->m_flags |= M_BCAST|M_MCAST;
		break;
	default:
		if (sc->sc_cmdcfg & PROMISC_EN) 
			m->m_flags |= M_PROMISC;
		break;
	}
	m->m_pkthdr.rcvif = ifp;

	ifp->if_ipackets++;
	ifp->if_ibytes += m->m_pkthdr.len;

	/*
	 * Let's give it to the network subsystm to deal with.
	 */
#ifdef BCMETH_MPSAFE
	mutex_exit(sc->sc_lock);
	(*ifp->if_input)(ifp, m);
	mutex_enter(sc->sc_lock);
#else
	int s = splnet();
	bpf_mtap(ifp, m);
	(*ifp->if_input)(ifp, m);
	splx(s);
#endif
}

static void
bcmeth_rxq_consume(
	struct bcmeth_softc *sc,
	struct bcmeth_rxqueue *rxq)
{
	struct ifnet * const ifp = &sc->sc_if;
	struct gmac_rxdb *consumer = rxq->rxq_consumer;
	size_t rxconsumed = 0;

	for (;;) {
		if (consumer == rxq->rxq_producer) {
			rxq->rxq_consumer = consumer;
			rxq->rxq_inuse -= rxconsumed;
			KASSERT(rxq->rxq_inuse == 0);
			return;
		}
		
		uint32_t rcvsts0 = bcmeth_read_4(sc, rxq->rxq_reg_rcvsts0);
		uint32_t currdscr = __SHIFTOUT(rcvsts0, RCV_CURRDSCR);
		if (consumer == rxq->rxq_first + currdscr) {
			rxq->rxq_consumer = consumer;
			rxq->rxq_inuse -= rxconsumed;
			return;
		}
		bcmeth_rxq_desc_postsync(sc, rxq, consumer, 1);

		/*
		 * We own this packet again.  Copy the rxsts word from it.
		 */
		rxconsumed++;
		uint32_t rxsts;
		KASSERT(rxq->rxq_mhead != NULL);
		bus_dmamap_t map = M_GETCTX(rxq->rxq_mhead, bus_dmamap_t);
		bus_dmamap_sync(sc->sc_dmat, map, 0, arm_dcache_align,
		    BUS_DMASYNC_POSTREAD);
		memcpy(&rxsts, rxq->rxq_mhead->m_data, 4);
#if 0
		KASSERTMSG(rxsts != BCMETH_RCVMAGIC, "currdscr=%u consumer=%zd",
		    currdscr, consumer - rxq->rxq_first);
#endif

		/*
		 * Get the count of descriptors.  Fetch the correct number
		 * of mbufs.
		 */
#ifdef BCMETH_RCVMAGIC
		size_t desc_count = rxsts != BCMETH_RCVMAGIC ? __SHIFTOUT(rxsts, RXSTS_DESC_COUNT) + 1 : 1;
#else
		size_t desc_count = __SHIFTOUT(rxsts, RXSTS_DESC_COUNT) + 1;
#endif
		struct mbuf *m = rxq->rxq_mhead;
		struct mbuf *m_last = m;
		for (size_t i = 1; i < desc_count; i++) {
			if (++consumer == rxq->rxq_last) {
				consumer = rxq->rxq_first;
			}
			KASSERTMSG(consumer != rxq->rxq_first + currdscr,
			    "i=%zu rxsts=%#x desc_count=%zu currdscr=%u consumer=%zd",
			    i, rxsts, desc_count, currdscr,
			    consumer - rxq->rxq_first);
			m_last = m_last->m_next;
		}

		/*
		 * Now remove it/them from the list of enqueued mbufs.
		 */
		if ((rxq->rxq_mhead = m_last->m_next) == NULL)
			rxq->rxq_mtail = &rxq->rxq_mhead;
		m_last->m_next = NULL;

#ifdef BCMETH_RCVMAGIC
		if (rxsts == BCMETH_RCVMAGIC) {	
			ifp->if_ierrors++;
			if ((m->m_ext.ext_paddr >> 28) == 8) {
				BCMETH_EVCNT_INCR(sc->sc_ev_rx_badmagic_lo);
			} else {
				BCMETH_EVCNT_INCR( sc->sc_ev_rx_badmagic_hi);
			}
			IF_ENQUEUE(&sc->sc_rx_bufcache, m);
		} else
#endif /* BCMETH_RCVMAGIC */
		if (rxsts & (RXSTS_CRC_ERROR|RXSTS_OVERSIZED|RXSTS_PKT_OVERFLOW)) {
			aprint_error_dev(sc->sc_dev, "[%zu]: count=%zu rxsts=%#x\n",
			    consumer - rxq->rxq_first, desc_count, rxsts);
			/*
			 * We encountered an error, take the mbufs and add them
			 * to the rx bufcache so we can quickly reuse them.
			 */
			ifp->if_ierrors++;
			do {
				struct mbuf *m0 = m->m_next;
				m->m_next = NULL;
				IF_ENQUEUE(&sc->sc_rx_bufcache, m);
				m = m0;
			} while (m);
		} else {
			uint32_t framelen = __SHIFTOUT(rxsts, RXSTS_FRAMELEN);
			framelen += BCMETH_RCVOFFSET;
			m->m_pkthdr.len = framelen;
			if (desc_count == 1) {
				KASSERT(framelen <= MCLBYTES);
				m->m_len = framelen;
			} else {
				m_last->m_len = framelen & (MCLBYTES - 1);
			}

#ifdef BCMETH_MPSAFE
			/*
			 * Wrap at the last entry!
			 */
			if (++consumer == rxq->rxq_last) {
				KASSERT(consumer[-1].rxdb_flags & RXDB_FLAG_ET);
				rxq->rxq_consumer = rxq->rxq_first;
			} else {
				rxq->rxq_consumer = consumer;
			}
			rxq->rxq_inuse -= rxconsumed;
#endif /* BCMETH_MPSAFE */

			/*
			 * Receive the packet (which releases our lock)
			 */
			bcmeth_rx_input(sc, m, rxsts);

#ifdef BCMETH_MPSAFE
			/*
			 * Since we had to give up our lock, we need to
			 * refresh these.
			 */
			consumer = rxq->rxq_consumer;
			rxconsumed = 0;
			continue;
#endif /* BCMETH_MPSAFE */
		}

		/*
		 * Wrap at the last entry!
		 */
		if (++consumer == rxq->rxq_last) {
			KASSERT(consumer[-1].rxdb_flags & RXDB_FLAG_ET);
			consumer = rxq->rxq_first;
		}
	}
}

static void
bcmeth_rxq_purge(
	struct bcmeth_softc *sc,
	struct bcmeth_rxqueue *rxq,
	bool discard)
{
	struct mbuf *m;

	if ((m = rxq->rxq_mhead) != NULL) {
		if (discard) {
			bcmeth_rx_map_unload(sc, m);
			m_freem(m);
		} else {
			while (m != NULL) {
				struct mbuf *m0 = m->m_next;
				m->m_next = NULL;
				IF_ENQUEUE(&sc->sc_rx_bufcache, m);
				m = m0;
			}
		}
			
	}

	rxq->rxq_mhead = NULL;
	rxq->rxq_mtail = &rxq->rxq_mhead;
	rxq->rxq_inuse = 0;
}

static void
bcmeth_rxq_reset(
	struct bcmeth_softc *sc,
	struct bcmeth_rxqueue *rxq)
{
	/*
	 * sync all the descriptors
	 */
	bcmeth_rxq_desc_postsync(sc, rxq, rxq->rxq_first,
	    rxq->rxq_last - rxq->rxq_first);

	/*
	 * Make sure we own all descriptors in the ring.
	 */
	struct gmac_rxdb *rxdb;
	for (rxdb = rxq->rxq_first; rxdb < rxq->rxq_last - 1; rxdb++) {
		rxdb->rxdb_flags = RXDB_FLAG_IC;
	}

	/*
	 * Last descriptor has the wrap flag.
	 */
	rxdb->rxdb_flags = RXDB_FLAG_ET|RXDB_FLAG_IC;

	/*
	 * Reset the producer consumer indexes.
	 */
	rxq->rxq_consumer = rxq->rxq_first;
	rxq->rxq_producer = rxq->rxq_first;
	rxq->rxq_inuse = 0;
	if (rxq->rxq_threshold < BCMETH_MINRXMBUFS)
		rxq->rxq_threshold = BCMETH_MINRXMBUFS;

	sc->sc_intmask |= RCVINT|RCVFIFOOF|RCVDESCUF;

	/*
	 * Restart the receiver at the first descriptor
	 */
	bcmeth_write_4(sc, rxq->rxq_reg_rcvaddrlo,
	    rxq->rxq_descmap->dm_segs[0].ds_addr);
}

static int
bcmeth_rxq_attach(
	struct bcmeth_softc *sc,
	struct bcmeth_rxqueue *rxq,
	u_int qno)
{
	size_t desc_count = BCMETH_RINGSIZE / sizeof(rxq->rxq_first[0]);
	int error;
	void *descs;

	KASSERT(desc_count == 256 || desc_count == 512);

	error = bcmeth_dmamem_alloc(sc->sc_dmat, BCMETH_RINGSIZE,
	   &rxq->rxq_descmap_seg, &rxq->rxq_descmap, &descs);
	if (error)
		return error;

	memset(descs, 0, BCMETH_RINGSIZE);
	rxq->rxq_first = descs;
	rxq->rxq_last = rxq->rxq_first + desc_count;
	rxq->rxq_consumer = descs;
	rxq->rxq_producer = descs;

	bcmeth_rxq_purge(sc, rxq, true);
	bcmeth_rxq_reset(sc, rxq);

	rxq->rxq_reg_rcvaddrlo = GMAC_RCVADDR_LOW;
	rxq->rxq_reg_rcvctl = GMAC_RCVCONTROL;
	rxq->rxq_reg_rcvptr = GMAC_RCVPTR;
	rxq->rxq_reg_rcvsts0 = GMAC_RCVSTATUS0;
	rxq->rxq_reg_rcvsts1 = GMAC_RCVSTATUS1;

	return 0;
}

static bool
bcmeth_txq_active_p(
	struct bcmeth_softc * const sc,
	struct bcmeth_txqueue *txq)
{
	return !IF_IS_EMPTY(&txq->txq_mbufs);
}

static bool
bcmeth_txq_fillable_p(
	struct bcmeth_softc * const sc,
	struct bcmeth_txqueue *txq)
{
	return txq->txq_free >= txq->txq_threshold;
}

static int
bcmeth_txq_attach(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq,
	u_int qno)
{
	size_t desc_count = BCMETH_RINGSIZE / sizeof(txq->txq_first[0]);
	int error;
	void *descs;

	KASSERT(desc_count == 256 || desc_count == 512);

	error = bcmeth_dmamem_alloc(sc->sc_dmat, BCMETH_RINGSIZE,
	   &txq->txq_descmap_seg, &txq->txq_descmap, &descs);
	if (error)
		return error;

	memset(descs, 0, BCMETH_RINGSIZE);
	txq->txq_first = descs;
	txq->txq_last = txq->txq_first + desc_count;
	txq->txq_consumer = descs;
	txq->txq_producer = descs;

	IFQ_SET_MAXLEN(&txq->txq_mbufs, BCMETH_MAXTXMBUFS);

	txq->txq_reg_xmtaddrlo = GMAC_XMTADDR_LOW;
	txq->txq_reg_xmtctl = GMAC_XMTCONTROL;
	txq->txq_reg_xmtptr = GMAC_XMTPTR;
	txq->txq_reg_xmtsts0 = GMAC_XMTSTATUS0;
	txq->txq_reg_xmtsts1 = GMAC_XMTSTATUS1;

	bcmeth_txq_reset(sc, txq);

	return 0;
}

static int
bcmeth_txq_map_load(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq,
	struct mbuf *m)
{
	bus_dmamap_t map;
	int error;

	map = M_GETCTX(m, bus_dmamap_t);
	if (map != NULL)
		return 0;

	map = bcmeth_mapcache_get(sc, sc->sc_tx_mapcache);
	if (map == NULL)
		return ENOMEM;

	error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
	    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	if (error)
		return error;

	bus_dmamap_sync(sc->sc_dmat, map, 0, m->m_pkthdr.len,
	    BUS_DMASYNC_PREWRITE);
	M_SETCTX(m, map);
	return 0;
}

static void
bcmeth_txq_map_unload(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq,
	struct mbuf *m)
{
	KASSERT(m);
	bus_dmamap_t map = M_GETCTX(m, bus_dmamap_t);
	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, map);
	bcmeth_mapcache_put(sc, sc->sc_tx_mapcache, map);
}

static bool
bcmeth_txq_produce(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq,
	struct mbuf *m)
{
	bus_dmamap_t map = M_GETCTX(m, bus_dmamap_t);

	if (map->dm_nsegs > txq->txq_free)
		return false;

	/*
	 * TCP Offload flag must be set in the first descriptor.
	 */
	struct gmac_txdb *producer = txq->txq_producer;
	uint32_t first_flags = TXDB_FLAG_SF;
	uint32_t last_flags = TXDB_FLAG_EF;

	/*
	 * If we've produced enough descriptors without consuming any
	 * we need to ask for an interrupt to reclaim some.
	 */
	txq->txq_lastintr += map->dm_nsegs;
	if (txq->txq_lastintr >= txq->txq_threshold
	    || txq->txq_mbufs.ifq_len + 1 == txq->txq_mbufs.ifq_maxlen) {
		txq->txq_lastintr = 0;
		last_flags |= TXDB_FLAG_IC;
	}

	KASSERT(producer != txq->txq_last);

	struct gmac_txdb *start = producer;
	size_t count = map->dm_nsegs;
	producer->txdb_flags |= first_flags;
	producer->txdb_addrlo = map->dm_segs[0].ds_addr;
	producer->txdb_buflen = map->dm_segs[0].ds_len;
	for (u_int i = 1; i < map->dm_nsegs; i++) {
#if 0
		printf("[%zu]: %#x/%#x/%#x/%#x\n", producer - txq->txq_first,
		     producer->txdb_flags, producer->txdb_buflen,
		     producer->txdb_addrlo, producer->txdb_addrhi);
#endif
		if (__predict_false(++producer == txq->txq_last)) {
			bcmeth_txq_desc_presync(sc, txq, start,
			    txq->txq_last - start);
			count -= txq->txq_last - start;
			producer = txq->txq_first;
			start = txq->txq_first;
		}
		producer->txdb_addrlo = map->dm_segs[i].ds_addr;
		producer->txdb_buflen = map->dm_segs[i].ds_len;
	}
	producer->txdb_flags |= last_flags;
#if 0
	printf("[%zu]: %#x/%#x/%#x/%#x\n", producer - txq->txq_first,
	     producer->txdb_flags, producer->txdb_buflen,
	     producer->txdb_addrlo, producer->txdb_addrhi);
#endif
	if (count)
		bcmeth_txq_desc_presync(sc, txq, start, count);

	/*
	 * Reduce free count by the number of segments we consumed.
	 */
	txq->txq_free -= map->dm_nsegs;
	KASSERT(map->dm_nsegs == 1 || txq->txq_producer != producer);
	KASSERT(map->dm_nsegs == 1 || (txq->txq_producer->txdb_flags & TXDB_FLAG_EF) == 0);
	KASSERT(producer->txdb_flags & TXDB_FLAG_EF);

#if 0
	printf("%s: mbuf %p: produced a %u byte packet in %u segments (%zd..%zd)\n",
	    __func__, m, m->m_pkthdr.len, map->dm_nsegs,
	    txq->txq_producer - txq->txq_first, producer - txq->txq_first);
#endif

	if (producer + 1 == txq->txq_last)
		txq->txq_producer = txq->txq_first;
	else
		txq->txq_producer = producer + 1;
	IF_ENQUEUE(&txq->txq_mbufs, m);

	/*
	 * Let the transmitter know there's more to do
	 */
	bcmeth_write_4(sc, txq->txq_reg_xmtptr,
	    txq->txq_descmap->dm_segs[0].ds_addr
	    + ((uintptr_t)txq->txq_producer & XMT_LASTDSCR));

	return true;
}

static struct mbuf *
bcmeth_copy_packet(struct mbuf *m)
{
	struct mbuf *mext = NULL;
	size_t misalignment = 0;
	size_t hlen = 0;

	for (mext = m; mext != NULL; mext = mext->m_next) {
		if (mext->m_flags & M_EXT) {
			misalignment = mtod(mext, vaddr_t) & arm_dcache_align;
			break;
		}
		hlen += m->m_len;
	}

	struct mbuf *n = m->m_next;
	if (m != mext && hlen + misalignment <= MHLEN && false) {
		KASSERT(m->m_pktdat <= m->m_data && m->m_data <= &m->m_pktdat[MHLEN - m->m_len]);
		size_t oldoff = m->m_data - m->m_pktdat;
		size_t off;
		if (mext == NULL) {
			off = (oldoff + hlen > MHLEN) ? 0 : oldoff;
		} else {
			off = MHLEN - (hlen + misalignment);
		}
		KASSERT(off + hlen + misalignment <= MHLEN);
		if (((oldoff ^ off) & arm_dcache_align) != 0 || off < oldoff) {
			memmove(&m->m_pktdat[off], m->m_data, m->m_len);
			m->m_data = &m->m_pktdat[off];
		}
		m_copydata(n, 0, hlen - m->m_len, &m->m_data[m->m_len]);
		m->m_len = hlen;
		m->m_next = mext;
		while (n != mext) {
			n = m_free(n);
		}
		return m;
	}

	struct mbuf *m0 = m_gethdr(M_DONTWAIT, m->m_type);
	if (m0 == NULL) {
		return NULL;
	}
	M_COPY_PKTHDR(m0, m);
	MCLAIM(m0, m->m_owner);
	if (m0->m_pkthdr.len > MHLEN) {
		MCLGET(m0, M_DONTWAIT);
		if ((m0->m_flags & M_EXT) == 0) {
			m_freem(m0);
			return NULL;
		}
	}
	m0->m_len = m->m_pkthdr.len;
	m_copydata(m, 0, m0->m_len, mtod(m0, void *));
	m_freem(m);
	return m0;
}

static bool
bcmeth_txq_enqueue(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq)
{
	for (;;) {
		if (IF_QFULL(&txq->txq_mbufs))
			return false;
		struct mbuf *m = txq->txq_next;
		if (m == NULL) {
			int s = splnet();
			IF_DEQUEUE(&sc->sc_if.if_snd, m);
			splx(s);
			if (m == NULL)
				return true;
			M_SETCTX(m, NULL);
		} else {
			txq->txq_next = NULL;
		}
		/*
		 * If LINK2 is set and this packet uses multiple mbufs,
		 * consolidate it into a single mbuf.
		 */
		if (m->m_next != NULL && (sc->sc_if.if_flags & IFF_LINK2)) {
			struct mbuf *m0 = bcmeth_copy_packet(m);
			if (m0 == NULL) {
				txq->txq_next = m;
				return true;
			}
			m = m0;
		}
		int error = bcmeth_txq_map_load(sc, txq, m);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "discarded packet due to "
			    "dmamap load failure: %d\n", error);
			m_freem(m);
			continue;
		}
		KASSERT(txq->txq_next == NULL);
		if (!bcmeth_txq_produce(sc, txq, m)) {
			txq->txq_next = m;
			return false;
		}
		KASSERT(txq->txq_next == NULL);
	}
}

static bool
bcmeth_txq_consume(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq)
{
	struct ifnet * const ifp = &sc->sc_if;
	struct gmac_txdb *consumer = txq->txq_consumer;
	size_t txfree = 0;

#if 0
	printf("%s: entry: free=%zu\n", __func__, txq->txq_free);
#endif

	for (;;) {
		if (consumer == txq->txq_producer) {
			txq->txq_consumer = consumer;
			txq->txq_free += txfree;
			txq->txq_lastintr -= min(txq->txq_lastintr, txfree);
#if 0
			printf("%s: empty: freed %zu descriptors going from %zu to %zu\n",
			    __func__, txfree, txq->txq_free - txfree, txq->txq_free);
#endif
			KASSERT(txq->txq_lastintr == 0);
			KASSERT(txq->txq_free == txq->txq_last - txq->txq_first - 1);
			return true;
		}
		bcmeth_txq_desc_postsync(sc, txq, consumer, 1);
		uint32_t s0 = bcmeth_read_4(sc, txq->txq_reg_xmtsts0);
		if (consumer == txq->txq_first + __SHIFTOUT(s0, XMT_CURRDSCR)) {
			txq->txq_consumer = consumer;
			txq->txq_free += txfree;
			txq->txq_lastintr -= min(txq->txq_lastintr, txfree);
#if 0
			printf("%s: freed %zu descriptors\n",
			    __func__, txfree);
#endif
			return bcmeth_txq_fillable_p(sc, txq);
		}

		/*
		 * If this is the last descriptor in the chain, get the
		 * mbuf, free its dmamap, and free the mbuf chain itself.
		 */
		const uint32_t txdb_flags = consumer->txdb_flags;
		if (txdb_flags & TXDB_FLAG_EF) {
			struct mbuf *m;

			IF_DEQUEUE(&txq->txq_mbufs, m);
			KASSERT(m);
			bcmeth_txq_map_unload(sc, txq, m);
#if 0
			printf("%s: mbuf %p: consumed a %u byte packet\n",
			    __func__, m, m->m_pkthdr.len);
#endif
			bpf_mtap(ifp, m);
			ifp->if_opackets++;
			ifp->if_obytes += m->m_pkthdr.len;
			if (m->m_flags & M_MCAST)
				ifp->if_omcasts++;
			m_freem(m);
		}

		/*
		 * We own this packet again.  Clear all flags except wrap.
		 */
		txfree++;

		/*
		 * Wrap at the last entry!
		 */
		if (txdb_flags & TXDB_FLAG_ET) {
			consumer->txdb_flags = TXDB_FLAG_ET;
			KASSERT(consumer + 1 == txq->txq_last);
			consumer = txq->txq_first;
		} else {
			consumer->txdb_flags = 0;
			consumer++;
			KASSERT(consumer < txq->txq_last);
		}
	}
}

static void
bcmeth_txq_purge(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq)
{
	struct mbuf *m;
	KASSERT((bcmeth_read_4(sc, UNIMAC_COMMAND_CONFIG) & TX_ENA) == 0);

	for (;;) {
		IF_DEQUEUE(&txq->txq_mbufs, m);
		if (m == NULL)
			break;
		bcmeth_txq_map_unload(sc, txq, m);
		m_freem(m);
	}
	if ((m = txq->txq_next) != NULL) {
		txq->txq_next = NULL;
		bcmeth_txq_map_unload(sc, txq, m);
		m_freem(m);
	}
}

static void
bcmeth_txq_reset(
	struct bcmeth_softc *sc,
	struct bcmeth_txqueue *txq)
{
	/*
	 * sync all the descriptors
	 */
	bcmeth_txq_desc_postsync(sc, txq, txq->txq_first,
	    txq->txq_last - txq->txq_first);

	/*
	 * Make sure we own all descriptors in the ring.
	 */
	struct gmac_txdb *txdb;
	for (txdb = txq->txq_first; txdb < txq->txq_last - 1; txdb++) {
		txdb->txdb_flags = 0;
	}

	/*
	 * Last descriptor has the wrap flag.
	 */
	txdb->txdb_flags = TXDB_FLAG_ET;

	/*
	 * Reset the producer consumer indexes.
	 */
	txq->txq_consumer = txq->txq_first;
	txq->txq_producer = txq->txq_first;
	txq->txq_free = txq->txq_last - txq->txq_first - 1;
	txq->txq_threshold = txq->txq_free / 2;
	txq->txq_lastintr = 0;

	/*
	 * What do we want to get interrupted on?
	 */
	sc->sc_intmask |= XMTINT_0 | XMTUF;

	/*
	 * Restart the transmiter at the first descriptor
	 */
	bcmeth_write_4(sc, txq->txq_reg_xmtaddrlo,
	    txq->txq_descmap->dm_segs->ds_addr);
}

static void
bcmeth_ifstart(struct ifnet *ifp)
{
	struct bcmeth_softc * const sc = ifp->if_softc;

	if (__predict_false((ifp->if_flags & IFF_RUNNING) == 0)) {
		return;
	}

#ifdef BCMETH_MPSAFETX
	if (cpu_intr_p()) {
#endif
		atomic_or_uint(&sc->sc_soft_flags, SOFT_TXINTR);
		softint_schedule(sc->sc_soft_ih);
#ifdef BCMETH_MPSAFETX
	} else {
		/*
		 * Either we are in a softintr thread already or some other
		 * thread so just borrow it to do the send and save ourselves
		 * the overhead of a fast soft int.
		 */
		bcmeth_soft_txintr(sc);
	}
#endif
}

int
bcmeth_intr(void *arg)
{
	struct bcmeth_softc * const sc = arg;
	uint32_t soft_flags = 0;
	uint32_t work_flags = 0;
	int rv = 0;

	mutex_enter(sc->sc_hwlock);

	uint32_t intmask = sc->sc_intmask;
	BCMETH_EVCNT_INCR(sc->sc_ev_intr);

	for (;;) {
		uint32_t intstatus = bcmeth_read_4(sc, GMAC_INTSTATUS);
		intstatus &= intmask;
		bcmeth_write_4(sc, GMAC_INTSTATUS, intstatus);	/* write 1 to clear */
		if (intstatus == 0) {
			break;
		}
#if 0
		aprint_normal_dev(sc->sc_dev, "%s: intstatus=%#x intmask=%#x\n", 
		    __func__, intstatus, bcmeth_read_4(sc, GMAC_INTMASK));
#endif
		if (intstatus & RCVINT) {
			struct bcmeth_rxqueue * const rxq = &sc->sc_rxq;
			intmask &= ~RCVINT;

			uint32_t rcvsts0 = bcmeth_read_4(sc, rxq->rxq_reg_rcvsts0);
			uint32_t descs = __SHIFTOUT(rcvsts0, RCV_CURRDSCR);
			if (descs < rxq->rxq_consumer - rxq->rxq_first) {
				/*
				 * We wrapped at the end so count how far
				 * we are from the end.
				 */
				descs += rxq->rxq_last - rxq->rxq_consumer;
			} else {
				descs -= rxq->rxq_consumer - rxq->rxq_first;
			}
			/*
			 * If we "timedout" we can't be hogging so use
			 * softints.  If we exceeded then we might hogging
			 * so let the workqueue deal with them.
			 */
			const uint32_t framecount = __SHIFTOUT(sc->sc_rcvlazy, INTRCVLAZY_FRAMECOUNT);
			if (descs < framecount
			    || (curcpu()->ci_curlwp->l_flag & LW_IDLE)) {
				soft_flags |= SOFT_RXINTR;
			} else {
				work_flags |= WORK_RXINTR;
			}
		}

		if (intstatus & XMTINT_0) {
			intmask &= ~XMTINT_0;
			soft_flags |= SOFT_TXINTR;
		}

		if (intstatus & RCVDESCUF) {
			intmask &= ~RCVDESCUF;
			work_flags |= WORK_RXUNDERFLOW;
		}

		intstatus &= intmask;
		if (intstatus) {
			aprint_error_dev(sc->sc_dev,
			    "intr: intstatus=%#x\n", intstatus);
			aprint_error_dev(sc->sc_dev,
			    "rcvbase=%p/%#lx rcvptr=%#x rcvsts=%#x/%#x\n",
			    sc->sc_rxq.rxq_first,
			    sc->sc_rxq.rxq_descmap->dm_segs[0].ds_addr,
			    bcmeth_read_4(sc, sc->sc_rxq.rxq_reg_rcvptr),
			    bcmeth_read_4(sc, sc->sc_rxq.rxq_reg_rcvsts0),
			    bcmeth_read_4(sc, sc->sc_rxq.rxq_reg_rcvsts1));
			aprint_error_dev(sc->sc_dev,
			    "xmtbase=%p/%#lx xmtptr=%#x xmtsts=%#x/%#x\n",
			    sc->sc_txq.txq_first,
			    sc->sc_txq.txq_descmap->dm_segs[0].ds_addr,
			    bcmeth_read_4(sc, sc->sc_txq.txq_reg_xmtptr),
			    bcmeth_read_4(sc, sc->sc_txq.txq_reg_xmtsts0),
			    bcmeth_read_4(sc, sc->sc_txq.txq_reg_xmtsts1));
			intmask &= ~intstatus;
			work_flags |= WORK_REINIT;
			break;
		}
	}

	if (intmask != sc->sc_intmask) {
		bcmeth_write_4(sc, GMAC_INTMASK, sc->sc_intmask);
	}

	if (work_flags) {
		if (sc->sc_work_flags == 0) {
			workqueue_enqueue(sc->sc_workq, &sc->sc_work, NULL);
		}
		atomic_or_32(&sc->sc_work_flags, work_flags);
		rv = 1;
	}

	if (soft_flags) {
		if (sc->sc_soft_flags == 0) {
			softint_schedule(sc->sc_soft_ih);
		}
		atomic_or_32(&sc->sc_soft_flags, soft_flags);
		rv = 1;
	}

	mutex_exit(sc->sc_hwlock);

	return rv;
}

#ifdef BCMETH_MPSAFETX
void
bcmeth_soft_txintr(struct bcmeth_softc *sc)
{
	mutex_enter(sc->sc_lock);
	/*
	 * Let's do what we came here for.  Consume transmitted
	 * packets off the the transmit ring.
	 */
	if (!bcmeth_txq_consume(sc, &sc->sc_txq)
	    || !bcmeth_txq_enqueue(sc, &sc->sc_txq)) {
		BCMETH_EVCNT_INCR(sc->sc_ev_tx_stall);
		sc->sc_if.if_flags |= IFF_OACTIVE;
	} else {
		sc->sc_if.if_flags &= ~IFF_OACTIVE;
	}
	if (sc->sc_if.if_flags & IFF_RUNNING) {
		mutex_spin_enter(sc->sc_hwlock);
		sc->sc_intmask |= XMTINT_0;
		bcmeth_write_4(sc, GMAC_INTMASK, sc->sc_intmask);
		mutex_spin_exit(sc->sc_hwlock);
	}
	mutex_exit(sc->sc_lock);
}
#endif /* BCMETH_MPSAFETX */

void
bcmeth_soft_intr(void *arg)
{
	struct bcmeth_softc * const sc = arg;
	struct ifnet * const ifp = &sc->sc_if;
	uint32_t intmask = 0;

	mutex_enter(sc->sc_lock);

	u_int soft_flags = atomic_swap_uint(&sc->sc_soft_flags, 0);

	BCMETH_EVCNT_INCR(sc->sc_ev_soft_intr);

	if ((soft_flags & SOFT_TXINTR)
	    || bcmeth_txq_active_p(sc, &sc->sc_txq)) {
		/*
		 * Let's do what we came here for.  Consume transmitted
		 * packets off the the transmit ring.
		 */
		if (!bcmeth_txq_consume(sc, &sc->sc_txq)
		    || !bcmeth_txq_enqueue(sc, &sc->sc_txq)) {
			BCMETH_EVCNT_INCR(sc->sc_ev_tx_stall);
			ifp->if_flags |= IFF_OACTIVE;
		} else {
			ifp->if_flags &= ~IFF_OACTIVE;
		}
		intmask |= XMTINT_0;
	}

	if (soft_flags & SOFT_RXINTR) {
		/*
		 * Let's consume 
		 */
		bcmeth_rxq_consume(sc, &sc->sc_rxq);
		intmask |= RCVINT;
	}

	if (ifp->if_flags & IFF_RUNNING) {
		bcmeth_rxq_produce(sc, &sc->sc_rxq);
		mutex_spin_enter(sc->sc_hwlock);
		sc->sc_intmask |= intmask;
		bcmeth_write_4(sc, GMAC_INTMASK, sc->sc_intmask);
		mutex_spin_exit(sc->sc_hwlock);
	}

	mutex_exit(sc->sc_lock);
}

void
bcmeth_worker(struct work *wk, void *arg)
{
	struct bcmeth_softc * const sc = arg;
	struct ifnet * const ifp = &sc->sc_if;
	uint32_t intmask = 0;

	mutex_enter(sc->sc_lock);

	BCMETH_EVCNT_INCR(sc->sc_ev_work);

	uint32_t work_flags = atomic_swap_32(&sc->sc_work_flags, 0);
	if (work_flags & WORK_REINIT) {
		int s = splnet();
		sc->sc_soft_flags = 0;
		bcmeth_ifinit(ifp);
		splx(s);
		work_flags &= ~WORK_RXUNDERFLOW;
	}

	if (work_flags & WORK_RXUNDERFLOW) {
		struct bcmeth_rxqueue * const rxq = &sc->sc_rxq;
		size_t threshold = 5 * rxq->rxq_threshold / 4;
		if (threshold >= rxq->rxq_last - rxq->rxq_first) {
			threshold = rxq->rxq_last - rxq->rxq_first - 1;
		} else {
			intmask |= RCVDESCUF;
		}
		aprint_normal_dev(sc->sc_dev,
		    "increasing receive buffers from %zu to %zu\n",
		    rxq->rxq_threshold, threshold);
		rxq->rxq_threshold = threshold;
	}

	if (work_flags & WORK_RXINTR) {
		/*
		 * Let's consume 
		 */
		bcmeth_rxq_consume(sc, &sc->sc_rxq);
		intmask |= RCVINT;
	}

	if (ifp->if_flags & IFF_RUNNING) {
		bcmeth_rxq_produce(sc, &sc->sc_rxq);
#if 0
		uint32_t intstatus = bcmeth_read_4(sc, GMAC_INTSTATUS);
		if (intstatus & RCVINT) {
			bcmeth_write_4(sc, GMAC_INTSTATUS, RCVINT);
			work_flags |= WORK_RXINTR;
			continue;
		}
#endif
		mutex_spin_enter(sc->sc_hwlock);
		sc->sc_intmask |= intmask;
		bcmeth_write_4(sc, GMAC_INTMASK, sc->sc_intmask);
		mutex_spin_exit(sc->sc_hwlock);
	}

	mutex_exit(sc->sc_lock);
}
