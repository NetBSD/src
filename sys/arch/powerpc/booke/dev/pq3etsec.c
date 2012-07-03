/*	$NetBSD: pq3etsec.c,v 1.9.8.2 2012/07/03 21:16:15 jdc Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include "opt_inet.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pq3etsec.c,v 1.9.8.2 2012/07/03 21:16:15 jdc Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/atomic.h>
#include <sys/callout.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include "ioconf.h"

#include <net/bpf.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_offload.h>
#endif /* INET */
#ifdef INET6
#include <netinet6/in6.h>
#include <netinet/ip6.h>
#endif
#include <netinet6/in6_offload.h>


#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/etsecreg.h>

#define	M_HASFCB		M_LINK2	/* tx packet has FCB prepended */

#define	ETSEC_MAXTXMBUFS	30
#define	ETSEC_NTXSEGS		30
#define	ETSEC_MAXRXMBUFS	511
#define	ETSEC_MINRXMBUFS	32
#define	ETSEC_NRXSEGS		1

#define	IFCAP_RCTRL_IPCSEN	IFCAP_CSUM_IPv4_Rx
#define	IFCAP_RCTRL_TUCSEN	(IFCAP_CSUM_TCPv4_Rx\
				 |IFCAP_CSUM_UDPv4_Rx\
				 |IFCAP_CSUM_TCPv6_Rx\
				 |IFCAP_CSUM_UDPv6_Rx)

#define	IFCAP_TCTRL_IPCSEN	IFCAP_CSUM_IPv4_Tx
#define	IFCAP_TCTRL_TUCSEN	(IFCAP_CSUM_TCPv4_Tx\
				 |IFCAP_CSUM_UDPv4_Tx\
				 |IFCAP_CSUM_TCPv6_Tx\
				 |IFCAP_CSUM_UDPv6_Tx)

#define	IFCAP_ETSEC		(IFCAP_RCTRL_IPCSEN|IFCAP_RCTRL_TUCSEN\
				 |IFCAP_TCTRL_IPCSEN|IFCAP_TCTRL_TUCSEN)

#define	M_CSUM_IP	(M_CSUM_CIP|M_CSUM_CTU)
#define	M_CSUM_IP6	(M_CSUM_TCPv6|M_CSUM_UDPv6)
#define	M_CSUM_TUP	(M_CSUM_TCPv4|M_CSUM_UDPv4|M_CSUM_TCPv6|M_CSUM_UDPv6)
#define	M_CSUM_UDP	(M_CSUM_UDPv4|M_CSUM_UDPv6)
#define	M_CSUM_IP4	(M_CSUM_IPv4|M_CSUM_UDPv4|M_CSUM_TCPv4)
#define	M_CSUM_CIP	(M_CSUM_IPv4)
#define	M_CSUM_CTU	(M_CSUM_TCPv4|M_CSUM_UDPv4|M_CSUM_TCPv6|M_CSUM_UDPv6)

struct pq3etsec_txqueue {
	bus_dmamap_t txq_descmap;
	volatile struct txbd *txq_consumer;
	volatile struct txbd *txq_producer;
	volatile struct txbd *txq_first;
	volatile struct txbd *txq_last;
	struct ifqueue txq_mbufs;
	struct mbuf *txq_next;
#ifdef ETSEC_DEBUG
	struct mbuf *txq_lmbufs[512];
#endif
	uint32_t txq_qmask;
	uint32_t txq_free;
	uint32_t txq_threshold;
	uint32_t txq_lastintr;
	bus_size_t txq_reg_tbase;
	bus_dma_segment_t txq_descmap_seg;
};

struct pq3etsec_rxqueue {
	bus_dmamap_t rxq_descmap;
	volatile struct rxbd *rxq_consumer;
	volatile struct rxbd *rxq_producer;
	volatile struct rxbd *rxq_first;
	volatile struct rxbd *rxq_last;
	struct mbuf *rxq_mhead;
	struct mbuf **rxq_mtail;
	struct mbuf *rxq_mconsumer;
#ifdef ETSEC_DEBUG
	struct mbuf *rxq_mbufs[512];
#endif
	uint32_t rxq_qmask;
	uint32_t rxq_inuse;
	uint32_t rxq_threshold;
	bus_size_t rxq_reg_rbase;
	bus_size_t rxq_reg_rbptr;
	bus_dma_segment_t rxq_descmap_seg;
};

struct pq3etsec_mapcache {
	u_int dmc_nmaps;
	u_int dmc_maxseg;
	u_int dmc_maxmaps;
	u_int dmc_maxmapsize;
	bus_dmamap_t dmc_maps[0];
};

struct pq3etsec_softc {
	device_t sc_dev;
	struct ethercom sc_ec;
#define sc_if		sc_ec.ec_if
	struct mii_data sc_mii;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	int sc_phy_addr;
	prop_dictionary_t sc_intrmap;
	uint32_t sc_intrmask;

	uint32_t sc_soft_flags;
#define	SOFT_RESET		0x0001
#define	SOFT_RXINTR		0x0010
#define	SOFT_RXBSY		0x0020
#define	SOFT_TXINTR		0x0100
#define	SOFT_TXERROR		0x0200

	struct pq3etsec_txqueue sc_txq;
	struct pq3etsec_rxqueue sc_rxq;
	uint32_t sc_txerrors;
	uint32_t sc_rxerrors;

	size_t sc_rx_adjlen;

	/*
	 * Copies of various ETSEC registers.
	 */
	uint32_t sc_imask;
	uint32_t sc_maccfg1;
	uint32_t sc_maccfg2;
	uint32_t sc_maxfrm;
	uint32_t sc_ecntrl;
	uint32_t sc_dmactrl;
	uint32_t sc_macstnaddr1;
	uint32_t sc_macstnaddr2;
	uint32_t sc_tctrl;
	uint32_t sc_rctrl;
	uint32_t sc_gaddr[16];
	uint64_t sc_macaddrs[15];

	void *sc_tx_ih;
	void *sc_rx_ih;
	void *sc_error_ih;
	void *sc_soft_ih;

	kmutex_t *sc_lock;

	struct evcnt sc_ev_tx_stall;
	struct evcnt sc_ev_tx_intr;
	struct evcnt sc_ev_rx_stall;
	struct evcnt sc_ev_rx_intr;
	struct evcnt sc_ev_error_intr;
	struct evcnt sc_ev_soft_intr;
	struct evcnt sc_ev_tx_pause;
	struct evcnt sc_ev_rx_pause;
	struct evcnt sc_ev_mii_ticks;

	struct callout sc_mii_callout;
	uint64_t sc_mii_last_tick;

	struct ifqueue sc_rx_bufcache;
	struct pq3etsec_mapcache *sc_rx_mapcache; 
	struct pq3etsec_mapcache *sc_tx_mapcache; 
};

static int pq3etsec_match(device_t, cfdata_t, void *);
static void pq3etsec_attach(device_t, device_t, void *);

static void pq3etsec_ifstart(struct ifnet *);
static void pq3etsec_ifwatchdog(struct ifnet *);
static int pq3etsec_ifinit(struct ifnet *);
static void pq3etsec_ifstop(struct ifnet *, int);
static int pq3etsec_ifioctl(struct ifnet *, u_long, void *);

static int pq3etsec_mapcache_create(struct pq3etsec_softc *,
    struct pq3etsec_mapcache **, size_t, size_t, size_t);
static void pq3etsec_mapcache_destroy(struct pq3etsec_softc *,
    struct pq3etsec_mapcache *);
static bus_dmamap_t pq3etsec_mapcache_get(struct pq3etsec_softc *,
    struct pq3etsec_mapcache *);
static void pq3etsec_mapcache_put(struct pq3etsec_softc *,
    struct pq3etsec_mapcache *, bus_dmamap_t);

static int pq3etsec_txq_attach(struct pq3etsec_softc *,
    struct pq3etsec_txqueue *, u_int);
static void pq3etsec_txq_purge(struct pq3etsec_softc *,
    struct pq3etsec_txqueue *);
static void pq3etsec_txq_reset(struct pq3etsec_softc *,
    struct pq3etsec_txqueue *);
static bool pq3etsec_txq_consume(struct pq3etsec_softc *,
    struct pq3etsec_txqueue *);
static bool pq3etsec_txq_produce(struct pq3etsec_softc *,
    struct pq3etsec_txqueue *, struct mbuf *m);
static bool pq3etsec_txq_active_p(struct pq3etsec_softc *,
    struct pq3etsec_txqueue *);

static int pq3etsec_rxq_attach(struct pq3etsec_softc *,
    struct pq3etsec_rxqueue *, u_int);
static bool pq3etsec_rxq_produce(struct pq3etsec_softc *,
    struct pq3etsec_rxqueue *);
static void pq3etsec_rxq_purge(struct pq3etsec_softc *,
    struct pq3etsec_rxqueue *, bool);
static void pq3etsec_rxq_reset(struct pq3etsec_softc *,
    struct pq3etsec_rxqueue *);

static void pq3etsec_mc_setup(struct pq3etsec_softc *);

static void pq3etsec_mii_tick(void *);
static int pq3etsec_rx_intr(void *);
static int pq3etsec_tx_intr(void *);
static int pq3etsec_error_intr(void *);
static void pq3etsec_soft_intr(void *);

CFATTACH_DECL_NEW(pq3etsec, sizeof(struct pq3etsec_softc),
    pq3etsec_match, pq3etsec_attach, NULL, NULL);

static int
pq3etsec_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!e500_cpunode_submatch(parent, cf, cf->cf_name, aux))
		return 0;

	return 1;
}

static inline uint32_t
etsec_read(struct pq3etsec_softc *sc, bus_size_t off)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, off);
}

static inline void
etsec_write(struct pq3etsec_softc *sc, bus_size_t off, uint32_t data)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, off, data);
}

static int
pq3etsec_mii_readreg(device_t self, int phy, int reg)
{
	struct pq3etsec_softc * const sc = device_private(self);
	uint32_t miimcom = etsec_read(sc, MIIMCOM);

//	int s = splnet();

	etsec_write(sc, MIIMADD,
	    __SHIFTIN(phy, MIIMADD_PHY) | __SHIFTIN(reg, MIIMADD_REG));

	etsec_write(sc, IEVENT, IEVENT_MMRD);
	etsec_write(sc, MIIMCOM, 0);	/* clear any past bits */
	etsec_write(sc, MIIMCOM, MIIMCOM_READ);
#if 0
	sc->sc_imask |= IEVENT_MMRD;
	etsec_write(sc, IMASK, sc->sc_imask);
#endif

	while (etsec_read(sc, MIIMIND) != 0) {
			delay(1);
	}
	int data = etsec_read(sc, MIIMSTAT);

	if (miimcom == MIIMCOM_SCAN)
		etsec_write(sc, MIIMCOM, miimcom);

#if 0
	aprint_normal_dev(sc->sc_dev, "%s: phy %d reg %d: %#x\n",
	    __func__, phy, reg, data);
#endif
	etsec_write(sc, IEVENT, IEVENT_MMRD);
//	splx(s);
	return data;
}

