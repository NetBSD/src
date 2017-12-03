/* $NetBSD: if_gmc.c,v 1.5.2.2 2017/12/03 11:35:53 jdolecek Exp $ */
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
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

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_gmacreg.h>
#include <arm/gemini/gemini_gmacvar.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_dl.h>

__KERNEL_RCSID(0, "$NetBSD: if_gmc.c,v 1.5.2.2 2017/12/03 11:35:53 jdolecek Exp $");

#define	MAX_TXSEG	32

struct gmc_softc {
	device_t sc_dev;
	struct gmac_softc *sc_psc;
	struct gmc_softc *sc_sibling;
	bus_dma_tag_t sc_dmat;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_dma_ioh;
	bus_space_handle_t sc_gmac_ioh;
	struct ethercom sc_ec;
	struct mii_data sc_mii;
	void *sc_ih;
	bool sc_port1;
	uint8_t sc_phy;
	gmac_hwqueue_t *sc_rxq;
	gmac_hwqueue_t *sc_txq[6];
	callout_t sc_mii_ch;

	uint32_t sc_gmac_status;
	uint32_t sc_gmac_sta_add[3];
	uint32_t sc_gmac_mcast_filter[2];
	uint32_t sc_gmac_rx_filter;
	uint32_t sc_gmac_config[2];
	uint32_t sc_dmavr;

	uint32_t sc_int_mask[5];
	uint32_t sc_int_enabled[5];
};

#define	sc_if	sc_ec.ec_if

static bool
gmc_txqueue(struct gmc_softc *sc, gmac_hwqueue_t *hwq, struct mbuf *m)
{
	bus_dmamap_t map;
	uint32_t desc0, desc1, desc3;
	struct mbuf *last_m, *m0;
	size_t count, i;
	int error;
	gmac_desc_t *d;

	KASSERT(hwq != NULL);

	map = gmac_mapcache_get(hwq->hwq_hqm->hqm_mc);
	if (map == NULL)
		return false;

	for (last_m = NULL, m0 = m, count = 0;
	     m0 != NULL;
	     last_m = m0, m0 = m0->m_next) {
		vaddr_t addr = (uintptr_t)m0->m_data;
		if (m0->m_len == 0)
			continue;
		if (addr & 1) {
			if (last_m != NULL && M_TRAILINGSPACE(last_m) > 0) {
				last_m->m_data[last_m->m_len++] = *m->m_data++;
				m->m_len--;
			} else if (M_TRAILINGSPACE(m0) > 0) {
				memmove(m0->m_data + 1, m0->m_data, m0->m_len);
				m0->m_data++;
			} else if (M_LEADINGSPACE(m0) > 0) {
				memmove(m0->m_data - 1, m0->m_data, m0->m_len);
				m0->m_data--;
			} else {
				panic("gmc_txqueue: odd addr %p", m0->m_data);
			}
		}
		count += ((addr & PGOFSET) + m->m_len + PGOFSET) >> PGSHIFT;
	}

	gmac_hwqueue_sync(hwq);
	if (hwq->hwq_free <= count) {
		gmac_mapcache_put(hwq->hwq_hqm->hqm_mc, map);
		return false;
	}

	error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
	    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "ifstart: load failed: %d\n",
		    error);
		gmac_mapcache_put(hwq->hwq_hqm->hqm_mc, map);
		m_freem(m);
		sc->sc_if.if_oerrors++;
		return true;
	}
	KASSERT(map->dm_nsegs > 0);

	/*
	 * Sync the mbuf contents to memory/cache.
	 */
	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		BUS_DMASYNC_PREWRITE);

	/*
	 * Now we need to load the descriptors...
	 */
	desc0 = map->dm_nsegs << 16;
	desc1 = m->m_pkthdr.len;
	desc3 = DESC3_SOF;
	i = 0;
	d = NULL;
	do {
#if 0
		if (i > 0)
			aprint_debug_dev(sc->sc_dev,
			    "gmac_txqueue: %zu@%p=%#x/%#x/%#x/%#x\n",
			    i-1, d, d->d_desc0, d->d_desc1,
			    d->d_bufaddr, d->d_desc3);
#endif
		d = gmac_hwqueue_desc(hwq, i);
		KASSERT(map->dm_segs[i].ds_len > 0);
		KASSERT((map->dm_segs[i].ds_addr & 1) == 0);
		d->d_desc0 = htole32(map->dm_segs[i].ds_len | desc0);
		d->d_desc1 = htole32(desc1);
		d->d_bufaddr = htole32(map->dm_segs[i].ds_addr);
		d->d_desc3 = htole32(desc3);
		desc3 = 0;
	} while (++i < map->dm_nsegs);

	d->d_desc3 |= htole32(DESC3_EOF|DESC3_EOFIE);
