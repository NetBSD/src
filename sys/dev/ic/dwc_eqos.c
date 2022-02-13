/* $NetBSD: dwc_eqos.c,v 1.5 2022/02/13 18:29:15 riastradh Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * DesignWare Ethernet Quality-of-Service controller
 */

#include "opt_net_mpsafe.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwc_eqos.c,v 1.5 2022/02/13 18:29:15 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/callout.h>
#include <sys/cprng.h>
#include <sys/evcnt.h>

#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_eqos_reg.h>
#include <dev/ic/dwc_eqos_var.h>

CTASSERT(MCLBYTES == 2048);
#ifdef EQOS_DEBUG
#define	DPRINTF(...)	printf(##__VA_ARGS__)
#else
#define	DPRINTF(...)	((void)0)
#endif

#ifdef NET_MPSAFE
#define	EQOS_MPSAFE		1
#define	CALLOUT_FLAGS		CALLOUT_MPSAFE
#else
#define	CALLOUT_FLAGS		0
#endif

#define	DESC_BOUNDARY		(1ULL << 32)
#define	DESC_ALIGN		sizeof(struct eqos_dma_desc)
#define	TX_DESC_COUNT		EQOS_DMA_DESC_COUNT
#define	TX_DESC_SIZE		(TX_DESC_COUNT * DESC_ALIGN)
#define	RX_DESC_COUNT		EQOS_DMA_DESC_COUNT
#define	RX_DESC_SIZE		(RX_DESC_COUNT * DESC_ALIGN)
#define	MII_BUSY_RETRY		1000

#define	DESC_OFF(n)		((n) * sizeof(struct eqos_dma_desc))
#define	TX_SKIP(n, o)		(((n) + (o)) % TX_DESC_COUNT)
#define	TX_NEXT(n)		TX_SKIP(n, 1)
#define	RX_NEXT(n)		(((n) + 1) % RX_DESC_COUNT)

#define	TX_MAX_SEGS		128

#define	EQOS_LOCK(sc)			mutex_enter(&(sc)->sc_lock)
#define	EQOS_UNLOCK(sc)			mutex_exit(&(sc)->sc_lock)
#define	EQOS_ASSERT_LOCKED(sc)		KASSERT(mutex_owned(&(sc)->sc_lock))

#define	EQOS_TXLOCK(sc)			mutex_enter(&(sc)->sc_txlock)
#define	EQOS_TXUNLOCK(sc)		mutex_exit(&(sc)->sc_txlock)
#define	EQOS_ASSERT_TXLOCKED(sc)	KASSERT(mutex_owned(&(sc)->sc_txlock))

#define	EQOS_HW_FEATURE_ADDR64_32BIT(sc)				\
	(((sc)->sc_hw_feature[1] & GMAC_MAC_HW_FEATURE1_ADDR64_MASK) ==	\
	    GMAC_MAC_HW_FEATURE1_ADDR64_32BIT)


#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#define STUB(...)				\
	printf("%s: TODO\n", __func__);	\

static int
eqos_mii_readreg(device_t dev, int phy, int reg, uint16_t *val)
{
	struct eqos_softc *sc = device_private(dev);
	uint32_t addr;
	int retry;

	addr = sc->sc_clock_range |
	    (phy << GMAC_MAC_MDIO_ADDRESS_PA_SHIFT) |
	    (reg << GMAC_MAC_MDIO_ADDRESS_RDA_SHIFT) |
	    GMAC_MAC_MDIO_ADDRESS_GOC_READ |
	    GMAC_MAC_MDIO_ADDRESS_GB;
	WR4(sc, GMAC_MAC_MDIO_ADDRESS, addr);

	delay(10000);

	for (retry = MII_BUSY_RETRY; retry > 0; retry--) {
		addr = RD4(sc, GMAC_MAC_MDIO_ADDRESS);
		if ((addr & GMAC_MAC_MDIO_ADDRESS_GB) == 0) {
			*val = RD4(sc, GMAC_MAC_MDIO_DATA) & 0xFFFF;
			break;
		}
		delay(10);
	}
	if (retry == 0) {
		device_printf(dev, "phy read timeout, phy=%d reg=%d\n",
		    phy, reg);
		return ETIMEDOUT;
	}

	return 0;
}

static int
eqos_mii_writereg(device_t dev, int phy, int reg, uint16_t val)
{
	struct eqos_softc *sc = device_private(dev);
	uint32_t addr;
	int retry;

	WR4(sc, GMAC_MAC_MDIO_DATA, val);

	addr = sc->sc_clock_range |
	    (phy << GMAC_MAC_MDIO_ADDRESS_PA_SHIFT) |
	    (reg << GMAC_MAC_MDIO_ADDRESS_RDA_SHIFT) |
	    GMAC_MAC_MDIO_ADDRESS_GOC_WRITE |
	    GMAC_MAC_MDIO_ADDRESS_GB;
	WR4(sc, GMAC_MAC_MDIO_ADDRESS, addr);

	delay(10000);

	for (retry = MII_BUSY_RETRY; retry > 0; retry--) {
		addr = RD4(sc, GMAC_MAC_MDIO_ADDRESS);
		if ((addr & GMAC_MAC_MDIO_ADDRESS_GB) == 0) {
			break;
		}
		delay(10);
	}
	if (retry == 0) {
		device_printf(dev, "phy write timeout, phy=%d reg=%d\n",
		    phy, reg);
		return ETIMEDOUT;
	}

	return 0;
}

static void
eqos_update_link(struct eqos_softc *sc)
{
	struct mii_data *mii = &sc->sc_mii;
	uint64_t baudrate;
	uint32_t conf;

	baudrate = ifmedia_baudrate(mii->mii_media_active);

	conf = RD4(sc, GMAC_MAC_CONFIGURATION);
	switch (baudrate) {
	case IF_Mbps(10):
		conf |= GMAC_MAC_CONFIGURATION_PS;
		conf &= ~GMAC_MAC_CONFIGURATION_FES;
		break;
	case IF_Mbps(100):
		conf |= GMAC_MAC_CONFIGURATION_PS;
		conf |= GMAC_MAC_CONFIGURATION_FES;
		break;
	case IF_Gbps(1):
		conf &= ~GMAC_MAC_CONFIGURATION_PS;
		conf &= ~GMAC_MAC_CONFIGURATION_FES;
		break;
	case IF_Mbps(2500ULL):
		conf &= ~GMAC_MAC_CONFIGURATION_PS;
		conf |= GMAC_MAC_CONFIGURATION_FES;
		break;
	}

	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_FDX) != 0) {
		conf |= GMAC_MAC_CONFIGURATION_DM;
	} else {
		conf &= ~GMAC_MAC_CONFIGURATION_DM;
	}

	WR4(sc, GMAC_MAC_CONFIGURATION, conf);
}