static void
pq3etsec_mii_writereg(device_t self, int phy, int reg, int data)
{
	struct pq3etsec_softc * const sc = device_private(self);
	uint32_t miimcom = etsec_read(sc, MIIMCOM);

#if 0
	aprint_normal_dev(sc->sc_dev, "%s: phy %d reg %d: %#x\n",
	    __func__, phy, reg, data);
#endif

//	int s = splnet();
	etsec_write(sc, IEVENT, IEVENT_MMWR);
	etsec_write(sc, MIIMADD,
	    __SHIFTIN(phy, MIIMADD_PHY) | __SHIFTIN(reg, MIIMADD_REG));
	etsec_write(sc, MIIMCOM, 0);	/* clear any past bits */
	etsec_write(sc, MIIMCON, data);

#if 0
	sc->sc_imask |= IEVENT_MMWR;
	etsec_write(sc, IMASK, sc->sc_imask);
#endif

	int timo = 1000;	/* 1ms */
	while ((etsec_read(sc, MIIMIND) & MIIMIND_BUSY) && --timo > 0) {
			delay(1);
	}

	if (miimcom == MIIMCOM_SCAN)
		etsec_write(sc, MIIMCOM, miimcom);
	etsec_write(sc, IEVENT, IEVENT_MMWR);
//	splx(s);
}

static void
pq3etsec_mii_statchg(device_t self)
{
	struct pq3etsec_softc * const sc = device_private(self);
	struct mii_data * const mii = &sc->sc_mii;

	uint32_t maccfg1 = sc->sc_maccfg1;
	uint32_t maccfg2 = sc->sc_maccfg2;
	uint32_t ecntrl = sc->sc_ecntrl;

	maccfg1 &= ~(MACCFG1_TX_FLOW|MACCFG1_RX_FLOW);
	maccfg2 &= ~(MACCFG2_IFMODE|MACCFG2_FD);

	if (sc->sc_mii.mii_media_active & IFM_FDX) {
		maccfg2 |= MACCFG2_FD;
	}

	/*
	 * Now deal with the flow control bits.
	 */
	if (IFM_SUBTYPE(mii->mii_media.ifm_cur->ifm_media) == IFM_AUTO
	    && (mii->mii_media_active & IFM_ETH_FMASK)) {
		if (mii->mii_media_active & IFM_ETH_RXPAUSE)
			maccfg1 |= MACCFG1_RX_FLOW;
		if (mii->mii_media_active & IFM_ETH_TXPAUSE)
			maccfg1 |= MACCFG1_TX_FLOW;
	}

	/*
	 * Now deal with the speed.
	 */
	if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) {
		maccfg2 |= MACCFG2_IFMODE_GMII;
	} else {
		maccfg2 |= MACCFG2_IFMODE_MII;
		ecntrl &= ~ECNTRL_R100M;
		if (IFM_SUBTYPE(mii->mii_media_active) != IFM_10_T) {
			ecntrl |= ECNTRL_R100M;
		}
	}

	/*
	 * If things are different, re-init things.
	 */
	if (maccfg1 != sc->sc_maccfg1
	    || maccfg2 != sc->sc_maccfg2
	    || ecntrl != sc->sc_ecntrl) {
		if (sc->sc_if.if_flags & IFF_RUNNING)
			atomic_or_uint(&sc->sc_soft_flags, SOFT_RESET);
		sc->sc_maccfg1 = maccfg1;
		sc->sc_maccfg2 = maccfg2;
		sc->sc_ecntrl = ecntrl;
	}
}

#if 0
static void
pq3etsec_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct pq3etsec_softc * const sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);
	ether_mediastatus(ifp, ifmr);
        ifmr->ifm_status = sc->sc_mii.mii_media_status;
        ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

static int
pq3etsec_mediachange(struct ifnet *ifp)
{
	struct pq3etsec_softc * const sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_UP) == 0)
		return 0;

	int rv = mii_mediachg(&sc->sc_mii);
	return (rv == ENXIO) ? 0 : rv;
}
#endif

static void
pq3etsec_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct pq3etsec_softc * const sc = device_private(self);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;
	cfdata_t cf = device_cfdata(self);
	int error;

	psc->sc_children |= cna->cna_childmask;
	sc->sc_dev = self;
	sc->sc_bst = cna->cna_memt;
	sc->sc_dmat = &booke_bus_dma_tag;

	/*
	 * If we have a common MDIO bus, if all off instance 1.
	 */
	device_t miiself = (cf->cf_flags & 0x100) ? tsec_cd.cd_devs[0] : self;

	/*
	 * See if the phy is in the config file...
	 */
	if (cf->cf_flags & 0x3f) {
		sc->sc_phy_addr = (cf->cf_flags & 0x3f) - 1;
	} else {
		unsigned char prop_name[20];
		snprintf(prop_name, sizeof(prop_name), "tsec%u-phy-addr",
		    cnl->cnl_instance);
		sc->sc_phy_addr = board_info_get_number(prop_name);
	}
	if (sc->sc_phy_addr != MII_PHY_ANY)
		aprint_normal(" phy %d", sc->sc_phy_addr);

	error = bus_space_map(sc->sc_bst, cnl->cnl_addr, cnl->cnl_size, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error(": error mapping registers: %d\n", error);
		return;
	}

	/*
	 * Assume firmware has aready set the mac address and fetch it
	 * before we reinit it.
	 */
	sc->sc_macstnaddr2 = etsec_read(sc, MACSTNADDR2);
	sc->sc_macstnaddr1 = etsec_read(sc, MACSTNADDR1);
	sc->sc_rctrl = RCTRL_DEFAULT;
	sc->sc_ecntrl = etsec_read(sc, ECNTRL);
	sc->sc_maccfg1 = etsec_read(sc, MACCFG1);
	sc->sc_maccfg2 = etsec_read(sc, MACCFG2) | MACCFG2_DEFAULT;

	if (sc->sc_macstnaddr1 == 0 && sc->sc_macstnaddr2 == 0) {
		size_t len;
		const uint8_t *mac_addr =
		    board_info_get_data("tsec-mac-addr-base", &len);
		KASSERT(len == ETHER_ADDR_LEN);
		sc->sc_macstnaddr2 =
		    (mac_addr[1] << 24)
		    | (mac_addr[0] << 16);
		sc->sc_macstnaddr1 =
		    ((mac_addr[5] + cnl->cnl_instance - 1) << 24)
		    | (mac_addr[4] << 16)
		    | (mac_addr[3] << 8)
		    | (mac_addr[2] << 0);
#if 0
		aprint_error(": mac-address unknown\n");
		return;
#endif
	}

	char enaddr[ETHER_ADDR_LEN] = {
	    [0] = sc->sc_macstnaddr2 >> 16,
	    [1] = sc->sc_macstnaddr2 >> 24,
	    [2] = sc->sc_macstnaddr1 >>  0,
	    [3] = sc->sc_macstnaddr1 >>  8,
	    [4] = sc->sc_macstnaddr1 >> 16,
	    [5] = sc->sc_macstnaddr1 >> 24,
	};

	error = pq3etsec_rxq_attach(sc, &sc->sc_rxq, 0);
	if (error) {
		aprint_error(": failed to init rxq: %d\n", error);
		return;
	}

	error = pq3etsec_txq_attach(sc, &sc->sc_txq, 0);
	if (error) {
		aprint_error(": failed to init txq: %d\n", error);
		return;
	}

	error = pq3etsec_mapcache_create(sc, &sc->sc_rx_mapcache, 
	    ETSEC_MAXRXMBUFS, MCLBYTES, ETSEC_NRXSEGS);
	if (error) {
		aprint_error(": failed to allocate rx dmamaps: %d\n", error);
		return;
	}

	error = pq3etsec_mapcache_create(sc, &sc->sc_tx_mapcache, 
	    ETSEC_MAXTXMBUFS, MCLBYTES, ETSEC_NTXSEGS);
	if (error) {
		aprint_error(": failed to allocate tx dmamaps: %d\n", error);
		return;
	}

	sc->sc_tx_ih = intr_establish(cnl->cnl_intrs[0], IPL_VM, IST_ONCHIP,
	    pq3etsec_tx_intr, sc);
	if (sc->sc_tx_ih == NULL) {
		aprint_error(": failed to establish tx interrupt: %d\n",
		    cnl->cnl_intrs[0]);
		return;
	}

	sc->sc_rx_ih = intr_establish(cnl->cnl_intrs[1], IPL_VM, IST_ONCHIP,
	    pq3etsec_rx_intr, sc);
	if (sc->sc_rx_ih == NULL) {
		aprint_error(": failed to establish rx interrupt: %d\n",
		    cnl->cnl_intrs[1]);
		return;
	}

	sc->sc_error_ih = intr_establish(cnl->cnl_intrs[2], IPL_VM, IST_ONCHIP,
	    pq3etsec_error_intr, sc);
	if (sc->sc_error_ih == NULL) {
		aprint_error(": failed to establish error interrupt: %d\n",
		    cnl->cnl_intrs[2]);
		return;
	}

	sc->sc_soft_ih = softint_establish(SOFTINT_NET|SOFTINT_MPSAFE,
	    pq3etsec_soft_intr, sc);
	if (sc->sc_soft_ih == NULL) {
		aprint_error(": failed to establish soft interrupt\n");
		return;
	}

	aprint_normal("\n");

	etsec_write(sc, ATTR, ATTR_DEFAULT);
	etsec_write(sc, ATTRELI, ATTRELI_DEFAULT);

	sc->sc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTNET);

	callout_init(&sc->sc_mii_callout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_mii_callout, pq3etsec_mii_tick, sc);

	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	   ether_sprintf(enaddr));

	const char * const xname = device_xname(sc->sc_dev);
	struct ethercom * const ec = &sc->sc_ec;
	struct ifnet * const ifp = &ec->ec_if;

	ec->ec_mii = &sc->sc_mii;

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = pq3etsec_mii_readreg;
	sc->sc_mii.mii_writereg = pq3etsec_mii_writereg;
	sc->sc_mii.mii_statchg = pq3etsec_mii_statchg;

	ifmedia_init(&sc->sc_mii.mii_media, 0, ether_mediachange,
	    ether_mediastatus);

	if (sc->sc_phy_addr < 32) {
		mii_attach(miiself, &sc->sc_mii, 0xffffffff,
		    sc->sc_phy_addr, MII_OFFSET_ANY, MIIF_DOPAUSE);

		if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
			ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
			ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
		} else {
			callout_schedule(&sc->sc_mii_callout, hz);
			ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
		}
	} else {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX);
	}

	ec->ec_capabilities = ETHERCAP_VLAN_MTU | ETHERCAP_VLAN_HWTAGGING
	    | ETHERCAP_JUMBO_MTU;

	strlcpy(ifp->if_xname, xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_capabilities = IFCAP_ETSEC;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = pq3etsec_ifioctl;
	ifp->if_start = pq3etsec_ifstart;
	ifp->if_watchdog = pq3etsec_ifwatchdog;
	ifp->if_init = pq3etsec_ifinit;
	ifp->if_stop = pq3etsec_ifstop;
	IFQ_SET_READY(&ifp->if_snd);

	pq3etsec_ifstop(ifp, true);

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	evcnt_attach_dynamic(&sc->sc_ev_rx_stall, EVCNT_TYPE_MISC,
	    NULL, xname, "rx stall");
	evcnt_attach_dynamic(&sc->sc_ev_tx_stall, EVCNT_TYPE_MISC,
	    NULL, xname, "tx stall");
	evcnt_attach_dynamic(&sc->sc_ev_tx_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "tx intr");
	evcnt_attach_dynamic(&sc->sc_ev_rx_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "rx intr");
	evcnt_attach_dynamic(&sc->sc_ev_error_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "error intr");
	evcnt_attach_dynamic(&sc->sc_ev_soft_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "soft intr");
	evcnt_attach_dynamic(&sc->sc_ev_tx_pause, EVCNT_TYPE_MISC,
	    NULL, xname, "tx pause");
	evcnt_attach_dynamic(&sc->sc_ev_rx_pause, EVCNT_TYPE_MISC,
	    NULL, xname, "rx pause");
	evcnt_attach_dynamic(&sc->sc_ev_mii_ticks, EVCNT_TYPE_MISC,
	    NULL, xname, "mii ticks");
}