#if 0
	aprint_debug_dev(sc->sc_dev,
	    "gmac_txqueue: %zu@%p=%#x/%#x/%#x/%#x\n",
	    i-1, d, d->d_desc0, d->d_desc1, d->d_bufaddr, d->d_desc3);
#endif
	M_SETCTX(m, map);
	IF_ENQUEUE(&hwq->hwq_ifq, m);
	/*
	 * Last descriptor has been marked.  Give them to the h/w.
	 * This will sync for us.
	 */
	gmac_hwqueue_produce(hwq, map->dm_nsegs);
#if 0
	aprint_debug_dev(sc->sc_dev,
	    "gmac_txqueue: *%zu@%p=%#x/%#x/%#x/%#x\n",
	    i-1, d, d->d_desc0, d->d_desc1, d->d_bufaddr, d->d_desc3);
#endif
	return true;
}

static void
gmc_filter_change(struct gmc_softc *sc)
{
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t mhash[2];
	uint32_t new0, new1, new2;
	const char * const eaddr = CLLADDR(sc->sc_if.if_sadl);

	new0 = eaddr[0] | ((eaddr[1] | (eaddr[2] | (eaddr[3] << 8)) << 8) << 8);
	new1 = eaddr[4] | (eaddr[5] << 8);
	new2 = 0;
	if (sc->sc_gmac_sta_add[0] != new0
	    || sc->sc_gmac_sta_add[1] != new1
	    || sc->sc_gmac_sta_add[2] != new2) {
		bus_space_write_4(sc->sc_iot, sc->sc_gmac_ioh, GMAC_STA_ADD0,
		    new0);
		bus_space_write_4(sc->sc_iot, sc->sc_gmac_ioh, GMAC_STA_ADD1,
		    new1);
		bus_space_write_4(sc->sc_iot, sc->sc_gmac_ioh, GMAC_STA_ADD2,
		    new2);
		sc->sc_gmac_sta_add[0] = new0;
		sc->sc_gmac_sta_add[1] = new1;
		sc->sc_gmac_sta_add[2] = new2;
	}

	mhash[0] = 0;
	mhash[1] = 0;
	ETHER_FIRST_MULTI(step, &sc->sc_ec, enm);
	while (enm != NULL) {
		size_t i;
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			mhash[0] = mhash[1] = 0xffffffff;
			break;
		}
		i = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);
		mhash[(i >> 5) & 1] |= 1 << (i & 31);
		ETHER_NEXT_MULTI(step, enm);
	}

	if (sc->sc_gmac_mcast_filter[0] != mhash[0]
	    || sc->sc_gmac_mcast_filter[1] != mhash[1]) {
		bus_space_write_4(sc->sc_iot, sc->sc_gmac_ioh,
		    GMAC_MCAST_FILTER0, mhash[0]);
		bus_space_write_4(sc->sc_iot, sc->sc_gmac_ioh,
		    GMAC_MCAST_FILTER1, mhash[1]);
		sc->sc_gmac_mcast_filter[0] = mhash[0];
		sc->sc_gmac_mcast_filter[1] = mhash[1];
	}

	new0 = sc->sc_gmac_rx_filter & ~RXFILTER_PROMISC;
	new0 |= RXFILTER_BROADCAST | RXFILTER_UNICAST | RXFILTER_MULTICAST;
	if (sc->sc_if.if_flags & IFF_PROMISC)
		new0 |= RXFILTER_PROMISC;

	if (new0 != sc->sc_gmac_rx_filter) {
		bus_space_write_4(sc->sc_iot, sc->sc_gmac_ioh, GMAC_RX_FILTER,
		    new0);
		sc->sc_gmac_rx_filter = new0;
	}
}