static void
eqos_mii_statchg(struct ifnet *ifp)
{
	struct eqos_softc * const sc = ifp->if_softc;

	eqos_update_link(sc);
}

static void
eqos_dma_sync(struct eqos_softc *sc, bus_dmamap_t map,
    u_int start, u_int end, u_int total, int flags)
{
	if (end > start) {
		bus_dmamap_sync(sc->sc_dmat, map, DESC_OFF(start),
		    DESC_OFF(end) - DESC_OFF(start), flags);
	} else {
		bus_dmamap_sync(sc->sc_dmat, map, DESC_OFF(start),
		    DESC_OFF(total) - DESC_OFF(start), flags);
		if (DESC_OFF(end) - DESC_OFF(0) > 0) {
			bus_dmamap_sync(sc->sc_dmat, map, DESC_OFF(0),
			    DESC_OFF(end) - DESC_OFF(0), flags);
		}
	}
}

static void
eqos_setup_txdesc(struct eqos_softc *sc, int index, int flags,
    bus_addr_t paddr, u_int len, u_int total_len)
{
	uint32_t tdes2, tdes3;

	if (paddr == 0 || len == 0) {
		KASSERT(flags == 0);
		tdes2 = 0;
		tdes3 = 0;
		--sc->sc_tx.queued;
	} else {
		tdes2 = (flags & EQOS_TDES3_LD) ? EQOS_TDES2_IOC : 0;
		tdes3 = flags;
		++sc->sc_tx.queued;
	}

	KASSERT(!EQOS_HW_FEATURE_ADDR64_32BIT(sc) || (paddr >> 32) == 0);

	sc->sc_tx.desc_ring[index].tdes0 = htole32((uint32_t)paddr);
	sc->sc_tx.desc_ring[index].tdes1 = htole32((uint32_t)(paddr >> 32));
	sc->sc_tx.desc_ring[index].tdes2 = htole32(tdes2 | len);
	sc->sc_tx.desc_ring[index].tdes3 = htole32(tdes3 | total_len);
}

static int
eqos_setup_txbuf(struct eqos_softc *sc, int index, struct mbuf *m)
{
	bus_dma_segment_t *segs;
	int error, nsegs, cur, i;
	uint32_t flags;
	bool nospace;

	/* at least one descriptor free ? */
	if (sc->sc_tx.queued >= TX_DESC_COUNT - 1)
		return -1;

	error = bus_dmamap_load_mbuf(sc->sc_dmat,
	    sc->sc_tx.buf_map[index].map, m, BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	if (error == EFBIG) {
		device_printf(sc->sc_dev,
		    "TX packet needs too many DMA segments, dropping...\n");
		return -2;
	}
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "TX packet cannot be mapped, retried...\n");
		return 0;
	}

	segs = sc->sc_tx.buf_map[index].map->dm_segs;
	nsegs = sc->sc_tx.buf_map[index].map->dm_nsegs;

	nospace = sc->sc_tx.queued >= TX_DESC_COUNT - nsegs;
	if (nospace) {
		bus_dmamap_unload(sc->sc_dmat,
		    sc->sc_tx.buf_map[index].map);
		/* XXX coalesce and retry ? */
		return -1;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx.buf_map[index].map,
	    0, sc->sc_tx.buf_map[index].map->dm_mapsize, BUS_DMASYNC_PREWRITE);

	/* stored in same index as loaded map */
	sc->sc_tx.buf_map[index].mbuf = m;

	flags = EQOS_TDES3_FD;

	for (cur = index, i = 0; i < nsegs; i++) {
		if (i == nsegs - 1)
			flags |= EQOS_TDES3_LD;

		eqos_setup_txdesc(sc, cur, flags, segs[i].ds_addr,
		    segs[i].ds_len, m->m_pkthdr.len);
		flags &= ~EQOS_TDES3_FD;
		cur = TX_NEXT(cur);

		flags |= EQOS_TDES3_OWN;
	}

	/*
	 * Defer setting OWN bit on the first descriptor until all
	 * descriptors have been updated.  The hardware will not try to
	 * process any descriptors past the first one still owned by
	 * software (i.e., with the OWN bit clear).
	 */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx.desc_map,
	    DESC_OFF(index), offsetof(struct eqos_dma_desc, tdes3),
	    BUS_DMASYNC_PREWRITE);
	sc->sc_tx.desc_ring[index].tdes3 |= htole32(EQOS_TDES3_OWN);

	return nsegs;
}

static void
eqos_setup_rxdesc(struct eqos_softc *sc, int index, bus_addr_t paddr)
{

	sc->sc_rx.desc_ring[index].tdes0 = htole32((uint32_t)paddr);
	sc->sc_rx.desc_ring[index].tdes1 = htole32((uint32_t)(paddr >> 32));
	sc->sc_rx.desc_ring[index].tdes2 = htole32(0);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rx.desc_map,
	    DESC_OFF(index), offsetof(struct eqos_dma_desc, tdes3),
	    BUS_DMASYNC_PREWRITE);
	sc->sc_rx.desc_ring[index].tdes3 =
	    htole32(EQOS_TDES3_OWN | EQOS_TDES3_IOC | EQOS_TDES3_BUF1V);
}

static int
eqos_setup_rxbuf(struct eqos_softc *sc, int index, struct mbuf *m)
{
	int error;

	m_adj(m, ETHER_ALIGN);

	error = bus_dmamap_load_mbuf(sc->sc_dmat,
	    sc->sc_rx.buf_map[index].map, m, BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error != 0)
		return error;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_rx.buf_map[index].map,
	    0, sc->sc_rx.buf_map[index].map->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	sc->sc_rx.buf_map[index].mbuf = m;
	eqos_setup_rxdesc(sc, index,
	    sc->sc_rx.buf_map[index].map->dm_segs[0].ds_addr);

	return 0;
}

static struct mbuf *
eqos_alloc_mbufcl(struct eqos_softc *sc)
{
	struct mbuf *m;

	m = m_getcl(M_NOWAIT, MT_DATA, M_PKTHDR);
	if (m != NULL)
		m->m_pkthdr.len = m->m_len = m->m_ext.ext_size;

	return m;
}

static void
eqos_enable_intr(struct eqos_softc *sc)
{
	WR4(sc, GMAC_DMA_CHAN0_INTR_ENABLE,
	    GMAC_DMA_CHAN0_INTR_ENABLE_NIE |
	    GMAC_DMA_CHAN0_INTR_ENABLE_AIE |
	    GMAC_DMA_CHAN0_INTR_ENABLE_FBE |
	    GMAC_DMA_CHAN0_INTR_ENABLE_RIE |
	    GMAC_DMA_CHAN0_INTR_ENABLE_TIE);
}