static uint64_t
pq3etsec_macaddr_create(const uint8_t *lladdr)
{
	uint64_t macaddr = 0;

	lladdr += ETHER_ADDR_LEN;
	for (u_int i = ETHER_ADDR_LEN; i-- > 0; ) {
		macaddr = (macaddr << 8) | *--lladdr;
	}
	return macaddr << 16;
}

static int
pq3etsec_ifinit(struct ifnet *ifp)
{
	struct pq3etsec_softc * const sc = ifp->if_softc;
	int error = 0;

	KASSERT(!cpu_softintr_p());

	sc->sc_maxfrm = max(ifp->if_mtu + 32, MCLBYTES);
	if (ifp->if_mtu > ETHERMTU_JUMBO)
		return error;

	KASSERT(ifp->if_flags & IFF_UP);

	/*
	 * Stop the interface (steps 1 to 4 in the Soft Reset and
	 * Reconfigurating Procedure.
	 */
	pq3etsec_ifstop(ifp, 0);

	/*
	 * If our frame size has changed (or it's our first time through)
	 * destroy the existing transmit mapcache.
	 */
	if (sc->sc_tx_mapcache != NULL
	    && sc->sc_maxfrm != sc->sc_tx_mapcache->dmc_maxmapsize) {
		pq3etsec_mapcache_destroy(sc, sc->sc_tx_mapcache);
		sc->sc_tx_mapcache = NULL;
	}

	if (sc->sc_tx_mapcache == NULL) {
		error = pq3etsec_mapcache_create(sc, &sc->sc_tx_mapcache,
		    ETSEC_MAXTXMBUFS, sc->sc_maxfrm, ETSEC_NTXSEGS);
		if (error)
			return error;
	}

	sc->sc_ev_mii_ticks.ev_count++;
	mii_tick(&sc->sc_mii);

	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_rctrl |= RCTRL_PROM;
	} else {
		sc->sc_rctrl &= ~RCTRL_PROM;
	}

	uint32_t rctrl_prsdep = 0;
	sc->sc_rctrl &= ~(RCTRL_IPCSEN|RCTRL_TUCSEN|RCTRL_VLEX|RCTRL_PRSDEP);
	if (VLAN_ATTACHED(&sc->sc_ec)) {
		sc->sc_rctrl |= RCTRL_VLEX;
		rctrl_prsdep = RCTRL_PRSDEP_L2;
	}
	if (ifp->if_capenable & IFCAP_RCTRL_IPCSEN) {
		sc->sc_rctrl |= RCTRL_IPCSEN;
		rctrl_prsdep = RCTRL_PRSDEP_L3;
	}
	if (ifp->if_capenable & IFCAP_RCTRL_TUCSEN) {
		sc->sc_rctrl |= RCTRL_TUCSEN;
		rctrl_prsdep = RCTRL_PRSDEP_L4;
	}
	sc->sc_rctrl |= rctrl_prsdep;
#if 0
	if (sc->sc_rctrl & (RCTRL_IPCSEN|RCTRL_TUCSEN|RCTRL_VLEX|RCTRL_PRSDEP))
		aprint_normal_dev(sc->sc_dev,
		    "rctrl=%#x ipcsen=%"PRIuMAX" tucsen=%"PRIuMAX" vlex=%"PRIuMAX" prsdep=%"PRIuMAX"\n",
		    sc->sc_rctrl,
		    __SHIFTOUT(sc->sc_rctrl, RCTRL_IPCSEN),
		    __SHIFTOUT(sc->sc_rctrl, RCTRL_TUCSEN),
		    __SHIFTOUT(sc->sc_rctrl, RCTRL_VLEX),
		    __SHIFTOUT(sc->sc_rctrl, RCTRL_PRSDEP));
#endif

	sc->sc_tctrl &= ~(TCTRL_IPCSEN|TCTRL_TUCSEN|TCTRL_VLINS);
	if (VLAN_ATTACHED(&sc->sc_ec))		/* is this really true */
		sc->sc_tctrl |= TCTRL_VLINS;
	if (ifp->if_capenable & IFCAP_TCTRL_IPCSEN)
		sc->sc_tctrl |= TCTRL_IPCSEN;
	if (ifp->if_capenable & IFCAP_TCTRL_TUCSEN)
		sc->sc_tctrl |= TCTRL_TUCSEN;
#if 0
	if (sc->sc_tctrl & (TCTRL_IPCSEN|TCTRL_TUCSEN|TCTRL_VLINS))
		aprint_normal_dev(sc->sc_dev,
		    "tctrl=%#x ipcsen=%"PRIuMAX" tucsen=%"PRIuMAX" vlins=%"PRIuMAX"\n",
		    sc->sc_tctrl,
		    __SHIFTOUT(sc->sc_tctrl, TCTRL_IPCSEN),
		    __SHIFTOUT(sc->sc_tctrl, TCTRL_TUCSEN),
		    __SHIFTOUT(sc->sc_tctrl, TCTRL_VLINS));
#endif

	sc->sc_maccfg1 &= ~(MACCFG1_TX_EN|MACCFG1_RX_EN);

	const uint64_t macstnaddr =
	    pq3etsec_macaddr_create(CLLADDR(ifp->if_sadl));

	sc->sc_imask = IEVENT_DPE;

	/* 5. Load TDBPH, TBASEH, TBASE0-TBASE7 with new Tx BD pointers */
	pq3etsec_rxq_reset(sc, &sc->sc_rxq);
	pq3etsec_rxq_produce(sc, &sc->sc_rxq);	/* fill with rx buffers */

	/* 6. Load RDBPH, RBASEH, RBASE0-RBASE7 with new Rx BD pointers */
	pq3etsec_txq_reset(sc, &sc->sc_txq);

	/* 7. Setup other MAC registers (MACCFG2, MAXFRM, etc.) */
	KASSERT(MACCFG2_PADCRC & sc->sc_maccfg2);
	etsec_write(sc, MAXFRM, sc->sc_maxfrm);
	etsec_write(sc, MACSTNADDR1, (uint32_t)(macstnaddr >> 32));
	etsec_write(sc, MACSTNADDR2, (uint32_t)(macstnaddr >>  0));
	etsec_write(sc, MACCFG1, sc->sc_maccfg1);
	etsec_write(sc, MACCFG2, sc->sc_maccfg2);
	etsec_write(sc, ECNTRL, sc->sc_ecntrl);

	/* 8. Setup group address hash table (GADDR0-GADDR15) */
	pq3etsec_mc_setup(sc);

	/* 9. Setup receive frame filer table (via RQFAR, RQFCR, and RQFPR) */
	etsec_write(sc, MRBLR, MCLBYTES);

	/* 10. Setup WWR, WOP, TOD bits in DMACTRL register */
	sc->sc_dmactrl |= DMACTRL_DEFAULT;
	etsec_write(sc, DMACTRL, sc->sc_dmactrl);

	/* 11. Enable transmit queues in TQUEUE, and ensure that the transmit scheduling mode is correctly set in TCTRL. */
	etsec_write(sc, TQUEUE, TQUEUE_EN0);
	sc->sc_imask |= IEVENT_TXF|IEVENT_TXE|IEVENT_TXC;

	etsec_write(sc, TCTRL, sc->sc_tctrl);	/* for TOE stuff */

	/* 12. Enable receive queues in RQUEUE, */
	etsec_write(sc, RQUEUE, RQUEUE_EN0|RQUEUE_EX0);
	sc->sc_imask |= IEVENT_RXF|IEVENT_BSY|IEVENT_RXC;

	/*     and optionally set TOE functionality in RCTRL. */
	etsec_write(sc, RCTRL, sc->sc_rctrl);
	sc->sc_rx_adjlen = __SHIFTOUT(sc->sc_rctrl, RCTRL_PAL);
	if ((sc->sc_rctrl & RCTRL_PRSDEP) != RCTRL_PRSDEP_OFF)
		sc->sc_rx_adjlen += sizeof(struct rxfcb);

	/* 13. Clear THLT and TXF bits in TSTAT register by writing 1 to them */
	etsec_write(sc, TSTAT, TSTAT_THLT | TSTAT_TXF);

	/* 14. Clear QHLT and RXF bits in RSTAT register by writing 1 to them.*/
	etsec_write(sc, RSTAT, RSTAT_QHLT | RSTAT_RXF);

	/* 15. Clear GRS/GTS bits in DMACTRL (do not change other bits) */
	sc->sc_dmactrl &= ~(DMACTRL_GRS|DMACTRL_GTS);
	etsec_write(sc, DMACTRL, sc->sc_dmactrl);

	/* 16. Enable Tx_EN/Rx_EN in MACCFG1 register */
	etsec_write(sc, MACCFG1, sc->sc_maccfg1 | MACCFG1_TX_EN|MACCFG1_RX_EN);
	etsec_write(sc, MACCFG1, sc->sc_maccfg1 | MACCFG1_TX_EN|MACCFG1_RX_EN);

	sc->sc_soft_flags = 0;

	etsec_write(sc, IMASK, sc->sc_imask);

	ifp->if_flags |= IFF_RUNNING;

	return error;
}