static void
gmc_mii_tick(void *arg)
{
	struct gmc_softc * const sc = arg;
	struct gmac_softc * const psc = sc->sc_psc;
	int s = splnet();

	/*
	 * If we had to increase the number of receive mbufs due to fifo
	 * overflows, we need a way to decrease them.  So every second we
	 * recieve less than or equal to MIN_RXMAPS packets, we decrement
	 * swfree_min until it returns to MIN_RXMAPS.
	 */
	if (psc->sc_rxpkts_per_sec <= MIN_RXMAPS
	    && psc->sc_swfree_min > MIN_RXMAPS) {
		psc->sc_swfree_min--;
		gmac_swfree_min_update(psc);
	}
	/*
	 * If only one GMAC is running or this is port0, reset the count.
	 */
	if (psc->sc_running != 3 || !sc->sc_port1)
		psc->sc_rxpkts_per_sec = 0;

	mii_tick(&sc->sc_mii);
	if (sc->sc_if.if_flags & IFF_RUNNING)
		callout_schedule(&sc->sc_mii_ch, hz);

	splx(s);
}

static int
gmc_mediachange(struct ifnet *ifp)
{
	struct gmc_softc * const sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_UP) == 0)
		return 0;

	return mii_mediachg(&sc->sc_mii);
}

static void
gmc_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct gmc_softc * const sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

static void
gmc_mii_statchg(struct ifnet *ifp)
{
	struct gmc_softc * const sc = ifp->if_softc;
	uint32_t gmac_status;
	
	gmac_status = sc->sc_gmac_status;

	gmac_status &= ~STATUS_PHYMODE_MASK;
	gmac_status |= STATUS_PHYMODE_RGMII_A;

	gmac_status &= ~STATUS_SPEED_MASK;
	if (IFM_SUBTYPE(sc->sc_mii.mii_media_active) == IFM_1000_T) {
		gmac_status |= STATUS_SPEED_1000M;
	} else if (IFM_SUBTYPE(sc->sc_mii.mii_media_active) == IFM_100_TX) {
		gmac_status |= STATUS_SPEED_100M;
	} else {
		gmac_status |= STATUS_SPEED_10M;
	}

        if (sc->sc_mii.mii_media_active & IFM_FDX)
		gmac_status |= STATUS_DUPLEX_FULL;
	else
		gmac_status &= ~STATUS_DUPLEX_FULL;

        if (sc->sc_mii.mii_media_status & IFM_ACTIVE)
		gmac_status |= STATUS_LINK_ON;
	else
		gmac_status &= ~STATUS_LINK_ON;

	if (sc->sc_gmac_status != gmac_status) {
		aprint_debug_dev(sc->sc_dev,
		    "status change old=%#x new=%#x active=%#x\n",
		    sc->sc_gmac_status, gmac_status,
		    sc->sc_mii.mii_media_active);
		sc->sc_gmac_status = gmac_status;
		bus_space_write_4(sc->sc_iot, sc->sc_gmac_ioh, GMAC_STATUS,
		    sc->sc_gmac_status);
	}

	(*sc->sc_mii.mii_writereg)(sc->sc_dev, sc->sc_phy, 0x0018, 0x0041);
}

static int
gmc_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct gmc_softc * const sc = ifp->if_softc;
	struct ifreq * const ifr = data;
	int s;
	int error;
	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING) {
				/*
				 * If the interface is running, we have to 
				 * update its multicast filter.
				 */
				gmc_filter_change(sc);
			}
			error = 0;
		}
	}

	splx(s);
	return error;
}