static void
eqos_disable_intr(struct eqos_softc *sc)
{
	WR4(sc, GMAC_DMA_CHAN0_INTR_ENABLE, 0);
}

static void
eqos_tick(void *softc)
{
	struct eqos_softc *sc = softc;
	struct mii_data *mii = &sc->sc_mii;
#ifndef EQOS_MPSAFE
	int s = splnet();
#endif

	EQOS_LOCK(sc);
	mii_tick(mii);
	callout_schedule(&sc->sc_stat_ch, hz);
	EQOS_UNLOCK(sc);

#ifndef EQOS_MPSAFE
	splx(s);
#endif
}

static uint32_t
eqos_bitrev32(uint32_t x)
{
	x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
	x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
	x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
	x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));

	return (x >> 16) | (x << 16);
}

static void
eqos_setup_rxfilter(struct eqos_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	struct ifnet *ifp = &ec->ec_if;
	uint32_t pfil, crc, hashreg, hashbit, hash[2];
	struct ether_multi *enm;
	struct ether_multistep step;
	const uint8_t *eaddr;
	uint32_t val;

	EQOS_ASSERT_LOCKED(sc);

	pfil = RD4(sc, GMAC_MAC_PACKET_FILTER);
	pfil &= ~(GMAC_MAC_PACKET_FILTER_PR |
		  GMAC_MAC_PACKET_FILTER_PM |
		  GMAC_MAC_PACKET_FILTER_HMC |
		  GMAC_MAC_PACKET_FILTER_PCF_MASK);
	hash[0] = hash[1] = ~0U;

	if ((ifp->if_flags & IFF_PROMISC) != 0) {
		pfil |= GMAC_MAC_PACKET_FILTER_PR |
			GMAC_MAC_PACKET_FILTER_PCF_ALL;
	} else if ((ifp->if_flags & IFF_ALLMULTI) != 0) {
		pfil |= GMAC_MAC_PACKET_FILTER_PM;
	} else {
		hash[0] = hash[1] = 0;
		pfil |= GMAC_MAC_PACKET_FILTER_HMC;
		ETHER_LOCK(ec);
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);
			crc &= 0x7f;
			crc = eqos_bitrev32(~crc) >> 26;
			hashreg = (crc >> 5);
			hashbit = (crc & 0x1f);
			hash[hashreg] |= (1 << hashbit);
			ETHER_NEXT_MULTI(step, enm);
		}
		ETHER_UNLOCK(ec);
	}

	/* Write our unicast address */
	eaddr = CLLADDR(ifp->if_sadl);
	val = eaddr[4] | (eaddr[5] << 8);
	WR4(sc, GMAC_MAC_ADDRESS0_HIGH, val);
	val = eaddr[0] | (eaddr[1] << 8) | (eaddr[2] << 16) |
	    (eaddr[3] << 24);
	WR4(sc, GMAC_MAC_ADDRESS0_LOW, val);

	/* Multicast hash filters */
	WR4(sc, GMAC_MAC_HASH_TABLE_REG0, hash[1]);
	WR4(sc, GMAC_MAC_HASH_TABLE_REG1, hash[0]);

	/* Packet filter config */
	WR4(sc, GMAC_MAC_PACKET_FILTER, pfil);
}

static int
eqos_reset(struct eqos_softc *sc)
{
	uint32_t val;
	int retry;

	WR4(sc, GMAC_DMA_MODE, GMAC_DMA_MODE_SWR);
	for (retry = 2000; retry > 0; retry--) {
		delay(1000);
		val = RD4(sc, GMAC_DMA_MODE);
		if ((val & GMAC_DMA_MODE_SWR) == 0) {
			return 0;
		}
	}

	device_printf(sc->sc_dev, "reset timeout!\n");
	return ETIMEDOUT;
}

static void
eqos_init_rings(struct eqos_softc *sc, int qid)
{
	sc->sc_tx.queued = 0;

	WR4(sc, GMAC_DMA_CHAN0_TX_BASE_ADDR_HI,
	    (uint32_t)(sc->sc_tx.desc_ring_paddr >> 32));
	WR4(sc, GMAC_DMA_CHAN0_TX_BASE_ADDR,
	    (uint32_t)sc->sc_tx.desc_ring_paddr);
	WR4(sc, GMAC_DMA_CHAN0_TX_RING_LEN, TX_DESC_COUNT - 1);

	WR4(sc, GMAC_DMA_CHAN0_RX_BASE_ADDR_HI,
	    (uint32_t)(sc->sc_rx.desc_ring_paddr >> 32));
	WR4(sc, GMAC_DMA_CHAN0_RX_BASE_ADDR,
	    (uint32_t)sc->sc_rx.desc_ring_paddr);
	WR4(sc, GMAC_DMA_CHAN0_RX_RING_LEN, RX_DESC_COUNT - 1);
	WR4(sc, GMAC_DMA_CHAN0_RX_END_ADDR,
	    (uint32_t)sc->sc_rx.desc_ring_paddr +
	    DESC_OFF((sc->sc_rx.cur - 1) % RX_DESC_COUNT));
}