static void
pq3etsec_ifstop(struct ifnet *ifp, int disable)
{
	struct pq3etsec_softc * const sc = ifp->if_softc;

	KASSERT(!cpu_intr_p());
	const uint32_t imask_gsc_mask = IEVENT_GTSC|IEVENT_GRSC;
	/*
	 * Clear the GTSC and GRSC from the interrupt mask until
	 * we are ready for them.  Then clear them from IEVENT,
	 * request the graceful shutdown, and then enable the
	 * GTSC and GRSC bits in the mask.  This should cause the
	 * error interrupt to fire which will issue a wakeup to
	 * allow us to resume.
	 */

	/*
	 * 1. Set GRS/GTS bits in DMACTRL register
	 */
	sc->sc_dmactrl |= DMACTRL_GRS|DMACTRL_GTS;
	etsec_write(sc, IMASK, sc->sc_imask & ~imask_gsc_mask);
	etsec_write(sc, IEVENT, imask_gsc_mask);
	etsec_write(sc, DMACTRL, sc->sc_dmactrl);

	if (etsec_read(sc, MACCFG1) & (MACCFG1_TX_EN|MACCFG1_RX_EN)) {
		/*
		 * 2. Poll GRSC/GTSC bits in IEVENT register until both are set
		 */
		etsec_write(sc, IMASK, sc->sc_imask | imask_gsc_mask);

		u_int timo = 1000;
		uint32_t ievent = etsec_read(sc, IEVENT);
		while ((ievent & imask_gsc_mask) != imask_gsc_mask) {
			if (--timo == 0) {
				aprint_error_dev(sc->sc_dev,
				    "WARNING: "
				    "request to stop failed (IEVENT=%#x)\n",
				    ievent);
				break;
			}
			delay(10);
			ievent = etsec_read(sc, IEVENT);
		}
	}

	/*
	 * Now reset the controller.
	 *
	 * 3. Set SOFT_RESET bit in MACCFG1 register
	 * 4. Clear SOFT_RESET bit in MACCFG1 register
	 */
	etsec_write(sc, MACCFG1, MACCFG1_SOFT_RESET);
	etsec_write(sc, MACCFG1, 0);
	etsec_write(sc, IMASK, 0);
	etsec_write(sc, IEVENT, ~0);
	sc->sc_imask = 0;
	ifp->if_flags &= ~IFF_RUNNING;

	uint32_t tbipa = etsec_read(sc, TBIPA);
	if (tbipa == sc->sc_phy_addr) {
		aprint_normal_dev(sc->sc_dev, "relocating TBI\n");
		etsec_write(sc, TBIPA, 0x1f);
	}
	uint32_t miimcfg = etsec_read(sc, MIIMCFG);
	etsec_write(sc, MIIMCFG, MIIMCFG_RESET);
	etsec_write(sc, MIIMCFG, miimcfg);

	/*
	 * Let's consume any remaing transmitted packets.  And if we are
	 * disabling the interface, purge ourselves of any untransmitted
	 * packets.  But don't consume any received packets, just drop them.
	 * If we aren't disabling the interface, save the mbufs in the
	 * receive queue for reuse.
	 */
	pq3etsec_rxq_purge(sc, &sc->sc_rxq, disable);
	pq3etsec_txq_consume(sc, &sc->sc_txq);
	if (disable) {
		pq3etsec_txq_purge(sc, &sc->sc_txq);
		IF_PURGE(&ifp->if_snd);
	}
}

static void
pq3etsec_ifwatchdog(struct ifnet *ifp)
{
}

static void
pq3etsec_mc_setup(
	struct pq3etsec_softc *sc)
{
	struct ethercom * const ec = &sc->sc_ec;
	struct ifnet * const ifp = &sc->sc_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t *gaddr = sc->sc_gaddr + ((sc->sc_rctrl & RCTRL_GHTX) ? 0 : 8);
	const uint32_t crc_shift = 32 - ((sc->sc_rctrl & RCTRL_GHTX) ? 9 : 8);

	memset(sc->sc_gaddr, 0, sizeof(sc->sc_gaddr));
	memset(sc->sc_macaddrs, 0, sizeof(sc->sc_macaddrs));

	ifp->if_flags &= ~IFF_ALLMULTI;

	ETHER_FIRST_MULTI(step, ec, enm);
	for (u_int i = 0; enm != NULL; ) {
		const char *addr = enm->enm_addrlo;
		if (memcmp(addr, enm->enm_addrhi, ETHER_ADDR_LEN) != 0) {
			ifp->if_flags |= IFF_ALLMULTI;
			memset(gaddr, 0xff, 32 << (crc_shift & 1));
			memset(sc->sc_macaddrs, 0, sizeof(sc->sc_macaddrs));
			break;
		}
		if ((sc->sc_rctrl & RCTRL_EMEN)
		    && i < __arraycount(sc->sc_macaddrs)) {
			sc->sc_macaddrs[i++] = pq3etsec_macaddr_create(addr);
		} else {
			uint32_t crc = ether_crc32_be(addr, ETHER_ADDR_LEN);
#if 0
			printf("%s: %s: crc=%#x: %#x: [%u,%u]=%#x\n", __func__,
			    ether_sprintf(addr), crc,
			    crc >> crc_shift,
			    crc >> (crc_shift + 5),
			    (crc >> crc_shift) & 31,
			    1 << (((crc >> crc_shift) & 31) ^ 31));
#endif
			/*
			 * The documentation doesn't completely follow PowerPC
			 * bit order.  The BE crc32 (H) for 01:00:5E:00:00:01
			 * is 0x7fa32d9b.  By empirical testing, the
			 * corresponding hash bit is word 3, bit 31 (ppc bit
			 * order).  Since 3 << 31 | 31 is 0x7f, we deduce
			 * H[0:2] selects the register while H[3:7] selects
			 * the bit (ppc bit order).
			 */
			crc >>= crc_shift;
			gaddr[crc / 32] |= 1 << ((crc & 31) ^ 31);
		}
		ETHER_NEXT_MULTI(step, enm);
	}
	for (u_int i = 0; i < 8; i++) {
		etsec_write(sc, IGADDR(i), sc->sc_gaddr[i]);
		etsec_write(sc, GADDR(i), sc->sc_gaddr[i+8]);
#if 0
		if (sc->sc_gaddr[i] || sc->sc_gaddr[i+8])
		printf("%s: IGADDR%u(%#x)=%#x GADDR%u(%#x)=%#x\n", __func__,
		    i, IGADDR(i), etsec_read(sc, IGADDR(i)),
		    i, GADDR(i), etsec_read(sc, GADDR(i)));
#endif
	}
	for (u_int i = 0; i < __arraycount(sc->sc_macaddrs); i++) {
		uint64_t macaddr = sc->sc_macaddrs[i];
		etsec_write(sc, MACnADDR1(i), (uint32_t)(macaddr >> 32));
		etsec_write(sc, MACnADDR2(i), (uint32_t)(macaddr >>  0));
#if 0
		if (macaddr)
		printf("%s: MAC%02uADDR2(%08x)=%#x MAC%02uADDR2(%#x)=%08x\n", __func__,
		    i+1, MACnADDR1(i), etsec_read(sc, MACnADDR1(i)),
		    i+1, MACnADDR2(i), etsec_read(sc, MACnADDR2(i)));
#endif
	}
}

static int
pq3etsec_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct pq3etsec_softc *sc  = ifp->if_softc;
	struct ifreq * const ifr = data;
	const int s = splnet();
	int error;

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		/* Flow control requires full-duplex mode. */
		if (IFM_SUBTYPE(ifr->ifr_media) == IFM_AUTO ||
		    (ifr->ifr_media & IFM_FDX) == 0)
			ifr->ifr_media &= ~IFM_ETH_FMASK;
		if (IFM_SUBTYPE(ifr->ifr_media) != IFM_AUTO) {
			if ((ifr->ifr_media & IFM_ETH_FMASK) == IFM_FLOW) {
				/* We can do both TXPAUSE and RXPAUSE. */
				ifr->ifr_media |=
				    IFM_ETH_TXPAUSE | IFM_ETH_RXPAUSE;
			}
		}
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error != ENETRESET)
			break;

		if (cmd == SIOCADDMULTI || cmd == SIOCDELMULTI) {
			error = 0;
			if (ifp->if_flags & IFF_RUNNING)
				pq3etsec_mc_setup(sc);
			break;
		}
		error = pq3etsec_ifinit(ifp);
		break;
	}

	splx(s);
	return error;
}