static void
gmc_ifstart(struct ifnet *ifp)
{
	struct gmc_softc * const sc = ifp->if_softc;

#if 0
	if ((sc->sc_gmac_status & STATUS_LINK_ON) == 0)
		return;
#endif
	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	for (;;) {
		struct mbuf *m;
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;
		if (!gmc_txqueue(sc, sc->sc_txq[0], m)) {
			IF_PREPEND(&ifp->if_snd, m);
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
	}
}

static void
gmc_ifstop(struct ifnet *ifp, int disable)
{
	struct gmc_softc * const sc = ifp->if_softc;
	struct gmac_softc * const psc = sc->sc_psc;

	psc->sc_running &= ~(sc->sc_port1 ? 2 : 1);
	psc->sc_int_enabled[0] &= ~sc->sc_int_enabled[0];
	psc->sc_int_enabled[1] &= ~sc->sc_int_enabled[1];
	psc->sc_int_enabled[2] &= ~sc->sc_int_enabled[2];
	psc->sc_int_enabled[3] &= ~sc->sc_int_enabled[3];
	psc->sc_int_enabled[4] &= ~sc->sc_int_enabled[4] | INT4_SW_FREEQ_EMPTY;
	if (psc->sc_running == 0) {
		psc->sc_int_enabled[4] &= ~INT4_SW_FREEQ_EMPTY;
		KASSERT(psc->sc_int_enabled[0] == 0);
		KASSERT(psc->sc_int_enabled[1] == 0);
		KASSERT(psc->sc_int_enabled[2] == 0);
		KASSERT(psc->sc_int_enabled[3] == 0);
		KASSERT(psc->sc_int_enabled[4] == 0);
	} else if (((psc->sc_int_select[4] & INT4_SW_FREEQ_EMPTY) != 0)
			== sc->sc_port1) {
		psc->sc_int_select[4] &= ~INT4_SW_FREEQ_EMPTY;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_MASK,
		    psc->sc_int_select[4]);
	}
	gmac_intr_update(psc);
	if (disable) {
#if 0
		if (psc->sc_running == 0) {
			gmac_mapcache_destroy(&psc->sc_txmaps);
			gmac_mapcache_destroy(&psc->sc_rxmaps);
		}
#endif
	}
}