static int
eqos_init_locked(struct eqos_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct mii_data *mii = &sc->sc_mii;
	uint32_t val;

	EQOS_ASSERT_LOCKED(sc);
	EQOS_ASSERT_TXLOCKED(sc);

	if ((ifp->if_flags & IFF_RUNNING) != 0)
		return 0;

	/* Setup TX/RX rings */
	eqos_init_rings(sc, 0);

	/* Setup RX filter */
	eqos_setup_rxfilter(sc);

	WR4(sc, GMAC_MAC_1US_TIC_COUNTER, (sc->sc_csr_clock / 1000000) - 1);

	/* Enable transmit and receive DMA */
	val = RD4(sc, GMAC_DMA_CHAN0_CONTROL);
	val &= ~GMAC_DMA_CHAN0_CONTROL_DSL_MASK;
	val |= ((DESC_ALIGN - 16) / 8) << GMAC_DMA_CHAN0_CONTROL_DSL_SHIFT;
	val |= GMAC_DMA_CHAN0_CONTROL_PBLX8;
	WR4(sc, GMAC_DMA_CHAN0_CONTROL, val);
	val = RD4(sc, GMAC_DMA_CHAN0_TX_CONTROL);
	val |= GMAC_DMA_CHAN0_TX_CONTROL_OSP;
	val |= GMAC_DMA_CHAN0_TX_CONTROL_START;
	WR4(sc, GMAC_DMA_CHAN0_TX_CONTROL, val);
	val = RD4(sc, GMAC_DMA_CHAN0_RX_CONTROL);
	val &= ~GMAC_DMA_CHAN0_RX_CONTROL_RBSZ_MASK;
	val |= (MCLBYTES << GMAC_DMA_CHAN0_RX_CONTROL_RBSZ_SHIFT);
	val |= GMAC_DMA_CHAN0_RX_CONTROL_START;
	WR4(sc, GMAC_DMA_CHAN0_RX_CONTROL, val);

	/* Configure operation modes */
	WR4(sc, GMAC_MTL_TXQ0_OPERATION_MODE,
	    GMAC_MTL_TXQ0_OPERATION_MODE_TSF |
	    GMAC_MTL_TXQ0_OPERATION_MODE_TXQEN_EN);
	WR4(sc, GMAC_MTL_RXQ0_OPERATION_MODE,
	    GMAC_MTL_RXQ0_OPERATION_MODE_RSF |
	    GMAC_MTL_RXQ0_OPERATION_MODE_FEP |
	    GMAC_MTL_RXQ0_OPERATION_MODE_FUP);

	/* Enable flow control */
	val = RD4(sc, GMAC_MAC_Q0_TX_FLOW_CTRL);
	val |= 0xFFFFU << GMAC_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT;
	val |= GMAC_MAC_Q0_TX_FLOW_CTRL_TFE;
	WR4(sc, GMAC_MAC_Q0_TX_FLOW_CTRL, val);
	val = RD4(sc, GMAC_MAC_RX_FLOW_CTRL);
	val |= GMAC_MAC_RX_FLOW_CTRL_RFE;
	WR4(sc, GMAC_MAC_RX_FLOW_CTRL, val);

	/* Enable transmitter and receiver */
	val = RD4(sc, GMAC_MAC_CONFIGURATION);
	val |= GMAC_MAC_CONFIGURATION_BE;
	val |= GMAC_MAC_CONFIGURATION_JD;
	val |= GMAC_MAC_CONFIGURATION_JE;
	val |= GMAC_MAC_CONFIGURATION_DCRS;
	val |= GMAC_MAC_CONFIGURATION_TE;
	val |= GMAC_MAC_CONFIGURATION_RE;
	WR4(sc, GMAC_MAC_CONFIGURATION, val);

	/* Enable interrupts */
	eqos_enable_intr(sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	mii_mediachg(mii);
	callout_schedule(&sc->sc_stat_ch, hz);

	return 0;
}

static int
eqos_init(struct ifnet *ifp)
{
	struct eqos_softc *sc = ifp->if_softc;
	int error;

	EQOS_LOCK(sc);
	EQOS_TXLOCK(sc);
	error = eqos_init_locked(sc);
	EQOS_TXUNLOCK(sc);
	EQOS_UNLOCK(sc);

	return error;
}

static void
eqos_stop_locked(struct eqos_softc *sc, int disable)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint32_t val;
	int retry;

	EQOS_ASSERT_LOCKED(sc);

	callout_stop(&sc->sc_stat_ch);

	mii_down(&sc->sc_mii);

	/* Disable receiver */
	val = RD4(sc, GMAC_MAC_CONFIGURATION);
	val &= ~GMAC_MAC_CONFIGURATION_RE;
	WR4(sc, GMAC_MAC_CONFIGURATION, val);

	/* Stop receive DMA */
	val = RD4(sc, GMAC_DMA_CHAN0_RX_CONTROL);
	val &= ~GMAC_DMA_CHAN0_RX_CONTROL_START;
	WR4(sc, GMAC_DMA_CHAN0_RX_CONTROL, val);

	/* Stop transmit DMA */
	val = RD4(sc, GMAC_DMA_CHAN0_TX_CONTROL);
	val &= ~GMAC_DMA_CHAN0_TX_CONTROL_START;
	WR4(sc, GMAC_DMA_CHAN0_TX_CONTROL, val);

	if (disable) {
		/* Flush data in the TX FIFO */
		val = RD4(sc, GMAC_MTL_TXQ0_OPERATION_MODE);
		val |= GMAC_MTL_TXQ0_OPERATION_MODE_FTQ;
		WR4(sc, GMAC_MTL_TXQ0_OPERATION_MODE, val);
		/* Wait for flush to complete */
		for (retry = 10000; retry > 0; retry--) {
			val = RD4(sc, GMAC_MTL_TXQ0_OPERATION_MODE);
			if ((val & GMAC_MTL_TXQ0_OPERATION_MODE_FTQ) == 0) {
				break;
			}
			delay(1);
		}
		if (retry == 0) {
			device_printf(sc->sc_dev,
			    "timeout flushing TX queue\n");
		}
	}

	/* Disable transmitter */
	val = RD4(sc, GMAC_MAC_CONFIGURATION);
	val &= ~GMAC_MAC_CONFIGURATION_TE;
	WR4(sc, GMAC_MAC_CONFIGURATION, val);

	/* Disable interrupts */
	eqos_disable_intr(sc);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static void
eqos_stop(struct ifnet *ifp, int disable)
{
	struct eqos_softc * const sc = ifp->if_softc;

	EQOS_LOCK(sc);
	eqos_stop_locked(sc, disable);
	EQOS_UNLOCK(sc);
}

static void
eqos_rxintr(struct eqos_softc *sc, int qid)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int error, index, len, pkts = 0;
	struct mbuf *m, *m0;
	uint32_t tdes3;

	for (index = sc->sc_rx.cur; ; index = RX_NEXT(index)) {
		eqos_dma_sync(sc, sc->sc_rx.desc_map,
		    index, index + 1, RX_DESC_COUNT,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		tdes3 = le32toh(sc->sc_rx.desc_ring[index].tdes3);
		if ((tdes3 & EQOS_TDES3_OWN) != 0) {
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, sc->sc_rx.buf_map[index].map,
		    0, sc->sc_rx.buf_map[index].map->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat,
		    sc->sc_rx.buf_map[index].map);

		len = tdes3 & EQOS_TDES3_LENGTH_MASK;
		if (len != 0) {
			m = sc->sc_rx.buf_map[index].mbuf;
			m_set_rcvif(m, ifp);
			m->m_flags |= M_HASFCS;
			m->m_pkthdr.len = len;
			m->m_len = len;
			m->m_nextpkt = NULL;

			if_percpuq_enqueue(ifp->if_percpuq, m);
			++pkts;
		}

		if ((m0 = eqos_alloc_mbufcl(sc)) != NULL) {
			error = eqos_setup_rxbuf(sc, index, m0);
			if (error != 0) {
				/* XXX hole in RX ring */
			}
		} else {
			if_statinc(ifp, if_ierrors);
		}
		eqos_dma_sync(sc, sc->sc_rx.desc_map,
		    index, index + 1, RX_DESC_COUNT,
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		WR4(sc, GMAC_DMA_CHAN0_RX_END_ADDR,
		    (uint32_t)sc->sc_rx.desc_ring_paddr +
		    DESC_OFF(sc->sc_rx.cur));
	}

	sc->sc_rx.cur = index;

	if (pkts != 0) {
		rnd_add_uint32(&sc->sc_rndsource, pkts);
	}
}

static void
eqos_txintr(struct eqos_softc *sc, int qid)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct eqos_bufmap *bmap;
	struct eqos_dma_desc *desc;
	uint32_t tdes3;
	int i, pkts = 0;

	EQOS_ASSERT_LOCKED(sc);

	for (i = sc->sc_tx.next; sc->sc_tx.queued > 0; i = TX_NEXT(i)) {
		KASSERT(sc->sc_tx.queued > 0);
		KASSERT(sc->sc_tx.queued <= TX_DESC_COUNT);
		eqos_dma_sync(sc, sc->sc_tx.desc_map,
		    i, i + 1, TX_DESC_COUNT,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		desc = &sc->sc_tx.desc_ring[i];
		tdes3 = le32toh(desc->tdes3);
		if ((tdes3 & EQOS_TDES3_OWN) != 0) {
			break;
		}
		bmap = &sc->sc_tx.buf_map[i];
		if (bmap->mbuf != NULL) {
			bus_dmamap_sync(sc->sc_dmat, bmap->map,
			    0, bmap->map->dm_mapsize,
			    BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, bmap->map);
			m_freem(bmap->mbuf);
			bmap->mbuf = NULL;
			++pkts;
		}

		eqos_setup_txdesc(sc, i, 0, 0, 0, 0);
		eqos_dma_sync(sc, sc->sc_tx.desc_map,
		    i, i + 1, TX_DESC_COUNT,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		ifp->if_flags &= ~IFF_OACTIVE;

		/* Last descriptor in a packet contains DMA status */
		if ((tdes3 & EQOS_TDES3_LD) != 0) {
			if ((tdes3 & EQOS_TDES3_DE) != 0) {
				device_printf(sc->sc_dev,
				    "TX [%u] desc error: 0x%08x\n",
				    i, tdes3);
				if_statinc(ifp, if_oerrors);
			} else if ((tdes3 & EQOS_TDES3_ES) != 0) {
				device_printf(sc->sc_dev,
				    "TX [%u] tx error: 0x%08x\n",
				    i, tdes3);
				if_statinc(ifp, if_oerrors);
			} else {
				if_statinc(ifp, if_opackets);
			}
		}

	}

	sc->sc_tx.next = i;

	if (pkts != 0) {
		rnd_add_uint32(&sc->sc_rndsource, pkts);
	}
}

static void
eqos_start_locked(struct eqos_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct mbuf *m;
	int cnt, nsegs, start;

	EQOS_ASSERT_TXLOCKED(sc);

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (cnt = 0, start = sc->sc_tx.cur; ; cnt++) {
		if (sc->sc_tx.queued >= TX_DESC_COUNT - TX_MAX_SEGS) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL) {
			break;
		}

		nsegs = eqos_setup_txbuf(sc, sc->sc_tx.cur, m);
		if (nsegs <= 0) {
			if (nsegs == -1) {
				ifp->if_flags |= IFF_OACTIVE;
			} else if (nsegs == -2) {
				IFQ_DEQUEUE(&ifp->if_snd, m);
				m_freem(m);
			}
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);
		bpf_mtap(ifp, m, BPF_D_OUT);

		sc->sc_tx.cur = TX_SKIP(sc->sc_tx.cur, nsegs);
	}

	if (cnt != 0) {
		eqos_dma_sync(sc, sc->sc_tx.desc_map,
		    start, sc->sc_tx.cur, TX_DESC_COUNT,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* Start and run TX DMA */
		WR4(sc, GMAC_DMA_CHAN0_TX_END_ADDR,
		    (uint32_t)sc->sc_tx.desc_ring_paddr +
		    DESC_OFF(sc->sc_tx.cur));
	}
}

static void
eqos_start(struct ifnet *ifp)
{
	struct eqos_softc *sc = ifp->if_softc;

	EQOS_TXLOCK(sc);
	eqos_start_locked(sc);
	EQOS_TXUNLOCK(sc);
}

static void
eqos_intr_mtl(struct eqos_softc *sc, uint32_t mtl_status)
{
	uint32_t debug_data __unused = 0, ictrl = 0;

	if (mtl_status == 0)
		return;

	/* Drain the errors reported by MTL_INTERRUPT_STATUS */
	sc->sc_ev_mtl.ev_count++;

	if ((mtl_status & GMAC_MTL_INTERRUPT_STATUS_DBGIS) != 0) {
		debug_data = RD4(sc, GMAC_MTL_FIFO_DEBUG_DATA);
		sc->sc_ev_mtl_debugdata.ev_count++;
	}
	if ((mtl_status & GMAC_MTL_INTERRUPT_STATUS_Q0IS) != 0) {
		uint32_t new_status = 0;

		ictrl = RD4(sc, GMAC_MTL_Q0_INTERRUPT_CTRL_STATUS);
		if ((ictrl & GMAC_MTL_Q0_INTERRUPT_CTRL_STATUS_RXOVFIS) != 0) {
			new_status |= GMAC_MTL_Q0_INTERRUPT_CTRL_STATUS_RXOVFIS;
			sc->sc_ev_mtl_rxovfis.ev_count++;
		}
		if ((ictrl & GMAC_MTL_Q0_INTERRUPT_CTRL_STATUS_TXUNFIS) != 0) {
			new_status |= GMAC_MTL_Q0_INTERRUPT_CTRL_STATUS_TXUNFIS;
			sc->sc_ev_mtl_txovfis.ev_count++;
		}
		if (new_status) {
			new_status |= (ictrl &
			    (GMAC_MTL_Q0_INTERRUPT_CTRL_STATUS_RXOIE|
			     GMAC_MTL_Q0_INTERRUPT_CTRL_STATUS_TXUIE));
			WR4(sc, GMAC_MTL_Q0_INTERRUPT_CTRL_STATUS, new_status);
		}
	}
#ifdef DEBUG_LOUD
	device_printf(sc->sc_dev,
	    "GMAC_MTL_INTERRUPT_STATUS = 0x%08X, "
	    "GMAC_MTL_FIFO_DEBUG_DATA = 0x%08X, "
	    "GMAC_MTL_INTERRUPT_STATUS_Q0IS = 0x%08X\n",
	    mtl_status, debug_data, ictrl);
#endif
}

int
eqos_intr(void *arg)
{
	struct eqos_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint32_t mac_status, mtl_status, dma_status, rx_tx_status;

	sc->sc_ev_intr.ev_count++;

	mac_status = RD4(sc, GMAC_MAC_INTERRUPT_STATUS);
	mac_status &= RD4(sc, GMAC_MAC_INTERRUPT_ENABLE);

	if (mac_status) {
		sc->sc_ev_mac.ev_count++;
#ifdef DEBUG_LOUD
		device_printf(sc->sc_dev,
		    "GMAC_MAC_INTERRUPT_STATUS = 0x%08X\n", mac_status);
#endif
	}

	mtl_status = RD4(sc, GMAC_MTL_INTERRUPT_STATUS);
	eqos_intr_mtl(sc, mtl_status);

	dma_status = RD4(sc, GMAC_DMA_CHAN0_STATUS);
	dma_status &= RD4(sc, GMAC_DMA_CHAN0_INTR_ENABLE);
	if (dma_status) {
		WR4(sc, GMAC_DMA_CHAN0_STATUS, dma_status);
	}

	EQOS_LOCK(sc);
	if ((dma_status & GMAC_DMA_CHAN0_STATUS_RI) != 0) {
		eqos_rxintr(sc, 0);
		sc->sc_ev_rxintr.ev_count++;
	}

	if ((dma_status & GMAC_DMA_CHAN0_STATUS_TI) != 0) {
		eqos_txintr(sc, 0);
		if_schedule_deferred_start(ifp);
		sc->sc_ev_txintr.ev_count++;
	}
	EQOS_UNLOCK(sc);

#ifdef DEBUG_LOUD
	if ((mac_status | mtl_status | dma_status) == 0) {
		device_printf(sc->sc_dev, "spurious interrupt?!\n");
	}
#endif

	rx_tx_status = RD4(sc, GMAC_MAC_RX_TX_STATUS);
	if (rx_tx_status) {
		sc->sc_ev_status.ev_count++;
		if ((rx_tx_status & GMAC_MAC_RX_TX_STATUS_RWT) != 0)
			sc->sc_ev_rwt.ev_count++;
		if ((rx_tx_status & GMAC_MAC_RX_TX_STATUS_EXCOL) != 0)
			sc->sc_ev_excol.ev_count++;
		if ((rx_tx_status & GMAC_MAC_RX_TX_STATUS_LCOL) != 0)
			sc->sc_ev_lcol.ev_count++;
		if ((rx_tx_status & GMAC_MAC_RX_TX_STATUS_EXDEF) != 0)
			sc->sc_ev_exdef.ev_count++;
		if ((rx_tx_status & GMAC_MAC_RX_TX_STATUS_LCARR) != 0)
			sc->sc_ev_lcarr.ev_count++;
		if ((rx_tx_status & GMAC_MAC_RX_TX_STATUS_NCARR) != 0)
			sc->sc_ev_ncarr.ev_count++;
		if ((rx_tx_status & GMAC_MAC_RX_TX_STATUS_TJT) != 0)
			sc->sc_ev_tjt.ev_count++;
#ifdef DEBUG_LOUD
		device_printf(sc->sc_dev, "GMAC_MAC_RX_TX_STATUS = 0x%08x\n",
		    rx_tx_status);
#endif
	}

	return 1;
}

static int
eqos_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct eqos_softc *sc = ifp->if_softc;
	int error, s;

#ifndef EQOS_MPSAFE
	s = splnet();
#endif

	switch (cmd) {
	default:
#ifdef EQOS_MPSAFE
		s = splnet();
#endif
		error = ether_ioctl(ifp, cmd, data);
#ifdef EQOS_MPSAFE
		splx(s);
#endif
		if (error != ENETRESET)
			break;

		error = 0;

		if (cmd == SIOCSIFCAP)
			error = (*ifp->if_init)(ifp);
		else if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			;
		else if ((ifp->if_flags & IFF_RUNNING) != 0) {
			EQOS_LOCK(sc);
			eqos_setup_rxfilter(sc);
			EQOS_UNLOCK(sc);
		}
		break;
	}

#ifndef EQOS_MPSAFE
	splx(s);
#endif

	return error;
}

static void
eqos_get_eaddr(struct eqos_softc *sc, uint8_t *eaddr)
{
	prop_dictionary_t prop = device_properties(sc->sc_dev);
	uint32_t maclo, machi;
	prop_data_t eaprop;

	eaprop = prop_dictionary_get(prop, "mac-address");
	if (eaprop != NULL) {
		KASSERT(prop_object_type(eaprop) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(eaprop) == ETHER_ADDR_LEN);
		memcpy(eaddr, prop_data_value(eaprop),
		    ETHER_ADDR_LEN);
		return;
	}

	maclo = htobe32(RD4(sc, GMAC_MAC_ADDRESS0_LOW));
	machi = htobe16(RD4(sc, GMAC_MAC_ADDRESS0_HIGH) & 0xFFFF);

	if (maclo == 0xFFFFFFFF && machi == 0xFFFF) {
		/* Create one */
		maclo = 0x00f2 | (cprng_strong32() & 0xffff0000);
		machi = cprng_strong32() & 0xffff;
	}

	eaddr[0] = maclo & 0xff;
	eaddr[1] = (maclo >> 8) & 0xff;
	eaddr[2] = (maclo >> 16) & 0xff;
	eaddr[3] = (maclo >> 24) & 0xff;
	eaddr[4] = machi & 0xff;
	eaddr[5] = (machi >> 8) & 0xff;
}

static void
eqos_axi_configure(struct eqos_softc *sc)
{
	prop_dictionary_t prop = device_properties(sc->sc_dev);
	uint32_t val;
	u_int uival;
	bool bval;

	val = RD4(sc, GMAC_DMA_SYSBUS_MODE);
	if (prop_dictionary_get_bool(prop, "snps,mixed-burst", &bval) && bval) {
		val |= GMAC_DMA_SYSBUS_MODE_MB;
	}
	if (prop_dictionary_get_bool(prop, "snps,fixed-burst", &bval) && bval) {
		val |= GMAC_DMA_SYSBUS_MODE_FB;
	}
	if (prop_dictionary_get_uint(prop, "snps,wr_osr_lmt", &uival)) {
		val &= ~GMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_MASK;
		val |= uival << GMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_SHIFT;
	}
	if (prop_dictionary_get_uint(prop, "snps,rd_osr_lmt", &uival)) {
		val &= ~GMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK;
		val |= uival << GMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT;
	}

	if (!EQOS_HW_FEATURE_ADDR64_32BIT(sc)) {
		val |= GMAC_DMA_SYSBUS_MODE_EAME;
	}

	/* XXX */
	val |= GMAC_DMA_SYSBUS_MODE_BLEN16;
	val |= GMAC_DMA_SYSBUS_MODE_BLEN8;
	val |= GMAC_DMA_SYSBUS_MODE_BLEN4;

	WR4(sc, GMAC_DMA_SYSBUS_MODE, val);
}

static int
eqos_setup_dma(struct eqos_softc *sc, int qid)
{
	struct mbuf *m;
	int error, nsegs, i;

	/* Setup TX ring */
	error = bus_dmamap_create(sc->sc_dmat, TX_DESC_SIZE, 1, TX_DESC_SIZE,
	    DESC_BOUNDARY, BUS_DMA_WAITOK, &sc->sc_tx.desc_map);
	if (error) {
		return error;
	}
	error = bus_dmamem_alloc(sc->sc_dmat, TX_DESC_SIZE, DESC_ALIGN,
	    DESC_BOUNDARY, &sc->sc_tx.desc_dmaseg, 1, &nsegs, BUS_DMA_WAITOK);
	if (error) {
		return error;
	}
	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_tx.desc_dmaseg, nsegs,
	    TX_DESC_SIZE, (void *)&sc->sc_tx.desc_ring, BUS_DMA_WAITOK);
	if (error) {
		return error;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_tx.desc_map,
	    sc->sc_tx.desc_ring, TX_DESC_SIZE, NULL, BUS_DMA_WAITOK);
	if (error) {
		return error;
	}
	sc->sc_tx.desc_ring_paddr = sc->sc_tx.desc_map->dm_segs[0].ds_addr;

	memset(sc->sc_tx.desc_ring, 0, TX_DESC_SIZE);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx.desc_map, 0, TX_DESC_SIZE,
	    BUS_DMASYNC_PREWRITE);

	sc->sc_tx.queued = TX_DESC_COUNT;
	for (i = 0; i < TX_DESC_COUNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    TX_MAX_SEGS, MCLBYTES, 0, BUS_DMA_WAITOK,
		    &sc->sc_tx.buf_map[i].map);
		if (error != 0) {
			device_printf(sc->sc_dev,
			    "cannot create TX buffer map\n");
			return error;
		}
		eqos_setup_txdesc(sc, i, 0, 0, 0, 0);
	}

	/* Setup RX ring */
	error = bus_dmamap_create(sc->sc_dmat, RX_DESC_SIZE, 1, RX_DESC_SIZE,
	    DESC_BOUNDARY, BUS_DMA_WAITOK, &sc->sc_rx.desc_map);
	if (error) {
		return error;
	}
	error = bus_dmamem_alloc(sc->sc_dmat, RX_DESC_SIZE, DESC_ALIGN,
	    DESC_BOUNDARY, &sc->sc_rx.desc_dmaseg, 1, &nsegs, BUS_DMA_WAITOK);
	if (error) {
		return error;
	}
	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_rx.desc_dmaseg, nsegs,
	    RX_DESC_SIZE, (void *)&sc->sc_rx.desc_ring, BUS_DMA_WAITOK);
	if (error) {
		return error;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_rx.desc_map,
	    sc->sc_rx.desc_ring, RX_DESC_SIZE, NULL, BUS_DMA_WAITOK);
	if (error) {
		return error;
	}
	sc->sc_rx.desc_ring_paddr = sc->sc_rx.desc_map->dm_segs[0].ds_addr;

	memset(sc->sc_rx.desc_ring, 0, RX_DESC_SIZE);

	for (i = 0; i < RX_DESC_COUNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    RX_DESC_COUNT, MCLBYTES, 0, BUS_DMA_WAITOK,
		    &sc->sc_rx.buf_map[i].map);
		if (error != 0) {
			device_printf(sc->sc_dev,
			    "cannot create RX buffer map\n");
			return error;
		}
		if ((m = eqos_alloc_mbufcl(sc)) == NULL) {
			device_printf(sc->sc_dev, "cannot allocate RX mbuf\n");
			return ENOMEM;
		}
		error = eqos_setup_rxbuf(sc, i, m);
		if (error != 0) {
			device_printf(sc->sc_dev, "cannot create RX buffer\n");
			return error;
		}
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rx.desc_map,
	    0, sc->sc_rx.desc_map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	aprint_debug_dev(sc->sc_dev, "TX ring @ 0x%lX, RX ring @ 0x%lX\n",
	    sc->sc_tx.desc_ring_paddr, sc->sc_rx.desc_ring_paddr);

	return 0;
}