static void
pq3etsec_rxq_desc_presync(
	struct pq3etsec_softc *sc,
	struct pq3etsec_rxqueue *rxq,
	volatile struct rxbd *rxbd,
	size_t count)
{
	bus_dmamap_sync(sc->sc_dmat, rxq->rxq_descmap, 
	    (rxbd - rxq->rxq_first) * sizeof(*rxbd), count * sizeof(*rxbd),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
}

static void
pq3etsec_rxq_desc_postsync(
	struct pq3etsec_softc *sc,
	struct pq3etsec_rxqueue *rxq,
	volatile struct rxbd *rxbd,
	size_t count)
{
	bus_dmamap_sync(sc->sc_dmat, rxq->rxq_descmap, 
	    (rxbd - rxq->rxq_first) * sizeof(*rxbd), count * sizeof(*rxbd),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
}

static void
pq3etsec_txq_desc_presync(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq,
	volatile struct txbd *txbd,
	size_t count)
{
	bus_dmamap_sync(sc->sc_dmat, txq->txq_descmap, 
	    (txbd - txq->txq_first) * sizeof(*txbd), count * sizeof(*txbd),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
}

static void
pq3etsec_txq_desc_postsync(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq,
	volatile struct txbd *txbd,
	size_t count)
{
	bus_dmamap_sync(sc->sc_dmat, txq->txq_descmap, 
	    (txbd - txq->txq_first) * sizeof(*txbd), count * sizeof(*txbd),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
}

static bus_dmamap_t
pq3etsec_mapcache_get(
	struct pq3etsec_softc *sc,
	struct pq3etsec_mapcache *dmc)
{
	KASSERT(dmc->dmc_nmaps > 0);
	KASSERT(dmc->dmc_maps[dmc->dmc_nmaps-1] != NULL);
	return dmc->dmc_maps[--dmc->dmc_nmaps];
}

static void
pq3etsec_mapcache_put(
	struct pq3etsec_softc *sc,
	struct pq3etsec_mapcache *dmc,
	bus_dmamap_t map)
{
	KASSERT(map != NULL);
	KASSERT(dmc->dmc_nmaps < dmc->dmc_maxmaps);
	dmc->dmc_maps[dmc->dmc_nmaps++] = map;
}

static void
pq3etsec_mapcache_destroy(
	struct pq3etsec_softc *sc,
	struct pq3etsec_mapcache *dmc)
{
	const size_t dmc_size =
	    offsetof(struct pq3etsec_mapcache, dmc_maps[dmc->dmc_maxmaps]);

	for (u_int i = 0; i < dmc->dmc_maxmaps; i++) {
		bus_dmamap_destroy(sc->sc_dmat, dmc->dmc_maps[i]);
	}
	kmem_free(dmc, dmc_size);
}

static int
pq3etsec_mapcache_create(
	struct pq3etsec_softc *sc,
	struct pq3etsec_mapcache **dmc_p,
	size_t maxmaps,
	size_t maxmapsize,
	size_t maxseg)
{
	const size_t dmc_size =
	    offsetof(struct pq3etsec_mapcache, dmc_maps[maxmaps]);
	struct pq3etsec_mapcache * const dmc = kmem_zalloc(dmc_size, KM_SLEEP);

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
			kmem_free(dmc, dmc_size);
			return error;
		}
		KASSERT(dmc->dmc_maps[i] != NULL);
	}

	*dmc_p = dmc;

	return 0;
}

#if 0
static void
pq3etsec_dmamem_free(
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
pq3etsec_dmamem_alloc(
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

	error = bus_dmamem_alloc(dmat, map_size, PAGE_SIZE, 0,
	   seg, 1, &nseg, 0);
	if (error)
		return error;

	KASSERT(nseg == 1);

	error = bus_dmamem_map(dmat, seg, nseg, map_size, (void **)kvap,
	    BUS_DMA_COHERENT);
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
pq3etsec_rx_buf_alloc(
	struct pq3etsec_softc *sc)
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

	bus_dmamap_t map = pq3etsec_mapcache_get(sc, sc->sc_rx_mapcache);
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
		pq3etsec_mapcache_put(sc, sc->sc_rx_mapcache, map);
		return NULL;
	}
	KASSERT(map->dm_mapsize == MCLBYTES);
	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	return m;
}

static void
pq3etsec_rx_map_unload(
	struct pq3etsec_softc *sc,
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
		pq3etsec_mapcache_put(sc, sc->sc_rx_mapcache, map);
		M_SETCTX(m, NULL);
	}
}

static bool
pq3etsec_rxq_produce(
	struct pq3etsec_softc *sc,
	struct pq3etsec_rxqueue *rxq)
{
	volatile struct rxbd *producer = rxq->rxq_producer;
#if 0
	size_t inuse = rxq->rxq_inuse;
#endif
	while (rxq->rxq_inuse < rxq->rxq_threshold) {
		struct mbuf *m;
		IF_DEQUEUE(&sc->sc_rx_bufcache, m);
		if (m == NULL) {
			m = pq3etsec_rx_buf_alloc(sc);
			if (m == NULL) {
				printf("%s: pq3etsec_rx_buf_alloc failed\n", __func__);
				break;
			}
		}
		bus_dmamap_t map = M_GETCTX(m, bus_dmamap_t);
		KASSERT(map);

#ifdef ETSEC_DEBUG
		KASSERT(rxq->rxq_mbufs[producer-rxq->rxq_first] == NULL);
		rxq->rxq_mbufs[producer-rxq->rxq_first] = m;
#endif

		/* rxbd_len is write-only by the ETSEC */
		producer->rxbd_bufptr = map->dm_segs[0].ds_addr;
		membar_producer();
		producer->rxbd_flags |= RXBD_E;
		if (__predict_false(rxq->rxq_mhead == NULL)) {
			KASSERT(producer == rxq->rxq_consumer);
			rxq->rxq_mconsumer = m;
		}
		*rxq->rxq_mtail = m;
		rxq->rxq_mtail = &m->m_next;
		m->m_len = MCLBYTES;
		m->m_next = NULL;
		rxq->rxq_inuse++;
		if (++producer == rxq->rxq_last) {
			membar_producer();
			pq3etsec_rxq_desc_presync(sc, rxq, rxq->rxq_producer,
			    rxq->rxq_last - rxq->rxq_producer);
			producer = rxq->rxq_producer = rxq->rxq_first;
		}
	}
	if (producer != rxq->rxq_producer) {
		membar_producer();
		pq3etsec_rxq_desc_presync(sc, rxq, rxq->rxq_producer,
		    producer - rxq->rxq_producer);
		rxq->rxq_producer = producer;
	}
	uint32_t qhlt = etsec_read(sc, RSTAT) & RSTAT_QHLT;
	if (qhlt) {
		KASSERT(qhlt & rxq->rxq_qmask);
		sc->sc_ev_rx_stall.ev_count++;
		etsec_write(sc, RSTAT, RSTAT_QHLT & rxq->rxq_qmask);
	}
#if 0
	aprint_normal_dev(sc->sc_dev,
	    "%s: buffers inuse went from %zu to %zu\n",
	    __func__, inuse, rxq->rxq_inuse);
#endif
	return true;
}

static bool
pq3etsec_rx_offload(
	struct pq3etsec_softc *sc,
	struct mbuf *m,
	const struct rxfcb *fcb)
{
	if (fcb->rxfcb_flags & RXFCB_VLN) {
		VLAN_INPUT_TAG(&sc->sc_if, m, fcb->rxfcb_vlctl,
		    m_freem(m); return false);
	}
	if ((fcb->rxfcb_flags & RXFCB_IP) == 0
	    || (fcb->rxfcb_flags & (RXFCB_CIP|RXFCB_CTU)) == 0)
		return true;
	int csum_flags = 0;
	if ((fcb->rxfcb_flags & (RXFCB_IP6|RXFCB_CIP)) == RXFCB_CIP) {
		csum_flags |= M_CSUM_IPv4;
		if (fcb->rxfcb_flags & RXFCB_EIP)
			csum_flags |= M_CSUM_IPv4_BAD;
	}
	if ((fcb->rxfcb_flags & RXFCB_CTU) == RXFCB_CTU) {
		int ipv_flags;
		if (fcb->rxfcb_flags & RXFCB_IP6)
			ipv_flags = M_CSUM_TCPv6|M_CSUM_UDPv6;
		else
			ipv_flags = M_CSUM_TCPv4|M_CSUM_UDPv4;
		if (fcb->rxfcb_pro == IPPROTO_TCP) {
			csum_flags |= (M_CSUM_TCPv4|M_CSUM_TCPv6) & ipv_flags;
		} else {
			csum_flags |= (M_CSUM_UDPv4|M_CSUM_UDPv6) & ipv_flags;
		}
		if (fcb->rxfcb_flags & RXFCB_ETU)
			csum_flags |= M_CSUM_TCP_UDP_BAD;
	}

	m->m_pkthdr.csum_flags = csum_flags;
	return true;
}

static void
pq3etsec_rx_input(
	struct pq3etsec_softc *sc,
	struct mbuf *m,
	uint16_t rxbd_flags)
{
	struct ifnet * const ifp = &sc->sc_if;

	pq3etsec_rx_map_unload(sc, m);

	if ((sc->sc_rctrl & RCTRL_PRSDEP) != RCTRL_PRSDEP_OFF) {
		struct rxfcb fcb = *mtod(m, struct rxfcb *);
		if (!pq3etsec_rx_offload(sc, m, &fcb))
			return;
	}
	m_adj(m, sc->sc_rx_adjlen);

	if (rxbd_flags & RXBD_M)
		m->m_flags |= M_PROMISC;
	if (rxbd_flags & RXBD_BC)
		m->m_flags |= M_BCAST;
	if (rxbd_flags & RXBD_MC)
		m->m_flags |= M_MCAST;
	m->m_flags |= M_HASFCS;
	m->m_pkthdr.rcvif = &sc->sc_if;

	ifp->if_ipackets++;
	ifp->if_ibytes += m->m_pkthdr.len;

	/*
	 * Let's give it to the network subsystm to deal with.
	 */
	int s = splnet();
	bpf_mtap(ifp, m);
	(*ifp->if_input)(ifp, m);
	splx(s);
}

static void
pq3etsec_rxq_consume(
	struct pq3etsec_softc *sc,
	struct pq3etsec_rxqueue *rxq)
{
	struct ifnet * const ifp = &sc->sc_if;
	volatile struct rxbd *consumer = rxq->rxq_consumer;
	size_t rxconsumed = 0;

	etsec_write(sc, RSTAT, RSTAT_RXF & rxq->rxq_qmask);

	for (;;) {
		if (consumer == rxq->rxq_producer) {
			rxq->rxq_consumer = consumer;
			rxq->rxq_inuse -= rxconsumed;
			KASSERT(rxq->rxq_inuse == 0);
			return;
		}
		pq3etsec_rxq_desc_postsync(sc, rxq, consumer, 1);
		const uint16_t rxbd_flags = consumer->rxbd_flags;
		if (rxbd_flags & RXBD_E) {
			rxq->rxq_consumer = consumer;
			rxq->rxq_inuse -= rxconsumed;
			return;
		}
		KASSERT(rxq->rxq_mconsumer != NULL);
#ifdef ETSEC_DEBUG
		KASSERT(rxq->rxq_mbufs[consumer - rxq->rxq_first] == rxq->rxq_mconsumer);
#endif
#if 0
		printf("%s: rxdb[%u]: flags=%#x len=%#x: %08x %08x %08x %08x\n",
		    __func__,
		    consumer - rxq->rxq_first, rxbd_flags, consumer->rxbd_len,
		    mtod(rxq->rxq_mconsumer, int *)[0],
		    mtod(rxq->rxq_mconsumer, int *)[1],
		    mtod(rxq->rxq_mconsumer, int *)[2],
		    mtod(rxq->rxq_mconsumer, int *)[3]);
#endif
		/*
		 * We own this packet again.  Clear all flags except wrap.
		 */
		rxconsumed++;
		consumer->rxbd_flags = rxbd_flags & (RXBD_W|RXBD_I);

		/*
		 * If this descriptor has the LAST bit set and no errors,
		 * it's a valid input packet.
		 */
		if ((rxbd_flags & (RXBD_L|RXBD_ERRORS)) == RXBD_L) {
			size_t rxbd_len = consumer->rxbd_len;
			struct mbuf *m = rxq->rxq_mhead;
			struct mbuf *m_last = rxq->rxq_mconsumer;
			if ((rxq->rxq_mhead = m_last->m_next) == NULL)
				rxq->rxq_mtail = &rxq->rxq_mhead;
			rxq->rxq_mconsumer = rxq->rxq_mhead;
			m_last->m_next = NULL;
			m_last->m_len = rxbd_len & (MCLBYTES - 1);
			m->m_pkthdr.len = rxbd_len;
			pq3etsec_rx_input(sc, m, rxbd_flags);
		} else if (rxbd_flags & RXBD_L) {
			KASSERT(rxbd_flags & RXBD_ERRORS);
			struct mbuf *m;
			/*
			 * We encountered an error, take the mbufs and add
			 * then to the rx bufcache so we can reuse them.
			 */
			ifp->if_ierrors++;
			for (m = rxq->rxq_mhead;
			     m != rxq->rxq_mconsumer;
			     m = m->m_next) {
				IF_ENQUEUE(&sc->sc_rx_bufcache, m);
			}
			m = rxq->rxq_mconsumer;
			if ((rxq->rxq_mhead = m->m_next) == NULL)
				rxq->rxq_mtail = &rxq->rxq_mhead;
			rxq->rxq_mconsumer = m->m_next;
			IF_ENQUEUE(&sc->sc_rx_bufcache, m);
		} else {
			rxq->rxq_mconsumer = rxq->rxq_mconsumer->m_next;
		}
#ifdef ETSEC_DEBUG
		rxq->rxq_mbufs[consumer - rxq->rxq_first] = NULL;
#endif

		/*
		 * Wrap at the last entry!
		 */
		if (rxbd_flags & RXBD_W) {
			KASSERT(consumer + 1 == rxq->rxq_last);
			consumer = rxq->rxq_first;
		} else {
			consumer++;
		}
#ifdef ETSEC_DEBUG
		KASSERT(rxq->rxq_mbufs[consumer - rxq->rxq_first] == rxq->rxq_mconsumer);
#endif
	}
}

static void
pq3etsec_rxq_purge(
	struct pq3etsec_softc *sc,
	struct pq3etsec_rxqueue *rxq,
	bool discard)
{
	struct mbuf *m;

	if ((m = rxq->rxq_mhead) != NULL) {
#ifdef ETSEC_DEBUG
		memset(rxq->rxq_mbufs, 0, sizeof(rxq->rxq_mbufs));
#endif

		if (discard) {
			pq3etsec_rx_map_unload(sc, m);
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

	rxq->rxq_mconsumer = NULL;
	rxq->rxq_mhead = NULL;
	rxq->rxq_mtail = &rxq->rxq_mhead;
	rxq->rxq_inuse = 0;
}

static void
pq3etsec_rxq_reset(
	struct pq3etsec_softc *sc,
	struct pq3etsec_rxqueue *rxq)
{
	/*
	 * sync all the descriptors
	 */
	pq3etsec_rxq_desc_postsync(sc, rxq, rxq->rxq_first,
	    rxq->rxq_last - rxq->rxq_first);

	/*
	 * Make sure we own all descriptors in the ring.
	 */
	volatile struct rxbd *rxbd;
	for (rxbd = rxq->rxq_first; rxbd < rxq->rxq_last - 1; rxbd++) {
		rxbd->rxbd_flags = RXBD_I;
	}

	/*
	 * Last descriptor has the wrap flag.
	 */
	rxbd->rxbd_flags = RXBD_W|RXBD_I;

	/*
	 * Reset the producer consumer indexes.
	 */
	rxq->rxq_consumer = rxq->rxq_first;
	rxq->rxq_producer = rxq->rxq_first;
	rxq->rxq_inuse = 0;
	if (rxq->rxq_threshold < ETSEC_MINRXMBUFS)
		rxq->rxq_threshold = ETSEC_MINRXMBUFS;

	sc->sc_imask |= IEVENT_RXF|IEVENT_BSY;

	/*
	 * Restart the transmit at the first descriptor
	 */
	etsec_write(sc, rxq->rxq_reg_rbase, rxq->rxq_descmap->dm_segs->ds_addr);
}

static int
pq3etsec_rxq_attach(
	struct pq3etsec_softc *sc,
	struct pq3etsec_rxqueue *rxq,
	u_int qno)
{
	size_t map_size = PAGE_SIZE;
	size_t desc_count = map_size / sizeof(struct rxbd);
	int error;
	void *descs;

	error = pq3etsec_dmamem_alloc(sc->sc_dmat, map_size,
	   &rxq->rxq_descmap_seg, &rxq->rxq_descmap, &descs);
	if (error)
		return error;

	memset(descs, 0, map_size);
	rxq->rxq_first = descs;
	rxq->rxq_last = rxq->rxq_first + desc_count;
	rxq->rxq_consumer = descs;
	rxq->rxq_producer = descs;

	pq3etsec_rxq_purge(sc, rxq, true);
	pq3etsec_rxq_reset(sc, rxq);

	rxq->rxq_reg_rbase = RBASEn(qno);
	rxq->rxq_qmask = RSTAT_QHLTn(qno) | RSTAT_RXFn(qno);

	return 0;
}

static bool
pq3etsec_txq_active_p(
	struct pq3etsec_softc * const sc,
	struct pq3etsec_txqueue *txq)
{
	return !IF_IS_EMPTY(&txq->txq_mbufs);
}

static bool
pq3etsec_txq_fillable_p(
	struct pq3etsec_softc * const sc,
	struct pq3etsec_txqueue *txq)
{
	return txq->txq_free >= txq->txq_threshold;
}

static int
pq3etsec_txq_attach(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq,
	u_int qno)
{
	size_t map_size = PAGE_SIZE;
	size_t desc_count = map_size / sizeof(struct txbd);
	int error;
	void *descs;

	error = pq3etsec_dmamem_alloc(sc->sc_dmat, map_size,
	   &txq->txq_descmap_seg, &txq->txq_descmap, &descs);
	if (error)
		return error;

	memset(descs, 0, map_size);
	txq->txq_first = descs;
	txq->txq_last = txq->txq_first + desc_count;
	txq->txq_consumer = descs;
	txq->txq_producer = descs;

	IFQ_SET_MAXLEN(&txq->txq_mbufs, ETSEC_MAXTXMBUFS);

	txq->txq_reg_tbase = TBASEn(qno);
	txq->txq_qmask = TSTAT_THLTn(qno) | TSTAT_TXFn(qno);

	pq3etsec_txq_reset(sc, txq);

	return 0;
}

static int
pq3etsec_txq_map_load(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq,
	struct mbuf *m)
{
	bus_dmamap_t map;
	int error;

	map = M_GETCTX(m, bus_dmamap_t);
	if (map != NULL)
		return 0;

	map = pq3etsec_mapcache_get(sc, sc->sc_tx_mapcache);
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
pq3etsec_txq_map_unload(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq,
	struct mbuf *m)
{
	KASSERT(m);
	bus_dmamap_t map = M_GETCTX(m, bus_dmamap_t);
	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, map);
	pq3etsec_mapcache_put(sc, sc->sc_tx_mapcache, map);
}

static bool
pq3etsec_txq_produce(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq,
	struct mbuf *m)
{
	bus_dmamap_t map = M_GETCTX(m, bus_dmamap_t);

	if (map->dm_nsegs > txq->txq_free)
		return false;

	/*
	 * TCP Offload flag must be set in the first descriptor.
	 */
	volatile struct txbd *producer = txq->txq_producer;
	uint16_t last_flags = TXBD_L;
	uint16_t first_flags = TXBD_R
	    | ((m->m_flags & M_HASFCB) ? TXBD_TOE : 0);

	/*
	 * If we've produced enough descriptors without consuming any
	 * we need to ask for an interrupt to reclaim some.
	 */
	txq->txq_lastintr += map->dm_nsegs;
	if (txq->txq_lastintr >= txq->txq_threshold
	    || txq->txq_mbufs.ifq_len + 1 == txq->txq_mbufs.ifq_maxlen) {
		txq->txq_lastintr = 0;
		last_flags |= TXBD_I;
	}

#ifdef ETSEC_DEBUG
	KASSERT(txq->txq_lmbufs[producer - txq->txq_first] == NULL);
#endif
	KASSERT(producer != txq->txq_last);
	producer->txbd_bufptr = map->dm_segs[0].ds_addr;
	producer->txbd_len = map->dm_segs[0].ds_len;

	if (map->dm_nsegs > 1) {
		volatile struct txbd *start = producer + 1;
		size_t count = map->dm_nsegs - 1;
		for (u_int i = 1; i < map->dm_nsegs; i++) {
			if (__predict_false(++producer == txq->txq_last)) {
				producer = txq->txq_first;
				if (start < txq->txq_last) {
					pq3etsec_txq_desc_presync(sc, txq,
					    start, txq->txq_last - start);
					count -= txq->txq_last - start;
				}
				start = txq->txq_first;
			}
#ifdef ETSEC_DEBUG
			KASSERT(txq->txq_lmbufs[producer - txq->txq_first] == NULL);
#endif
			producer->txbd_bufptr = map->dm_segs[i].ds_addr;
			producer->txbd_len = map->dm_segs[i].ds_len;
			producer->txbd_flags = TXBD_R
			    | (producer->txbd_flags & TXBD_W)
			    | (i == map->dm_nsegs - 1 ? last_flags : 0);
#if 0
			printf("%s: txbd[%u]=%#x/%u/%#x\n", __func__, producer - txq->txq_first,
			    producer->txbd_flags, producer->txbd_len, producer->txbd_bufptr);
#endif
		}
		pq3etsec_txq_desc_presync(sc, txq, start, count);
	} else {
		first_flags |= last_flags;
	}

	membar_producer();
	txq->txq_producer->txbd_flags =
	    first_flags | (txq->txq_producer->txbd_flags & TXBD_W);
#if 0
	printf("%s: txbd[%u]=%#x/%u/%#x\n", __func__,
	    txq->txq_producer - txq->txq_first, txq->txq_producer->txbd_flags,
	    txq->txq_producer->txbd_len, txq->txq_producer->txbd_bufptr);
#endif
	pq3etsec_txq_desc_presync(sc, txq, txq->txq_producer, 1);

	/*
	 * Reduce free count by the number of segments we consumed.
	 */
	txq->txq_free -= map->dm_nsegs;
	KASSERT(map->dm_nsegs == 1 || txq->txq_producer != producer);
	KASSERT(map->dm_nsegs == 1 || (txq->txq_producer->txbd_flags & TXBD_L) == 0);
	KASSERT(producer->txbd_flags & TXBD_L);
#ifdef ETSEC_DEBUG
	txq->txq_lmbufs[producer - txq->txq_first] = m;
#endif

#if 0
	printf("%s: mbuf %p: produced a %u byte packet in %u segments (%u..%u)\n",
	    __func__, m, m->m_pkthdr.len, map->dm_nsegs,
	    txq->txq_producer - txq->txq_first, producer - txq->txq_first);
#endif

	if (++producer == txq->txq_last)
		txq->txq_producer = txq->txq_first;
	else
		txq->txq_producer = producer;
	IF_ENQUEUE(&txq->txq_mbufs, m);

	/*
	 * Restart the transmitter.
	 */
	etsec_write(sc, TSTAT, txq->txq_qmask & TSTAT_THLT);	/* W1C */

	return true;
}

static void
pq3etsec_tx_offload(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq,
	struct mbuf **mp)
{
	struct mbuf *m = *mp;
	u_int csum_flags = m->m_pkthdr.csum_flags;
	struct m_tag *vtag = VLAN_OUTPUT_TAG(&sc->sc_ec, m);

	KASSERT(m->m_flags & M_PKTHDR);

	/*
	 * Let see if we are doing any offload first.
	 */
	if (csum_flags == 0 && vtag == 0) {
		m->m_flags &= ~M_HASFCB;
		return;
	}

	uint16_t flags = 0;
	if (csum_flags & M_CSUM_IP) {
		flags |= TXFCB_IP
		    | ((csum_flags & M_CSUM_IP6) ? TXFCB_IP6 : 0)
		    | ((csum_flags & M_CSUM_TUP) ? TXFCB_TUP : 0)
		    | ((csum_flags & M_CSUM_UDP) ? TXFCB_UDP : 0)
		    | ((csum_flags & M_CSUM_CIP) ? TXFCB_CIP : 0)
		    | ((csum_flags & M_CSUM_CTU) ? TXFCB_CTU : 0);
	}
	if (vtag) {
		flags |= TXFCB_VLN;
	}
	if (flags == 0) {
		m->m_flags &= ~M_HASFCB;
		return;
	}

	struct txfcb fcb;
	fcb.txfcb_flags = flags;
	if (csum_flags & M_CSUM_IPv4)
		fcb.txfcb_l4os = M_CSUM_DATA_IPv4_IPHL(m->m_pkthdr.csum_data);
	else
		fcb.txfcb_l4os = M_CSUM_DATA_IPv6_HL(m->m_pkthdr.csum_data);
	fcb.txfcb_l3os = ETHER_HDR_LEN;
	fcb.txfcb_phcs = 0;
	fcb.txfcb_vlctl = vtag ? VLAN_TAG_VALUE(vtag) & 0xffff : 0;

#if 0
	printf("%s: csum_flags=%#x: txfcb flags=%#x lsos=%u l4os=%u phcs=%u vlctl=%#x\n",
	    __func__, csum_flags, fcb.txfcb_flags, fcb.txfcb_l3os, fcb.txfcb_l4os,
	    fcb.txfcb_phcs, fcb.txfcb_vlctl);
#endif

	if (M_LEADINGSPACE(m) >= sizeof(fcb)) {
		m->m_data -= sizeof(fcb);
		m->m_len += sizeof(fcb);
	} else if (!(m->m_flags & M_EXT) && MHLEN - m->m_len >= sizeof(fcb)) {
		memmove(m->m_pktdat + sizeof(fcb), m->m_data, m->m_len);
		m->m_data = m->m_pktdat;
		m->m_len += sizeof(fcb);
	} else {
		struct mbuf *mn;
		MGET(mn, M_DONTWAIT, m->m_type);
		if (mn == NULL) {
			if (csum_flags & M_CSUM_IP4) {
#ifdef INET
				ip_undefer_csum(m, ETHER_HDR_LEN,
				    csum_flags & M_CSUM_IP4);
#else
				panic("%s: impossible M_CSUM flags %#x",
				    device_xname(sc->sc_dev), csum_flags);
#endif
			} else if (csum_flags & M_CSUM_IP6) {
#ifdef INET6
				ip6_undefer_csum(m, ETHER_HDR_LEN,
				    csum_flags & M_CSUM_IP6);
#else
				panic("%s: impossible M_CSUM flags %#x",
				    device_xname(sc->sc_dev), csum_flags);
#endif
			} else if (vtag) {
			}

			m->m_flags &= ~M_HASFCB;
			return;
		}

		M_MOVE_PKTHDR(mn, m);
		mn->m_next = m;
		m = mn;
		MH_ALIGN(m, sizeof(fcb));
		m->m_len = sizeof(fcb);
		*mp = m;
	}
	m->m_pkthdr.len += sizeof(fcb);
	m->m_flags |= M_HASFCB;
	*mtod(m, struct txfcb *) = fcb;
	return;
}

static bool
pq3etsec_txq_enqueue(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq)
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
			pq3etsec_tx_offload(sc, txq, &m);
		} else {
			txq->txq_next = NULL;
		}
		int error = pq3etsec_txq_map_load(sc, txq, m);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "discarded packet due to "
			    "dmamap load failure: %d\n", error);
			m_freem(m);
			continue;
		}
		KASSERT(txq->txq_next == NULL);
		if (!pq3etsec_txq_produce(sc, txq, m)) {
			txq->txq_next = m;
			return false;
		}
		KASSERT(txq->txq_next == NULL);
	}
}