static int
gmc_ifinit(struct ifnet *ifp)
{
	struct gmc_softc * const sc = ifp->if_softc;
	struct gmac_softc * const psc = sc->sc_psc;
	uint32_t new, mask;

	gmac_mapcache_fill(psc->sc_rxmaps, MIN_RXMAPS);
	gmac_mapcache_fill(psc->sc_txmaps, MIN_TXMAPS);

	if (sc->sc_rxq == NULL) {
		gmac_hwqmem_t *hqm;
		hqm = gmac_hwqmem_create(psc->sc_rxmaps, 16, /*RXQ_NDESCS,*/ 1,
		   HQM_CONSUMER|HQM_RX);
		sc->sc_rxq = gmac_hwqueue_create(hqm, sc->sc_iot,
		    sc->sc_ioh, GMAC_DEF_RXQn_RWPTR(sc->sc_port1),
		    GMAC_DEF_RXQn_BASE(sc->sc_port1), 0);
		if (sc->sc_rxq == NULL) {
			gmac_hwqmem_destroy(hqm);
			goto failed;
		}
		sc->sc_rxq->hwq_ifp = ifp;
		sc->sc_rxq->hwq_producer = psc->sc_swfreeq;
	}

	if (sc->sc_txq[0] == NULL) {
		gmac_hwqueue_t *hwq, *last_hwq;
		gmac_hwqmem_t *hqm;
		size_t i;

		hqm = gmac_hwqmem_create(psc->sc_txmaps, TXQ_NDESCS, 6,
		   HQM_PRODUCER|HQM_TX);
		KASSERT(hqm != NULL);
		for (i = 0; i < __arraycount(sc->sc_txq); i++) {
			sc->sc_txq[i] = gmac_hwqueue_create(hqm, sc->sc_iot,
			    sc->sc_dma_ioh, GMAC_SW_TX_Qn_RWPTR(i),
			    GMAC_SW_TX_Q_BASE, i);
			if (sc->sc_txq[i] == NULL) {
				if (i == 0)
					gmac_hwqmem_destroy(hqm);
				goto failed;
			}
			sc->sc_txq[i]->hwq_ifp = ifp;

			last_hwq = NULL;
			SLIST_FOREACH(hwq, &psc->sc_hwfreeq->hwq_producers,
			    hwq_link) {
				if (sc->sc_txq[i]->hwq_qoff < hwq->hwq_qoff)
					break;
				last_hwq = hwq;
			}
			if (last_hwq == NULL)
				SLIST_INSERT_HEAD(
				    &psc->sc_hwfreeq->hwq_producers,
				    sc->sc_txq[i], hwq_link);
			else
				SLIST_INSERT_AFTER(last_hwq, sc->sc_txq[i],
				    hwq_link);
		}
	}

	gmc_filter_change(sc);

	mask = DMAVR_LOOPBACK|DMAVR_DROP_SMALL_ACK|DMAVR_EXTRABYTES_MASK
	    |DMAVR_RXBURSTSIZE_MASK|DMAVR_RXBUSWIDTH_MASK
	    |DMAVR_TXBURSTSIZE_MASK|DMAVR_TXBUSWIDTH_MASK;
	new = DMAVR_RXDMA_ENABLE|DMAVR_TXDMA_ENABLE
	    |DMAVR_EXTRABYTES(2)
	    |DMAVR_RXBURSTSIZE(DMAVR_BURSTSIZE_32W)
	    |DMAVR_RXBUSWIDTH(DMAVR_BUSWIDTH_32BITS)
	    |DMAVR_TXBURSTSIZE(DMAVR_BURSTSIZE_32W)
	    |DMAVR_TXBUSWIDTH(DMAVR_BUSWIDTH_32BITS);
	new |= sc->sc_dmavr & ~mask;
	if (sc->sc_dmavr != new) {
		sc->sc_dmavr = new;
		bus_space_write_4(sc->sc_iot, sc->sc_dma_ioh, GMAC_DMAVR,
		    sc->sc_dmavr);
		aprint_debug_dev(sc->sc_dev, "gmc_ifinit: dmavr=%#x/%#x\n",
		    sc->sc_dmavr,
		    bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh, GMAC_DMAVR));
	}

	mask = CONFIG0_MAXLEN_MASK|CONFIG0_TX_DISABLE|CONFIG0_RX_DISABLE
	    |CONFIG0_LOOPBACK|/*CONFIG0_SIM_TEST|*/CONFIG0_INVERSE_RXC_RGMII
	    |CONFIG0_RGMII_INBAND_STATUS_ENABLE;
	new = CONFIG0_MAXLEN(CONFIG0_MAXLEN_1536)|CONFIG0_R_LATCHED_MMII;
	new |= (sc->sc_gmac_config[0] & ~mask);
	if (sc->sc_gmac_config[0] != new) {
		sc->sc_gmac_config[0] = new;
		bus_space_write_4(sc->sc_iot, sc->sc_gmac_ioh, GMAC_CONFIG0,
		    sc->sc_gmac_config[0]);
		aprint_debug_dev(sc->sc_dev, "gmc_ifinit: config0=%#x/%#x\n",
		    sc->sc_gmac_config[0],
		    bus_space_read_4(sc->sc_iot, sc->sc_gmac_ioh, GMAC_CONFIG0));
	}

	psc->sc_rxpkts_per_sec +=
	    gmac_rxproduce(psc->sc_swfreeq, psc->sc_swfree_min);

	/*
	 * If we will be the only active interface, make sure the sw freeq
	 * interrupt gets routed to use.
	 */
	if (psc->sc_running == 0
	    && (((psc->sc_int_select[4] & INT4_SW_FREEQ_EMPTY) != 0) != sc->sc_port1)) {
		psc->sc_int_select[4] ^= INT4_SW_FREEQ_EMPTY;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_MASK,
		    psc->sc_int_select[4]);
	}
	sc->sc_int_enabled[0] = sc->sc_int_mask[0]
	    & (INT0_TXDERR|INT0_TXPERR|INT0_RXDERR|INT0_RXPERR|INT0_SWTXQ_EOF);
	sc->sc_int_enabled[1] = sc->sc_int_mask[1] & INT1_DEF_RXQ_EOF;
	sc->sc_int_enabled[4] = INT4_SW_FREEQ_EMPTY | (sc->sc_int_mask[4]
	    & (INT4_TX_FAIL|INT4_MIB_HEMIWRAP|INT4_RX_FIFO_OVRN
	       |INT4_RGMII_STSCHG));

	psc->sc_int_enabled[0] |= sc->sc_int_enabled[0];
	psc->sc_int_enabled[1] |= sc->sc_int_enabled[1];
	psc->sc_int_enabled[4] |= sc->sc_int_enabled[4];

	gmac_intr_update(psc);

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		mii_tick(&sc->sc_mii);

	ifp->if_flags |= IFF_RUNNING;
	psc->sc_running |= (sc->sc_port1 ? 2 : 1);

	callout_schedule(&sc->sc_mii_ch, hz);
	
	return 0;