int
eqos_attach(struct eqos_softc *sc)
{
	struct mii_data *mii = &sc->sc_mii;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint8_t eaddr[ETHER_ADDR_LEN];
	u_int userver, snpsver;
	int mii_flags = 0;
	int error;
	int n;

	const uint32_t ver = RD4(sc, GMAC_MAC_VERSION);
	userver = (ver & GMAC_MAC_VERSION_USERVER_MASK) >>
	    GMAC_MAC_VERSION_USERVER_SHIFT;
	snpsver = ver & GMAC_MAC_VERSION_SNPSVER_MASK;

	if (snpsver != 0x51) {
		aprint_error(": EQOS version 0x%02xx not supported\n",
		    snpsver);
		return ENXIO;
	}

	if (sc->sc_csr_clock < 20000000) {
		aprint_error(": CSR clock too low\n");
		return EINVAL;
	} else if (sc->sc_csr_clock < 35000000) {
		sc->sc_clock_range = GMAC_MAC_MDIO_ADDRESS_CR_20_35;
	} else if (sc->sc_csr_clock < 60000000) {
		sc->sc_clock_range = GMAC_MAC_MDIO_ADDRESS_CR_35_60;
	} else if (sc->sc_csr_clock < 100000000) {
		sc->sc_clock_range = GMAC_MAC_MDIO_ADDRESS_CR_60_100;
	} else if (sc->sc_csr_clock < 150000000) {
		sc->sc_clock_range = GMAC_MAC_MDIO_ADDRESS_CR_100_150;
	} else if (sc->sc_csr_clock < 250000000) {
		sc->sc_clock_range = GMAC_MAC_MDIO_ADDRESS_CR_150_250;
	} else if (sc->sc_csr_clock < 300000000) {
		sc->sc_clock_range = GMAC_MAC_MDIO_ADDRESS_CR_300_500;
	} else if (sc->sc_csr_clock < 800000000) {
		sc->sc_clock_range = GMAC_MAC_MDIO_ADDRESS_CR_500_800;
	} else {
		aprint_error(": CSR clock too high\n");
		return EINVAL;
	}

	for (n = 0; n < 4; n++) {
		sc->sc_hw_feature[n] = RD4(sc, GMAC_MAC_HW_FEATURE(n));
	}

	aprint_naive("\n");
	aprint_normal(": DesignWare EQOS ver 0x%02x (0x%02x)\n",
	    snpsver, userver);
	aprint_verbose_dev(sc->sc_dev, "hw features %08x %08x %08x %08x\n",
	    sc->sc_hw_feature[0], sc->sc_hw_feature[1],
	    sc->sc_hw_feature[2], sc->sc_hw_feature[3]);

	if (EQOS_HW_FEATURE_ADDR64_32BIT(sc)) {
		bus_dma_tag_t ntag;

		error = bus_dmatag_subregion(sc->sc_dmat, 0, UINT32_MAX,
		    &ntag, 0);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "failed to restrict DMA: %d\n", error);
			return error;
		}
		aprint_verbose_dev(sc->sc_dev, "using 32-bit DMA\n");
		sc->sc_dmat = ntag;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_txlock, MUTEX_DEFAULT, IPL_NET);
	callout_init(&sc->sc_stat_ch, CALLOUT_FLAGS);
	callout_setfunc(&sc->sc_stat_ch, eqos_tick, sc);

	eqos_get_eaddr(sc, eaddr);
	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n", ether_sprintf(eaddr));

	/* Soft reset EMAC core */
	error = eqos_reset(sc);
	if (error != 0) {
		return error;
	}

	/* Configure AXI Bus mode parameters */
	eqos_axi_configure(sc);

	/* Setup DMA descriptors */
	if (eqos_setup_dma(sc, 0) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to setup DMA descriptors\n");
		return EINVAL;
	}

	/* Setup ethernet interface */
	ifp->if_softc = sc;
	snprintf(ifp->if_xname, IFNAMSIZ, "%s", device_xname(sc->sc_dev));
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef EQOS_MPSAFE
	ifp->if_extflags = IFEF_MPSAFE;