static bool
pq3etsec_txq_consume(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq)
{
	struct ifnet * const ifp = &sc->sc_if;
	volatile struct txbd *consumer = txq->txq_consumer;
	size_t txfree = 0;

#if 0
	printf("%s: entry: free=%zu\n", __func__, txq->txq_free);
#endif
	etsec_write(sc, TSTAT, TSTAT_TXF & txq->txq_qmask);

	for (;;) {
		if (consumer == txq->txq_producer) {
			txq->txq_consumer = consumer;
			txq->txq_free += txfree;
			txq->txq_lastintr -= min(txq->txq_lastintr, txfree);
#if 0
			printf("%s: empty: freed %zu descriptors going form %zu to %zu\n",
			    __func__, txfree, txq->txq_free - txfree, txq->txq_free);
#endif
			KASSERT(txq->txq_lastintr == 0);
			KASSERT(txq->txq_free == txq->txq_last - txq->txq_first - 1);
			return true;
		}
		pq3etsec_txq_desc_postsync(sc, txq, consumer, 1);
		const uint16_t txbd_flags = consumer->txbd_flags;
		if (txbd_flags & TXBD_R) {
			txq->txq_consumer = consumer;
			txq->txq_free += txfree;
			txq->txq_lastintr -= min(txq->txq_lastintr, txfree);
#if 0
			printf("%s: freed %zu descriptors\n",
			    __func__, txfree);
#endif
			return pq3etsec_txq_fillable_p(sc, txq);
		}

		/*
		 * If this is the last descriptor in the chain, get the
		 * mbuf, free its dmamap, and free the mbuf chain itself.
		 */
		if (txbd_flags & TXBD_L) {
			struct mbuf *m;

			IF_DEQUEUE(&txq->txq_mbufs, m);
#ifdef ETSEC_DEBUG
			KASSERTMSG(
			    m == txq->txq_lmbufs[consumer-txq->txq_first],
			    "%s: %p [%u]: flags %#x m (%p) != %p (%p)",
			    __func__, consumer, consumer - txq->txq_first,
			    txbd_flags, m,
			    &txq->txq_lmbufs[consumer-txq->txq_first],
			    txq->txq_lmbufs[consumer-txq->txq_first]);
#endif
			KASSERT(m);
			pq3etsec_txq_map_unload(sc, txq, m);
#if 0
			printf("%s: mbuf %p: consumed a %u byte packet\n",
			    __func__, m, m->m_pkthdr.len);
#endif
			if (m->m_flags & M_HASFCB)
				m_adj(m, sizeof(struct txfcb));
			ifp->if_opackets++;
			ifp->if_obytes += m->m_pkthdr.len;
			if (m->m_flags & M_MCAST)
				ifp->if_omcasts++;
			if (txbd_flags & TXBD_ERRORS)
				ifp->if_oerrors++;
			m_freem(m);
#ifdef ETSEC_DEBUG
			txq->txq_lmbufs[consumer - txq->txq_first] = NULL;
#endif
		} else {
#ifdef ETSEC_DEBUG
			KASSERT(txq->txq_lmbufs[consumer-txq->txq_first] == NULL);
#endif
		}

		/*
		 * We own this packet again.  Clear all flags except wrap.
		 */
		txfree++;
		//consumer->txbd_flags = txbd_flags & TXBD_W;

		/*
		 * Wrap at the last entry!
		 */
		if (txbd_flags & TXBD_W) {
			KASSERT(consumer + 1 == txq->txq_last);
			consumer = txq->txq_first;
		} else {
			consumer++;
			KASSERT(consumer < txq->txq_last);
		}
	}
}