failed:
	gmc_ifstop(ifp, true);
	return ENOMEM;
}

static int
gmc_intr(void *arg)
{
	struct gmc_softc * const sc = arg;
	uint32_t int0_status, int1_status, int4_status;
	uint32_t status;
	bool do_ifstart = false;
	int rv = 0;

	aprint_debug_dev(sc->sc_dev, "gmac_intr: entry\n");

	int0_status = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    GMAC_INT0_STATUS);
	int1_status = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    GMAC_INT1_STATUS);
	int4_status = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    GMAC_INT4_STATUS);

	aprint_debug_dev(sc->sc_dev, "gmac_intr: sts=%#x/%#x/%#x/%#x/%#x\n",
	    int0_status, int1_status,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_STATUS),
	    int4_status);

#if 0
	aprint_debug_dev(sc->sc_dev, "gmac_intr: mask=%#x/%#x/%#x/%#x/%#x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_MASK),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_MASK),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_MASK),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_MASK),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_MASK));
#endif

	status = int0_status & sc->sc_int_mask[0];
	if (status & (INT0_TXDERR|INT0_TXPERR)) {
		aprint_error_dev(sc->sc_dev,
		    "transmit%s%s error: %#x %08x bufaddr %#x\n",
		    status & INT0_TXDERR ? " data" : "",
		    status & INT0_TXPERR ? " protocol" : "",
		bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh,
		    GMAC_DMA_TX_CUR_DESC),
		bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh,
		    GMAC_SW_TX_Q0_RWPTR),
		bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh,
		    GMAC_DMA_TX_DESC2));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_STATUS,
		    status & (INT0_TXDERR|INT0_TXPERR));
		Debugger();
	}
	if (status & (INT0_RXDERR|INT0_RXPERR)) {
		aprint_error_dev(sc->sc_dev,
		    "receive%s%s error: %#x %#x=%#x/%#x/%#x/%#x\n",
		    status & INT0_RXDERR ? " data" : "",
		    status & INT0_RXPERR ? " protocol" : "",
		bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh,
		    GMAC_DMA_RX_CUR_DESC),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    GMAC_SWFREEQ_RWPTR),
		bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh,
		    GMAC_DMA_RX_DESC0),
		bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh,
		    GMAC_DMA_RX_DESC1),
		bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh,
		    GMAC_DMA_RX_DESC2),
		bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh,
		    GMAC_DMA_RX_DESC3));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_STATUS,
		    status & (INT0_RXDERR|INT0_RXPERR));
		    Debugger();
	}
	if (status & INT0_SWTXQ_EOF) {
		status &= INT0_SWTXQ_EOF;
		for (int i = 0; status && i < __arraycount(sc->sc_txq); i++) {
			if (status & INT0_SWTXQn_EOF(i)) {
				gmac_hwqueue_sync(sc->sc_txq[i]);
				bus_space_write_4(sc->sc_iot, sc->sc_ioh,
				    GMAC_INT0_STATUS,
				    sc->sc_int_mask[0] & (INT0_SWTXQn_EOF(i)|INT0_SWTXQn_FIN(i)));
				status &= ~INT0_SWTXQn_EOF(i);
			}
		}
		do_ifstart = true;
		rv = 1;
	}

	if (int4_status & INT4_SW_FREEQ_EMPTY) {
		struct gmac_softc * const psc = sc->sc_psc;
		psc->sc_rxpkts_per_sec +=
		    gmac_rxproduce(psc->sc_swfreeq, psc->sc_swfree_min);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_STATUS,
		    status & INT4_SW_FREEQ_EMPTY);
		rv = 1;
	}

	status = int1_status & sc->sc_int_mask[1];
	if (status & INT1_DEF_RXQ_EOF) {
		struct gmac_softc * const psc = sc->sc_psc;
		psc->sc_rxpkts_per_sec +=
		    gmac_hwqueue_consume(sc->sc_rxq, psc->sc_swfree_min);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_STATUS,
		    status & INT1_DEF_RXQ_EOF);
		rv = 1;
	}

	status = int4_status & sc->sc_int_enabled[4];
	if (status & INT4_TX_FAIL) {
	}
	if (status & INT4_MIB_HEMIWRAP) {
	}
	if (status & INT4_RX_XON) {
	}
	if (status & INT4_RX_XOFF) {
	}
	if (status & INT4_TX_XON) {
	}
	if (status & INT4_TX_XOFF) {
	}
	if (status & INT4_RX_FIFO_OVRN) {
#if 0
		if (sc->sc_psc->sc_swfree_min < MAX_RXMAPS) {
			sc->sc_psc->sc_swfree_min++;
			gmac_swfree_min_update(psc);
		}
#endif
		sc->sc_if.if_ierrors++;
	}
	if (status & INT4_RGMII_STSCHG) {
		mii_pollstat(&sc->sc_mii);
	}
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_STATUS, status);

	if (do_ifstart)
		if_schedule_deferred_start(&sc->sc_if);

	aprint_debug_dev(sc->sc_dev, "gmac_intr: sts=%#x/%#x/%#x/%#x/%#x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_STATUS));
	aprint_debug_dev(sc->sc_dev, "gmac_intr: exit rv=%d\n", rv);
	return rv;
}