#endif
	ifp->if_start = eqos_start;
	ifp->if_ioctl = eqos_ioctl;
	ifp->if_init = eqos_init;
	ifp->if_stop = eqos_stop;
	ifp->if_capabilities = 0;
	ifp->if_capenable = ifp->if_capabilities;
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifp->if_snd);

	/* 802.1Q VLAN-sized frames are supported */
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

	/* Attach MII driver */
	sc->sc_ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
	mii->mii_ifp = ifp;
	mii->mii_readreg = eqos_mii_readreg;
	mii->mii_writereg = eqos_mii_writereg;
	mii->mii_statchg = eqos_mii_statchg;
	mii_attach(sc->sc_dev, mii, 0xffffffff, sc->sc_phy_id, MII_OFFSET_ANY,
	    mii_flags);

	if (LIST_EMPTY(&mii->mii_phys)) {
		aprint_error_dev(sc->sc_dev, "no PHY found!\n");
		return ENOENT;
	}
	ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);

	/* Master interrupt evcnt */
	evcnt_attach_dynamic(&sc->sc_ev_intr, EVCNT_TYPE_INTR,
	    NULL, device_xname(sc->sc_dev), "interrupts");

	/* Per-interrupt type, using main interrupt */
	evcnt_attach_dynamic(&sc->sc_ev_rxintr, EVCNT_TYPE_INTR,
	    &sc->sc_ev_intr, device_xname(sc->sc_dev), "rxintr");
	evcnt_attach_dynamic(&sc->sc_ev_txintr, EVCNT_TYPE_INTR,
	    &sc->sc_ev_intr, device_xname(sc->sc_dev), "txintr");
	evcnt_attach_dynamic(&sc->sc_ev_mac, EVCNT_TYPE_INTR,
	    &sc->sc_ev_intr, device_xname(sc->sc_dev), "macstatus");
	evcnt_attach_dynamic(&sc->sc_ev_mtl, EVCNT_TYPE_INTR,
	    &sc->sc_ev_intr, device_xname(sc->sc_dev), "intrstatus");
	evcnt_attach_dynamic(&sc->sc_ev_status, EVCNT_TYPE_INTR,
	    &sc->sc_ev_intr, device_xname(sc->sc_dev), "rxtxstatus");

	/* MAC Status specific type, using macstatus interrupt */
	evcnt_attach_dynamic(&sc->sc_ev_mtl_debugdata, EVCNT_TYPE_INTR,
	    &sc->sc_ev_mtl, device_xname(sc->sc_dev), "debugdata");
	evcnt_attach_dynamic(&sc->sc_ev_mtl_rxovfis, EVCNT_TYPE_INTR,
	    &sc->sc_ev_mtl, device_xname(sc->sc_dev), "rxovfis");
	evcnt_attach_dynamic(&sc->sc_ev_mtl_txovfis, EVCNT_TYPE_INTR,
	    &sc->sc_ev_mtl, device_xname(sc->sc_dev), "txovfis");

	/* RX/TX Status specific type, using rxtxstatus interrupt */
	evcnt_attach_dynamic(&sc->sc_ev_rwt, EVCNT_TYPE_INTR,
	    &sc->sc_ev_status, device_xname(sc->sc_dev), "rwt");
	evcnt_attach_dynamic(&sc->sc_ev_excol, EVCNT_TYPE_INTR,
	    &sc->sc_ev_status, device_xname(sc->sc_dev), "excol");
	evcnt_attach_dynamic(&sc->sc_ev_lcol, EVCNT_TYPE_INTR,
	    &sc->sc_ev_status, device_xname(sc->sc_dev), "lcol");
	evcnt_attach_dynamic(&sc->sc_ev_exdef, EVCNT_TYPE_INTR,
	    &sc->sc_ev_status, device_xname(sc->sc_dev), "exdef");
	evcnt_attach_dynamic(&sc->sc_ev_lcarr, EVCNT_TYPE_INTR,
	    &sc->sc_ev_status, device_xname(sc->sc_dev), "lcarr");
	evcnt_attach_dynamic(&sc->sc_ev_ncarr, EVCNT_TYPE_INTR,
	    &sc->sc_ev_status, device_xname(sc->sc_dev), "ncarr");
	evcnt_attach_dynamic(&sc->sc_ev_tjt, EVCNT_TYPE_INTR,
	    &sc->sc_ev_status, device_xname(sc->sc_dev), "tjt");

	/* Attach interface */
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);

	/* Attach ethernet interface */
	ether_ifattach(ifp, eaddr);

	rnd_attach_source(&sc->sc_rndsource, ifp->if_xname, RND_TYPE_NET,
	    RND_FLAG_DEFAULT);

	return 0;
}