static void
pq3etsec_txq_purge(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq)
{
	struct mbuf *m;
	KASSERT((etsec_read(sc, MACCFG1) & MACCFG1_TX_EN) == 0);

	for (;;) {
		IF_DEQUEUE(&txq->txq_mbufs, m);
		if (m == NULL)
			break;
		pq3etsec_txq_map_unload(sc, txq, m);
		m_freem(m);
	}
	if ((m = txq->txq_next) != NULL) {
		txq->txq_next = NULL;
		pq3etsec_txq_map_unload(sc, txq, m);
		m_freem(m);
	}
#ifdef ETSEC_DEBUG
	memset(txq->txq_lmbufs, 0, sizeof(txq->txq_lmbufs));
#endif
}

static void
pq3etsec_txq_reset(
	struct pq3etsec_softc *sc,
	struct pq3etsec_txqueue *txq)
{
	/*
	 * sync all the descriptors
	 */
	pq3etsec_txq_desc_postsync(sc, txq, txq->txq_first,
	    txq->txq_last - txq->txq_first);

	/*
	 * Make sure we own all descriptors in the ring.
	 */
	volatile struct txbd *txbd;
	for (txbd = txq->txq_first; txbd < txq->txq_last - 1; txbd++) {
		txbd->txbd_flags = 0;
	}