static int
gmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gmac_softc *psc = device_private(parent);
	struct gmac_attach_args *gma = aux;

	if ((unsigned int)gma->gma_phy > 31)
		return 0;
	if ((unsigned int)gma->gma_port > 1)
		return 0;
	if (gma->gma_intr < 1 || gma->gma_intr > 2)
		return 0;

	if (psc->sc_ports & (1 << gma->gma_port))
		return 0;

	return 1;
}

static void
gmc_attach(device_t parent, device_t self, void *aux)
{
	struct gmac_softc * const psc = device_private(parent);
	struct gmc_softc * const sc = device_private(self);
	struct gmac_attach_args *gma = aux;
	struct ifnet * const ifp = &sc->sc_if;
	static const char eaddrs[2][6] = {
		"\x0\x52\xc3\x11\x22\x33",
		"\x0\x52\xc3\x44\x55\x66",
	};

	psc->sc_ports |= 1 << gma->gma_port;
	sc->sc_port1 = (gma->gma_port == 1);
	sc->sc_phy = gma->gma_phy;

	sc->sc_dev = self;
	sc->sc_psc = psc;
	sc->sc_iot = psc->sc_iot;
	sc->sc_ioh = psc->sc_ioh;
	sc->sc_dmat = psc->sc_dmat;

	bus_space_subregion(sc->sc_iot, sc->sc_ioh, 
	    GMAC_PORTn_DMA_OFFSET(gma->gma_port), GMAC_PORTn_DMA_SIZE,
	    &sc->sc_dma_ioh);
	bus_space_subregion(sc->sc_iot, sc->sc_ioh, 
	    GMAC_PORTn_GMAC_OFFSET(gma->gma_port), GMAC_PORTn_GMAC_SIZE,
	    &sc->sc_gmac_ioh);
	aprint_normal("\n");
	aprint_naive("\n");

	strlcpy(ifp->if_xname, device_xname(self), sizeof(ifp->if_xname));
	ifp->if_flags = IFF_SIMPLEX|IFF_MULTICAST|IFF_BROADCAST;
	ifp->if_softc = sc;
	ifp->if_ioctl = gmc_ifioctl;
	ifp->if_stop  = gmc_ifstop;
	ifp->if_start = gmc_ifstart;
	ifp->if_init  = gmc_ifinit;

	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_ec.ec_capabilities = ETHERCAP_VLAN_MTU | ETHERCAP_JUMBO_MTU;
	sc->sc_ec.ec_mii = &sc->sc_mii;

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_statchg = gmc_mii_statchg;
	sc->sc_mii.mii_readreg = gma->gma_mii_readreg;
	sc->sc_mii.mii_writereg = gma->gma_mii_writereg;

	ifmedia_init(&sc->sc_mii.mii_media, 0, gmc_mediachange,
	   gmc_mediastatus);

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, eaddrs[gma->gma_port]);
	mii_attach(sc->sc_dev, &sc->sc_mii, 0xffffffff,
	    gma->gma_phy, MII_OFFSET_ANY, 0);

	if (LIST_EMPTY(&sc->sc_mii.mii_phys)) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else {
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
//		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX);
	}

	sc->sc_gmac_status = bus_space_read_4(sc->sc_iot, sc->sc_gmac_ioh,
	    GMAC_STATUS);
	sc->sc_gmac_sta_add[0] = bus_space_read_4(sc->sc_iot, sc->sc_gmac_ioh,
	    GMAC_STA_ADD0);
	sc->sc_gmac_sta_add[1] = bus_space_read_4(sc->sc_iot, sc->sc_gmac_ioh,
	    GMAC_STA_ADD1);
	sc->sc_gmac_sta_add[2] = bus_space_read_4(sc->sc_iot, sc->sc_gmac_ioh,
	    GMAC_STA_ADD2);
	sc->sc_gmac_mcast_filter[0] = bus_space_read_4(sc->sc_iot,
	    sc->sc_gmac_ioh, GMAC_MCAST_FILTER0);
	sc->sc_gmac_mcast_filter[1] = bus_space_read_4(sc->sc_iot,
	    sc->sc_gmac_ioh, GMAC_MCAST_FILTER1);
	sc->sc_gmac_rx_filter = bus_space_read_4(sc->sc_iot, sc->sc_gmac_ioh,
	    GMAC_RX_FILTER);
	sc->sc_gmac_config[0] = bus_space_read_4(sc->sc_iot, sc->sc_gmac_ioh,
	    GMAC_CONFIG0);
	sc->sc_dmavr = bus_space_read_4(sc->sc_iot, sc->sc_dma_ioh, GMAC_DMAVR);

	/* sc->sc_int_enabled is already zeroed */
	sc->sc_int_mask[0] = (sc->sc_port1 ? INT0_GMAC1 : INT0_GMAC0);
	sc->sc_int_mask[1] = (sc->sc_port1 ? INT1_GMAC1 : INT1_GMAC0);
	sc->sc_int_mask[2] = (sc->sc_port1 ? INT2_GMAC1 : INT2_GMAC0);
	sc->sc_int_mask[3] = (sc->sc_port1 ? INT3_GMAC1 : INT3_GMAC0);
	sc->sc_int_mask[4] = (sc->sc_port1 ? INT4_GMAC1 : INT4_GMAC0);

	if (!sc->sc_port1) {
	sc->sc_ih = intr_establish(gma->gma_intr, IPL_NET, IST_LEVEL_HIGH,
	    gmc_intr, sc);
	KASSERT(sc->sc_ih != NULL);
	}

	callout_init(&sc->sc_mii_ch, 0);
	callout_setfunc(&sc->sc_mii_ch, gmc_mii_tick, sc);

	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	     ether_sprintf(CLLADDR(sc->sc_if.if_sadl)));
}

CFATTACH_DECL_NEW(gmc, sizeof(struct gmc_softc),
    gmc_match, gmc_attach, NULL, NULL);