	/*
	 * Last descriptor has the wrap flag.
	 */
	txbd->txbd_flags = TXBD_W;

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
	sc->sc_imask |= IEVENT_TXF|IEVENT_TXE;

	/*
	 * Restart the transmit at the first descriptor
	 */
	etsec_write(sc, txq->txq_reg_tbase, txq->txq_descmap->dm_segs->ds_addr);
}

static void
pq3etsec_ifstart(struct ifnet *ifp)
{
	struct pq3etsec_softc * const sc = ifp->if_softc;

	atomic_or_uint(&sc->sc_soft_flags, SOFT_TXINTR);
	softint_schedule(sc->sc_soft_ih);
}

static void
pq3etsec_tx_error(
	struct pq3etsec_softc * const sc)
{
	struct pq3etsec_txqueue * const txq = &sc->sc_txq;

	pq3etsec_txq_consume(sc, txq);

	if (pq3etsec_txq_fillable_p(sc, txq))
		sc->sc_if.if_flags &= ~IFF_OACTIVE;
	if (sc->sc_txerrors & (IEVENT_LC|IEVENT_CRL|IEVENT_XFUN|IEVENT_BABT)) {
	} else if (sc->sc_txerrors & IEVENT_EBERR) {
	}

	if (pq3etsec_txq_active_p(sc, txq))
		etsec_write(sc, TSTAT, TSTAT_THLT & txq->txq_qmask);
	if (!pq3etsec_txq_enqueue(sc, txq)) {
		sc->sc_ev_tx_stall.ev_count++;
		sc->sc_if.if_flags |= IFF_OACTIVE;
	}

	sc->sc_txerrors = 0;
}

int
pq3etsec_tx_intr(void *arg)
{
	struct pq3etsec_softc * const sc = arg;

	sc->sc_ev_tx_intr.ev_count++;

	uint32_t ievent = etsec_read(sc, IEVENT);
	ievent &= IEVENT_TXF|IEVENT_TXB;
	etsec_write(sc, IEVENT, ievent);	/* write 1 to clear */

#if 0
	aprint_normal_dev(sc->sc_dev, "%s: ievent=%#x imask=%#x\n", 
	    __func__, ievent, etsec_read(sc, IMASK));
#endif

	if (ievent == 0)
		return 0;

	sc->sc_imask &= ~(IEVENT_TXF|IEVENT_TXB);
	atomic_or_uint(&sc->sc_soft_flags, SOFT_TXINTR);
	etsec_write(sc, IMASK, sc->sc_imask);
	softint_schedule(sc->sc_soft_ih);
	return 1;
}

int
pq3etsec_rx_intr(void *arg)
{
	struct pq3etsec_softc * const sc = arg;

	sc->sc_ev_rx_intr.ev_count++;

	uint32_t ievent = etsec_read(sc, IEVENT);
	ievent &= IEVENT_RXF|IEVENT_RXB;
	etsec_write(sc, IEVENT, ievent);	/* write 1 to clear */
	if (ievent == 0)
		return 0;

#if 0
	aprint_normal_dev(sc->sc_dev, "%s: ievent=%#x\n", __func__, ievent);
#endif

	sc->sc_imask &= ~(IEVENT_RXF|IEVENT_RXB);
	atomic_or_uint(&sc->sc_soft_flags, SOFT_RXINTR);
	etsec_write(sc, IMASK, sc->sc_imask);
	softint_schedule(sc->sc_soft_ih);
	return 1;
}

int
pq3etsec_error_intr(void *arg)
{
	struct pq3etsec_softc * const sc = arg;

	sc->sc_ev_error_intr.ev_count++;

	for (int rv = 0, soft_flags = 0;; rv = 1) {
		uint32_t ievent = etsec_read(sc, IEVENT);
		ievent &= ~(IEVENT_RXF|IEVENT_RXB|IEVENT_TXF|IEVENT_TXB);
		etsec_write(sc, IEVENT, ievent);	/* write 1 to clear */
		if (ievent == 0) {
			if (soft_flags) {
				atomic_or_uint(&sc->sc_soft_flags, soft_flags);
				softint_schedule(sc->sc_soft_ih);
			}
			return rv;
		}
#if 0
		aprint_normal_dev(sc->sc_dev, "%s: ievent=%#x imask=%#x\n", 
		    __func__, ievent, etsec_read(sc, IMASK));
#endif

		if (ievent & (IEVENT_GRSC|IEVENT_GTSC)) {
			sc->sc_imask &= ~(IEVENT_GRSC|IEVENT_GTSC);
			etsec_write(sc, IMASK, sc->sc_imask);
			wakeup(sc);
		}
		if (ievent & (IEVENT_MMRD|IEVENT_MMWR)) {
			sc->sc_imask &= ~(IEVENT_MMRD|IEVENT_MMWR);
			etsec_write(sc, IMASK, sc->sc_imask);
			wakeup(&sc->sc_mii);
		}
		if (ievent & IEVENT_BSY) {
			soft_flags |= SOFT_RXBSY;
			sc->sc_imask &= ~IEVENT_BSY;
			etsec_write(sc, IMASK, sc->sc_imask);
		}
		if (ievent & IEVENT_TXE) {
			soft_flags |= SOFT_TXERROR;
			sc->sc_imask &= ~IEVENT_TXE;
			sc->sc_txerrors |= ievent;
		}
		if (ievent & IEVENT_TXC) {
			sc->sc_ev_tx_pause.ev_count++;
		}
		if (ievent & IEVENT_RXC) {
			sc->sc_ev_rx_pause.ev_count++;
		}
		if (ievent & IEVENT_DPE) {
			soft_flags |= SOFT_RESET;
			sc->sc_imask &= ~IEVENT_DPE;
			etsec_write(sc, IMASK, sc->sc_imask);
		}
	}
}

void
pq3etsec_soft_intr(void *arg)
{
	struct pq3etsec_softc * const sc = arg;
	struct ifnet * const ifp = &sc->sc_if;

	mutex_enter(sc->sc_lock);

	u_int soft_flags = atomic_swap_uint(&sc->sc_soft_flags, 0);

	sc->sc_ev_soft_intr.ev_count++;

	if (soft_flags & SOFT_RESET) {
		int s = splnet();
		pq3etsec_ifinit(ifp);
		splx(s);
		soft_flags = 0;
	}

	if (soft_flags & SOFT_RXBSY) {
		struct pq3etsec_rxqueue * const rxq = &sc->sc_rxq;
		size_t threshold = 5 * rxq->rxq_threshold / 4;
		if (threshold >= rxq->rxq_last - rxq->rxq_first) {
			threshold = rxq->rxq_last - rxq->rxq_first - 1;
		} else {
			sc->sc_imask |= IEVENT_BSY;
		}
		aprint_normal_dev(sc->sc_dev,
		    "increasing receive buffers from %zu to %zu\n",
		    rxq->rxq_threshold, threshold);
		rxq->rxq_threshold = threshold;
	}

	if ((soft_flags & SOFT_TXINTR)
	    || pq3etsec_txq_active_p(sc, &sc->sc_txq)) {
		/*
		 * Let's do what we came here for.  Consume transmitted
		 * packets off the the transmit ring.
		 */
		if (!pq3etsec_txq_consume(sc, &sc->sc_txq)
		    || !pq3etsec_txq_enqueue(sc, &sc->sc_txq)) {
			sc->sc_ev_tx_stall.ev_count++;
			ifp->if_flags |= IFF_OACTIVE;
		} else {
			ifp->if_flags &= ~IFF_OACTIVE;
		}
		sc->sc_imask |= IEVENT_TXF;
	}

	if (soft_flags & (SOFT_RXINTR|SOFT_RXBSY)) {
		/*
		 * Let's consume 
		 */
		pq3etsec_rxq_consume(sc, &sc->sc_rxq);
		sc->sc_imask |= IEVENT_RXF;
	}

	if (soft_flags & SOFT_TXERROR) {
		pq3etsec_tx_error(sc);
		sc->sc_imask |= IEVENT_TXE;
	}

	if (ifp->if_flags & IFF_RUNNING) {
		pq3etsec_rxq_produce(sc, &sc->sc_rxq);
		etsec_write(sc, IMASK, sc->sc_imask);
	} else {
		KASSERT((soft_flags & SOFT_RXBSY) == 0);
	}

	mutex_exit(sc->sc_lock);
}

static void
pq3etsec_mii_tick(void *arg)
{
	struct pq3etsec_softc * const sc = arg;
	mutex_enter(sc->sc_lock);
	callout_ack(&sc->sc_mii_callout);
	sc->sc_ev_mii_ticks.ev_count++;
#ifdef DEBUG
	uint64_t now = mftb();
	if (now - sc->sc_mii_last_tick < cpu_timebase - 5000) {
		aprint_debug_dev(sc->sc_dev, "%s: diff=%"PRIu64"\n",
		    __func__, now - sc->sc_mii_last_tick);
		callout_stop(&sc->sc_mii_callout);
	}
#endif
	mii_tick(&sc->sc_mii);
	int s = splnet();
	if (sc->sc_soft_flags & SOFT_RESET)
		softint_schedule(sc->sc_soft_ih);
	splx(s);
	callout_schedule(&sc->sc_mii_callout, hz);
#ifdef DEBUG
	sc->sc_mii_last_tick = now;
#endif
	mutex_exit(sc->sc_lock);
}
