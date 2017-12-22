/* $NetBSD: sunxi_emac.c,v 1.12 2017/12/22 13:39:57 jmcneill Exp $ */

/*-
 * Copyright (c) 2016-2017 Jared McNeill <jmcneill@invisible.ca>
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
 * Allwinner Gigabit Ethernet MAC (EMAC) controller
 */

#include "opt_net_mpsafe.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_emac.c,v 1.12 2017/12/22 13:39:57 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/callout.h>
#include <sys/gpio.h>
#include <sys/cprng.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#include <dev/mii/miivar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_emac.h>

#ifdef NET_MPSAFE
#define	EMAC_MPSAFE		1
#define	CALLOUT_FLAGS		CALLOUT_MPSAFE
#define	FDT_INTR_FLAGS		FDT_INTR_MPSAFE
#else
#define	CALLOUT_FLAGS		0
#define	FDT_INTR_FLAGS		0
#endif

#define	EMAC_IFNAME		"emac%d"

#define	ETHER_ALIGN		2

#define	EMAC_LOCK(sc)		mutex_enter(&(sc)->mtx)
#define	EMAC_UNLOCK(sc)		mutex_exit(&(sc)->mtx)
#define	EMAC_ASSERT_LOCKED(sc)	KASSERT(mutex_owned(&(sc)->mtx))

#define	DESC_ALIGN		sizeof(struct sunxi_emac_desc)
#define	TX_DESC_COUNT		1024
#define	TX_DESC_SIZE		(sizeof(struct sunxi_emac_desc) * TX_DESC_COUNT)
#define	RX_DESC_COUNT		256
#define	RX_DESC_SIZE		(sizeof(struct sunxi_emac_desc) * RX_DESC_COUNT)

#define	DESC_OFF(n)		((n) * sizeof(struct sunxi_emac_desc))
#define	TX_NEXT(n)		(((n) + 1) & (TX_DESC_COUNT - 1))
#define	TX_SKIP(n, o)		(((n) + (o)) & (TX_DESC_COUNT - 1))
#define	RX_NEXT(n)		(((n) + 1) & (RX_DESC_COUNT - 1))

#define	TX_MAX_SEGS		128

#define	SOFT_RST_RETRY		1000
#define	MII_BUSY_RETRY		1000
#define	MDIO_FREQ		2500000

#define	BURST_LEN_DEFAULT	8
#define	RX_TX_PRI_DEFAULT	0
#define	PAUSE_TIME_DEFAULT	0x400

/* syscon EMAC clock register */
#define	EMAC_CLK_REG		0x30
#define	 EMAC_CLK_EPHY_ADDR		(0x1f << 20)	/* H3 */
#define	 EMAC_CLK_EPHY_ADDR_SHIFT	20
#define	 EMAC_CLK_EPHY_LED_POL		(1 << 17)	/* H3 */
#define	 EMAC_CLK_EPHY_SHUTDOWN		(1 << 16)	/* H3 */
#define	 EMAC_CLK_EPHY_SELECT		(1 << 15)	/* H3 */
#define	 EMAC_CLK_RMII_EN		(1 << 13)
#define	 EMAC_CLK_ETXDC			(0x7 << 10)
#define	 EMAC_CLK_ETXDC_SHIFT		10
#define	 EMAC_CLK_ERXDC			(0x1f << 5)
#define	 EMAC_CLK_ERXDC_SHIFT		5
#define	 EMAC_CLK_PIT			(0x1 << 2)
#define	  EMAC_CLK_PIT_MII		(0 << 2)
#define	  EMAC_CLK_PIT_RGMII		(1 << 2)
#define	 EMAC_CLK_SRC			(0x3 << 0)
#define	  EMAC_CLK_SRC_MII		(0 << 0)
#define	  EMAC_CLK_SRC_EXT_RGMII	(1 << 0)
#define	  EMAC_CLK_SRC_RGMII		(2 << 0)

/* Burst length of RX and TX DMA transfers */
static int sunxi_emac_burst_len = BURST_LEN_DEFAULT;

/* RX / TX DMA priority. If 1, RX DMA has priority over TX DMA. */
static int sunxi_emac_rx_tx_pri = RX_TX_PRI_DEFAULT;

/* Pause time field in the transmitted control frame */
static int sunxi_emac_pause_time = PAUSE_TIME_DEFAULT;

enum sunxi_emac_type {
	EMAC_A83T = 1,
	EMAC_H3,
	EMAC_A64,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun8i-a83t-emac",	EMAC_A83T },
	{ "allwinner,sun8i-h3-emac",	EMAC_H3 },
	{ "allwinner,sun50i-a64-emac",	EMAC_A64 },
	{ NULL }
};

struct sunxi_emac_bufmap {
	bus_dmamap_t		map;
	struct mbuf		*mbuf;
};

struct sunxi_emac_txring {
	bus_dma_tag_t		desc_tag;
	bus_dmamap_t		desc_map;
	bus_dma_segment_t	desc_dmaseg;
	struct sunxi_emac_desc	*desc_ring;
	bus_addr_t		desc_ring_paddr;
	bus_dma_tag_t		buf_tag;
	struct sunxi_emac_bufmap buf_map[TX_DESC_COUNT];
	u_int			cur, next, queued;
};

struct sunxi_emac_rxring {
	bus_dma_tag_t		desc_tag;
	bus_dmamap_t		desc_map;
	bus_dma_segment_t	desc_dmaseg;
	struct sunxi_emac_desc	*desc_ring;
	bus_addr_t		desc_ring_paddr;
	bus_dma_tag_t		buf_tag;
	struct sunxi_emac_bufmap buf_map[RX_DESC_COUNT];
	u_int			cur;
};

enum {
	_RES_EMAC,
	_RES_SYSCON,
	_RES_NITEMS
};

struct sunxi_emac_softc {
	device_t		dev;
	int			phandle;
	enum sunxi_emac_type	type;
	bus_space_tag_t		bst;
	bus_dma_tag_t		dmat;

	bus_space_handle_t	bsh[_RES_NITEMS];
	struct clk		*clk_ahb;
	struct clk		*clk_ephy;
	struct fdtbus_reset	*rst_ahb;
	struct fdtbus_reset	*rst_ephy;
	struct fdtbus_regulator	*reg_phy;
	struct fdtbus_gpio_pin	*pin_reset;

	int			phy_id;

	kmutex_t		mtx;
	struct ethercom		ec;
	struct mii_data		mii;
	callout_t		stat_ch;
	void			*ih;
	u_int			mdc_div_ratio_m;

	struct sunxi_emac_txring	tx;
	struct sunxi_emac_rxring	rx;
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->bst, (sc)->bsh[_RES_EMAC], (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->bst, (sc)->bsh[_RES_EMAC], (reg), (val))

#define	SYSCONRD4(sc, reg)		\
	bus_space_read_4((sc)->bst, (sc)->bsh[_RES_SYSCON], (reg))
#define	SYSCONWR4(sc, reg, val)		\
	bus_space_write_4((sc)->bst, (sc)->bsh[_RES_SYSCON], (reg), (val))

static int
sunxi_emac_mii_readreg(device_t dev, int phy, int reg)
{
	struct sunxi_emac_softc *sc = device_private(dev);
	int retry, val;

	val = 0;

	WR4(sc, EMAC_MII_CMD,
	    (sc->mdc_div_ratio_m << MDC_DIV_RATIO_M_SHIFT) |
	    (phy << PHY_ADDR_SHIFT) |
	    (reg << PHY_REG_ADDR_SHIFT) |
	    MII_BUSY);
	for (retry = MII_BUSY_RETRY; retry > 0; retry--) {
		if ((RD4(sc, EMAC_MII_CMD) & MII_BUSY) == 0) {
			val = RD4(sc, EMAC_MII_DATA);
			break;
		}
		delay(10);
	}

	if (retry == 0)
		device_printf(dev, "phy read timeout, phy=%d reg=%d\n",
		    phy, reg);

	return val;
}

static void
sunxi_emac_mii_writereg(device_t dev, int phy, int reg, int val)
{
	struct sunxi_emac_softc *sc = device_private(dev);
	int retry;

	WR4(sc, EMAC_MII_DATA, val);
	WR4(sc, EMAC_MII_CMD,
	    (sc->mdc_div_ratio_m << MDC_DIV_RATIO_M_SHIFT) |
	    (phy << PHY_ADDR_SHIFT) |
	    (reg << PHY_REG_ADDR_SHIFT) |
	    MII_WR | MII_BUSY);
	for (retry = MII_BUSY_RETRY; retry > 0; retry--) {
		if ((RD4(sc, EMAC_MII_CMD) & MII_BUSY) == 0)
			break;
		delay(10);
	}

	if (retry == 0)
		device_printf(dev, "phy write timeout, phy=%d reg=%d\n",
		    phy, reg);
}

static void
sunxi_emac_update_link(struct sunxi_emac_softc *sc)
{
	struct mii_data *mii = &sc->mii;
	uint32_t val;

	val = RD4(sc, EMAC_BASIC_CTL_0);
	val &= ~(BASIC_CTL_SPEED | BASIC_CTL_DUPLEX);

	if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T ||
	    IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_SX)
		val |= BASIC_CTL_SPEED_1000 << BASIC_CTL_SPEED_SHIFT;
	else if (IFM_SUBTYPE(mii->mii_media_active) == IFM_100_TX)
		val |= BASIC_CTL_SPEED_100 << BASIC_CTL_SPEED_SHIFT;
	else
		val |= BASIC_CTL_SPEED_10 << BASIC_CTL_SPEED_SHIFT;

	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_FDX) != 0)
		val |= BASIC_CTL_DUPLEX;

	WR4(sc, EMAC_BASIC_CTL_0, val);

	val = RD4(sc, EMAC_RX_CTL_0);
	val &= ~RX_FLOW_CTL_EN;
	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_ETH_RXPAUSE) != 0)
		val |= RX_FLOW_CTL_EN;
	WR4(sc, EMAC_RX_CTL_0, val);

	val = RD4(sc, EMAC_TX_FLOW_CTL);
	val &= ~(PAUSE_TIME|TX_FLOW_CTL_EN);
	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_ETH_TXPAUSE) != 0)
		val |= TX_FLOW_CTL_EN;
	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_FDX) != 0)
		val |= sunxi_emac_pause_time << PAUSE_TIME_SHIFT;
	WR4(sc, EMAC_TX_FLOW_CTL, val);
}

static void
sunxi_emac_mii_statchg(struct ifnet *ifp)
{
	struct sunxi_emac_softc * const sc = ifp->if_softc;

	sunxi_emac_update_link(sc);
}

static void
sunxi_emac_dma_sync(struct sunxi_emac_softc *sc, bus_dma_tag_t dmat,
    bus_dmamap_t map, int start, int end, int total, int flags)
{
	if (end > start) {
		bus_dmamap_sync(dmat, map, DESC_OFF(start),
		    DESC_OFF(end) - DESC_OFF(start), flags);
	} else {
		bus_dmamap_sync(dmat, map, DESC_OFF(start),
		    DESC_OFF(total) - DESC_OFF(start), flags);
		if (DESC_OFF(end) - DESC_OFF(0) > 0)
			bus_dmamap_sync(dmat, map, DESC_OFF(0),
			    DESC_OFF(end) - DESC_OFF(0), flags);
	}
}

static void
sunxi_emac_setup_txdesc(struct sunxi_emac_softc *sc, int index, int flags,
    bus_addr_t paddr, u_int len)
{
	uint32_t status, size;

	if (paddr == 0 || len == 0) {
		status = 0;
		size = 0;
		--sc->tx.queued;
	} else {
		status = TX_DESC_CTL;
		size = flags | len;
		++sc->tx.queued;
	}

	sc->tx.desc_ring[index].addr = htole32((uint32_t)paddr);
	sc->tx.desc_ring[index].size = htole32(size);
	sc->tx.desc_ring[index].status = htole32(status);
}

static int
sunxi_emac_setup_txbuf(struct sunxi_emac_softc *sc, int index, struct mbuf *m)
{
	bus_dma_segment_t *segs;
	int error, nsegs, cur, i, flags;
	u_int csum_flags;

	error = bus_dmamap_load_mbuf(sc->tx.buf_tag,
	    sc->tx.buf_map[index].map, m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (error == EFBIG) {
		device_printf(sc->dev,
		    "TX packet needs too many DMA segments, dropping...\n");
		m_freem(m);
		return 0;
	}
	if (error != 0)
		return 0;

	segs = sc->tx.buf_map[index].map->dm_segs;
	nsegs = sc->tx.buf_map[index].map->dm_nsegs;

	flags = TX_FIR_DESC;
	if ((m->m_pkthdr.csum_flags & M_CSUM_IPv4) != 0) {
		if ((m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) != 0)
			csum_flags = TX_CHECKSUM_CTL_FULL;
		else
			csum_flags = TX_CHECKSUM_CTL_IP;
		flags |= (csum_flags << TX_CHECKSUM_CTL_SHIFT);
	}

	for (cur = index, i = 0; i < nsegs; i++) {
		sc->tx.buf_map[cur].mbuf = (i == 0 ? m : NULL);
		if (i == nsegs - 1)
			flags |= TX_LAST_DESC | TX_INT_CTL;

		sunxi_emac_setup_txdesc(sc, cur, flags, segs[i].ds_addr,
		    segs[i].ds_len);
		flags &= ~TX_FIR_DESC;
		cur = TX_NEXT(cur);
	}

	bus_dmamap_sync(sc->tx.buf_tag, sc->tx.buf_map[index].map,
	    0, sc->tx.buf_map[index].map->dm_mapsize, BUS_DMASYNC_PREWRITE);

	return nsegs;
}

static void
sunxi_emac_setup_rxdesc(struct sunxi_emac_softc *sc, int index,
    bus_addr_t paddr)
{
	uint32_t status, size;

	status = RX_DESC_CTL;
	size = MCLBYTES - 1;

	sc->rx.desc_ring[index].addr = htole32((uint32_t)paddr);
	sc->rx.desc_ring[index].size = htole32(size);
	sc->rx.desc_ring[index].next =
	    htole32(sc->rx.desc_ring_paddr + DESC_OFF(RX_NEXT(index)));
	sc->rx.desc_ring[index].status = htole32(status);
}

static int
sunxi_emac_setup_rxbuf(struct sunxi_emac_softc *sc, int index, struct mbuf *m)
{
	int error;

	m_adj(m, ETHER_ALIGN);

	error = bus_dmamap_load_mbuf(sc->rx.buf_tag,
	    sc->rx.buf_map[index].map, m, BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error != 0)
		return error;

	bus_dmamap_sync(sc->rx.buf_tag, sc->rx.buf_map[index].map,
	    0, sc->rx.buf_map[index].map->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	sc->rx.buf_map[index].mbuf = m;
	sunxi_emac_setup_rxdesc(sc, index,
	    sc->rx.buf_map[index].map->dm_segs[0].ds_addr);

	return 0;
}

static struct mbuf *
sunxi_emac_alloc_mbufcl(struct sunxi_emac_softc *sc)
{
	struct mbuf *m;

	m = m_getcl(M_NOWAIT, MT_DATA, M_PKTHDR);
	if (m != NULL)
		m->m_pkthdr.len = m->m_len = m->m_ext.ext_size;

	return m;
}

static void
sunxi_emac_start_locked(struct sunxi_emac_softc *sc)
{
	struct ifnet *ifp = &sc->ec.ec_if;
	struct mbuf *m;
	uint32_t val;
	int cnt, nsegs, start;

	EMAC_ASSERT_LOCKED(sc);

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (cnt = 0, start = sc->tx.cur; ; cnt++) {
		if (sc->tx.queued >= TX_DESC_COUNT - TX_MAX_SEGS) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		nsegs = sunxi_emac_setup_txbuf(sc, sc->tx.cur, m);
		if (nsegs == 0) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m);
		bpf_mtap(ifp, m);

		sc->tx.cur = TX_SKIP(sc->tx.cur, nsegs);
	}

	if (cnt != 0) {
		sunxi_emac_dma_sync(sc, sc->tx.desc_tag, sc->tx.desc_map,
		    start, sc->tx.cur, TX_DESC_COUNT,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* Start and run TX DMA */
		val = RD4(sc, EMAC_TX_CTL_1);
		WR4(sc, EMAC_TX_CTL_1, val | TX_DMA_START);
	}
}

static void
sunxi_emac_start(struct ifnet *ifp)
{
	struct sunxi_emac_softc *sc = ifp->if_softc;

	EMAC_LOCK(sc);
	sunxi_emac_start_locked(sc);
	EMAC_UNLOCK(sc);
}

static void
sunxi_emac_tick(void *softc)
{
	struct sunxi_emac_softc *sc = softc;
	struct mii_data *mii = &sc->mii;
#ifndef EMAC_MPSAFE
	int s = splnet();
#endif

	EMAC_LOCK(sc);
	mii_tick(mii);
	callout_schedule(&sc->stat_ch, hz);
	EMAC_UNLOCK(sc);

#ifndef EMAC_MPSAFE
	splx(s);
#endif
}

/* Bit Reversal - http://aggregate.org/MAGIC/#Bit%20Reversal */
static uint32_t
bitrev32(uint32_t x)
{
	x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
	x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
	x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
	x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));

	return (x >> 16) | (x << 16);
}

static void
sunxi_emac_setup_rxfilter(struct sunxi_emac_softc *sc)
{
	struct ifnet *ifp = &sc->ec.ec_if;
	uint32_t val, crc, hashreg, hashbit, hash[2], machi, maclo;
	struct ether_multi *enm;
	struct ether_multistep step;
	const uint8_t *eaddr;

	EMAC_ASSERT_LOCKED(sc);

	val = 0;
	hash[0] = hash[1] = 0;

	if ((ifp->if_flags & IFF_PROMISC) != 0)
		val |= DIS_ADDR_FILTER;
	else if ((ifp->if_flags & IFF_ALLMULTI) != 0) {
		val |= RX_ALL_MULTICAST;
		hash[0] = hash[1] = ~0;
	} else {
		val |= HASH_MULTICAST;
		ETHER_FIRST_MULTI(step, &sc->ec, enm);
		while (enm != NULL) {
			crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);
			crc &= 0x7f;
			crc = bitrev32(~crc) >> 26;
			hashreg = (crc >> 5);
			hashbit = (crc & 0x1f);
			hash[hashreg] |= (1 << hashbit);
			ETHER_NEXT_MULTI(step, enm);
		}
	}

	/* Write our unicast address */
	eaddr = CLLADDR(ifp->if_sadl);
	machi = (eaddr[5] << 8) | eaddr[4];
	maclo = (eaddr[3] << 24) | (eaddr[2] << 16) | (eaddr[1] << 8) |
	   (eaddr[0] << 0);
	WR4(sc, EMAC_ADDR_HIGH(0), machi);
	WR4(sc, EMAC_ADDR_LOW(0), maclo);

	/* Multicast hash filters */
	WR4(sc, EMAC_RX_HASH_0, hash[1]);
	WR4(sc, EMAC_RX_HASH_1, hash[0]);

	/* RX frame filter config */
	WR4(sc, EMAC_RX_FRM_FLT, val);
}

static void
sunxi_emac_enable_intr(struct sunxi_emac_softc *sc)
{
	/* Enable interrupts */
	WR4(sc, EMAC_INT_EN, RX_INT_EN | TX_INT_EN | TX_BUF_UA_INT_EN);
}

static void
sunxi_emac_disable_intr(struct sunxi_emac_softc *sc)
{
	/* Disable interrupts */
	WR4(sc, EMAC_INT_EN, 0);
}

static int
sunxi_emac_init_locked(struct sunxi_emac_softc *sc)
{
	struct ifnet *ifp = &sc->ec.ec_if;
	struct mii_data *mii = &sc->mii;
	uint32_t val;

	EMAC_ASSERT_LOCKED(sc);

	if ((ifp->if_flags & IFF_RUNNING) != 0)
		return 0;

	sunxi_emac_setup_rxfilter(sc);

	/* Configure DMA burst length and priorities */
	val = sunxi_emac_burst_len << BASIC_CTL_BURST_LEN_SHIFT;
	if (sunxi_emac_rx_tx_pri)
		val |= BASIC_CTL_RX_TX_PRI;
	WR4(sc, EMAC_BASIC_CTL_1, val);

	/* Enable interrupts */
	sunxi_emac_enable_intr(sc);

	/* Enable transmit DMA */
	val = RD4(sc, EMAC_TX_CTL_1);
	WR4(sc, EMAC_TX_CTL_1, val | TX_DMA_EN | TX_MD | TX_NEXT_FRAME);

	/* Enable receive DMA */
	val = RD4(sc, EMAC_RX_CTL_1);
	WR4(sc, EMAC_RX_CTL_1, val | RX_DMA_EN | RX_MD);

	/* Enable transmitter */
	val = RD4(sc, EMAC_TX_CTL_0);
	WR4(sc, EMAC_TX_CTL_0, val | TX_EN);

	/* Enable receiver */
	val = RD4(sc, EMAC_RX_CTL_0);
	WR4(sc, EMAC_RX_CTL_0, val | RX_EN | CHECK_CRC);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	mii_mediachg(mii);
	callout_schedule(&sc->stat_ch, hz);

	return 0;
}

static int
sunxi_emac_init(struct ifnet *ifp)
{
	struct sunxi_emac_softc *sc = ifp->if_softc;
	int error;

	EMAC_LOCK(sc);
	error = sunxi_emac_init_locked(sc);
	EMAC_UNLOCK(sc);

	return error;
}

static void
sunxi_emac_stop_locked(struct sunxi_emac_softc *sc, int disable)
{
	struct ifnet *ifp = &sc->ec.ec_if;
	uint32_t val;

	EMAC_ASSERT_LOCKED(sc);

	callout_stop(&sc->stat_ch);

	mii_down(&sc->mii);

	/* Stop transmit DMA and flush data in the TX FIFO */
	val = RD4(sc, EMAC_TX_CTL_1);
	val &= ~TX_DMA_EN;
	val |= FLUSH_TX_FIFO;
	WR4(sc, EMAC_TX_CTL_1, val);

	/* Disable transmitter */
	val = RD4(sc, EMAC_TX_CTL_0);
	WR4(sc, EMAC_TX_CTL_0, val & ~TX_EN);

	/* Disable receiver */
	val = RD4(sc, EMAC_RX_CTL_0);
	WR4(sc, EMAC_RX_CTL_0, val & ~RX_EN);

	/* Disable interrupts */
	sunxi_emac_disable_intr(sc);

	/* Disable transmit DMA */
	val = RD4(sc, EMAC_TX_CTL_1);
	WR4(sc, EMAC_TX_CTL_1, val & ~TX_DMA_EN);

	/* Disable receive DMA */
	val = RD4(sc, EMAC_RX_CTL_1);
	WR4(sc, EMAC_RX_CTL_1, val & ~RX_DMA_EN);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static void
sunxi_emac_stop(struct ifnet *ifp, int disable)
{
	struct sunxi_emac_softc * const sc = ifp->if_softc;

	EMAC_LOCK(sc);
	sunxi_emac_stop_locked(sc, disable);
	EMAC_UNLOCK(sc);
}

static int
sunxi_emac_rxintr(struct sunxi_emac_softc *sc)
{
	struct ifnet *ifp = &sc->ec.ec_if;
	int error, index, len, npkt;
	struct mbuf *m, *m0;
	uint32_t status;

	npkt = 0;

	for (index = sc->rx.cur; ; index = RX_NEXT(index)) {
		sunxi_emac_dma_sync(sc, sc->rx.desc_tag, sc->rx.desc_map,
		    index, index + 1,
		    RX_DESC_COUNT, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		status = le32toh(sc->rx.desc_ring[index].status);
		if ((status & RX_DESC_CTL) != 0)
			break;

		bus_dmamap_sync(sc->rx.buf_tag, sc->rx.buf_map[index].map,
		    0, sc->rx.buf_map[index].map->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->rx.buf_tag, sc->rx.buf_map[index].map);

		len = (status & RX_FRM_LEN) >> RX_FRM_LEN_SHIFT;
		if (len != 0) {
			m = sc->rx.buf_map[index].mbuf;
			m_set_rcvif(m, ifp);
			m->m_flags |= M_HASFCS;
			m->m_pkthdr.len = len;
			m->m_len = len;
			m->m_nextpkt = NULL;

			if ((ifp->if_capenable & IFCAP_CSUM_IPv4_Rx) != 0 &&
			    (status & RX_FRM_TYPE) != 0) {
				m->m_pkthdr.csum_flags = M_CSUM_IPv4 |
				    M_CSUM_TCPv4 | M_CSUM_UDPv4;
				if ((status & RX_HEADER_ERR) != 0)
					m->m_pkthdr.csum_flags |=
					    M_CSUM_IPv4_BAD;
				if ((status & RX_PAYLOAD_ERR) != 0)
					m->m_pkthdr.csum_flags |=
					    M_CSUM_TCP_UDP_BAD;
			}

			++npkt;

			if_percpuq_enqueue(ifp->if_percpuq, m);
		}

		if ((m0 = sunxi_emac_alloc_mbufcl(sc)) != NULL) {
			error = sunxi_emac_setup_rxbuf(sc, index, m0);
			if (error != 0) {
				/* XXX hole in RX ring */
			}
		} else
			ifp->if_ierrors++;

		sunxi_emac_dma_sync(sc, sc->rx.desc_tag, sc->rx.desc_map,
		    index, index + 1,
		    RX_DESC_COUNT, BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD);
	}

	sc->rx.cur = index;

	return npkt;
}

static void
sunxi_emac_txintr(struct sunxi_emac_softc *sc)
{
	struct ifnet *ifp = &sc->ec.ec_if;
	struct sunxi_emac_bufmap *bmap;
	struct sunxi_emac_desc *desc;
	uint32_t status;
	int i;

	EMAC_ASSERT_LOCKED(sc);

	for (i = sc->tx.next; sc->tx.queued > 0; i = TX_NEXT(i)) {
		KASSERT(sc->tx.queued > 0 && sc->tx.queued <= TX_DESC_COUNT);
		sunxi_emac_dma_sync(sc, sc->tx.desc_tag, sc->tx.desc_map,
		    i, i + 1, TX_DESC_COUNT,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		desc = &sc->tx.desc_ring[i];
		status = le32toh(desc->status);
		if ((status & TX_DESC_CTL) != 0)
			break;
		bmap = &sc->tx.buf_map[i];
		if (bmap->mbuf != NULL) {
			bus_dmamap_sync(sc->tx.buf_tag, bmap->map,
			    0, bmap->map->dm_mapsize,
			    BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->tx.buf_tag, bmap->map);
			m_freem(bmap->mbuf);
			bmap->mbuf = NULL;
		}

		sunxi_emac_setup_txdesc(sc, i, 0, 0, 0);
		sunxi_emac_dma_sync(sc, sc->tx.desc_tag, sc->tx.desc_map,
		    i, i + 1, TX_DESC_COUNT,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		ifp->if_flags &= ~IFF_OACTIVE;
		ifp->if_opackets++;
	}

	sc->tx.next = i;
}

static int
sunxi_emac_intr(void *arg)
{
	struct sunxi_emac_softc *sc = arg;
	struct ifnet *ifp = &sc->ec.ec_if;
	uint32_t val;

	EMAC_LOCK(sc);

	val = RD4(sc, EMAC_INT_STA);
	WR4(sc, EMAC_INT_STA, val);

	if (val & RX_INT)
		sunxi_emac_rxintr(sc);

	if (val & (TX_INT|TX_BUF_UA_INT)) {
		sunxi_emac_txintr(sc);
		if_schedule_deferred_start(ifp);
	}

	EMAC_UNLOCK(sc);

	return 1;
}

static int
sunxi_emac_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct sunxi_emac_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->mii;
	struct ifreq *ifr = data;
	int error, s;

#ifndef EMAC_MPSAFE
	s = splnet();
#endif

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
#ifdef EMAC_MPSAFE
		s = splnet();
#endif
		error = ifmedia_ioctl(ifp, ifr, &mii->mii_media, cmd);
#ifdef EMAC_MPSAFE
		splx(s);
#endif
		break;
	default:
#ifdef EMAC_MPSAFE
		s = splnet();
#endif
		error = ether_ioctl(ifp, cmd, data);
#ifdef EMAC_MPSAFE
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
			EMAC_LOCK(sc);
			sunxi_emac_setup_rxfilter(sc);
			EMAC_UNLOCK(sc);
		}
		break;
	}

#ifndef EMAC_MPSAFE
	splx(s);
#endif

	return error;
}

static bool
sunxi_emac_has_internal_phy(struct sunxi_emac_softc *sc)
{
	const char * mdio_internal_compat[] = {
		"allwinner,sun8i-h3-mdio-internal",
		NULL
	};
	int phy;

	/* Non-standard property, for compatible with old dts files */
	if (of_hasprop(sc->phandle, "allwinner,use-internal-phy"))
		return true;

	phy = fdtbus_get_phandle(sc->phandle, "phy-handle");
	if (phy == -1)
		return false;

	/* For internal PHY, check compatible string of parent node */
	return of_compatible(OF_parent(phy), mdio_internal_compat) >= 0;
}

static int
sunxi_emac_setup_phy(struct sunxi_emac_softc *sc)
{
	uint32_t reg, tx_delay, rx_delay;
	const char *phy_type;

	phy_type = fdtbus_get_string(sc->phandle, "phy-mode");
	if (phy_type == NULL)
		return 0;

	aprint_debug_dev(sc->dev, "PHY type: %s\n", phy_type);

	reg = SYSCONRD4(sc, 0);

	reg &= ~(EMAC_CLK_PIT | EMAC_CLK_SRC | EMAC_CLK_RMII_EN);
	if (strcmp(phy_type, "rgmii") == 0)
		reg |= EMAC_CLK_PIT_RGMII | EMAC_CLK_SRC_RGMII;
	else if (strcmp(phy_type, "rmii") == 0)
		reg |= EMAC_CLK_RMII_EN;
	else
		reg |= EMAC_CLK_PIT_MII | EMAC_CLK_SRC_MII;

	if (of_getprop_uint32(sc->phandle, "tx-delay", &tx_delay) == 0) {
		reg &= ~EMAC_CLK_ETXDC;
		reg |= (tx_delay << EMAC_CLK_ETXDC_SHIFT);
	}
	if (of_getprop_uint32(sc->phandle, "rx-delay", &rx_delay) == 0) {
		reg &= ~EMAC_CLK_ERXDC;
		reg |= (rx_delay << EMAC_CLK_ERXDC_SHIFT);
	}

	if (sc->type == EMAC_H3) {
		if (sunxi_emac_has_internal_phy(sc)) {
			reg |= EMAC_CLK_EPHY_SELECT;
			reg &= ~EMAC_CLK_EPHY_SHUTDOWN;
			if (of_hasprop(sc->phandle,
			    "allwinner,leds-active-low"))
				reg |= EMAC_CLK_EPHY_LED_POL;
			else
				reg &= ~EMAC_CLK_EPHY_LED_POL;

			/* Set internal PHY addr to 1 */
			reg &= ~EMAC_CLK_EPHY_ADDR;
			reg |= (1 << EMAC_CLK_EPHY_ADDR_SHIFT);
		} else {
			reg &= ~EMAC_CLK_EPHY_SELECT;
		}
	}

	aprint_debug_dev(sc->dev, "EMAC clock: 0x%08x\n", reg);

	SYSCONWR4(sc, 0, reg);

	return 0;
}

static int
sunxi_emac_setup_resources(struct sunxi_emac_softc *sc)
{
	u_int freq;
	int error, div;

	/* Configure PHY for MII or RGMII mode */
	if (sunxi_emac_setup_phy(sc) != 0)
		return ENXIO;

	/* Enable clocks */
	error = clk_enable(sc->clk_ahb);
	if (error != 0) {
		aprint_error_dev(sc->dev, "cannot enable ahb clock\n");
		return error;
	}

	if (sc->clk_ephy != NULL) {
		error = clk_enable(sc->clk_ephy);
		if (error != 0) {
			aprint_error_dev(sc->dev, "cannot enable ephy clock\n");
			return error;
		}
	}

	/* De-assert reset */
	error = fdtbus_reset_deassert(sc->rst_ahb);
	if (error != 0) {
		aprint_error_dev(sc->dev, "cannot de-assert ahb reset\n");
		return error;
	}
	if (sc->rst_ephy != NULL) {
		error = fdtbus_reset_deassert(sc->rst_ephy);
		if (error != 0) {
			aprint_error_dev(sc->dev,
			    "cannot de-assert ephy reset\n");
			return error;
		}
	}

	/* Enable PHY regulator if applicable */
	if (sc->reg_phy != NULL) {
		error = fdtbus_regulator_enable(sc->reg_phy);
		if (error != 0) {
			aprint_error_dev(sc->dev,
			    "cannot enable PHY regulator\n");
			return error;
		}
	}

	/* Determine MDC clock divide ratio based on AHB clock */
	freq = clk_get_rate(sc->clk_ahb);
	if (freq == 0) {
		aprint_error_dev(sc->dev, "cannot get AHB clock frequency\n");
		return ENXIO;
	}
	div = freq / MDIO_FREQ;
	if (div <= 16)
		sc->mdc_div_ratio_m = MDC_DIV_RATIO_M_16;
	else if (div <= 32)
		sc->mdc_div_ratio_m = MDC_DIV_RATIO_M_32;
	else if (div <= 64)
		sc->mdc_div_ratio_m = MDC_DIV_RATIO_M_64;
	else if (div <= 128)
		sc->mdc_div_ratio_m = MDC_DIV_RATIO_M_128;
	else {
		aprint_error_dev(sc->dev,
		    "cannot determine MDC clock divide ratio\n");
		return ENXIO;
	}

	aprint_debug_dev(sc->dev, "AHB frequency %u Hz, MDC div: 0x%x\n",
	    freq, sc->mdc_div_ratio_m);

	return 0;
}

static void 
sunxi_emac_get_eaddr(struct sunxi_emac_softc *sc, uint8_t *eaddr)
{
	uint32_t maclo, machi;
#if notyet
	u_char rootkey[16];
#endif

	machi = RD4(sc, EMAC_ADDR_HIGH(0)) & 0xffff;
	maclo = RD4(sc, EMAC_ADDR_LOW(0));

	if (maclo == 0xffffffff && machi == 0xffff) {
#if notyet
		/* MAC address in hardware is invalid, create one */
		if (aw_sid_get_rootkey(rootkey) == 0 &&
		    (rootkey[3] | rootkey[12] | rootkey[13] | rootkey[14] |
		     rootkey[15]) != 0) {
			/* MAC address is derived from the root key in SID */
			maclo = (rootkey[13] << 24) | (rootkey[12] << 16) |
				(rootkey[3] << 8) | 0x02;
			machi = (rootkey[15] << 8) | rootkey[14];
		} else {
#endif
			/* Create one */
			maclo = 0x00f2 | (cprng_strong32() & 0xffff0000);
			machi = cprng_strong32() & 0xffff;
#if notyet
		}
#endif
	}

	eaddr[0] = maclo & 0xff;
	eaddr[1] = (maclo >> 8) & 0xff;
	eaddr[2] = (maclo >> 16) & 0xff;
	eaddr[3] = (maclo >> 24) & 0xff;
	eaddr[4] = machi & 0xff;
	eaddr[5] = (machi >> 8) & 0xff;
}

#ifdef SUNXI_EMAC_DEBUG
static void
sunxi_emac_dump_regs(struct sunxi_emac_softc *sc)
{
	static const struct {
		const char *name;
		u_int reg;
	} regs[] = {
		{ "BASIC_CTL_0", EMAC_BASIC_CTL_0 },
		{ "BASIC_CTL_1", EMAC_BASIC_CTL_1 },
		{ "INT_STA", EMAC_INT_STA },
		{ "INT_EN", EMAC_INT_EN },
		{ "TX_CTL_0", EMAC_TX_CTL_0 },
		{ "TX_CTL_1", EMAC_TX_CTL_1 },
		{ "TX_FLOW_CTL", EMAC_TX_FLOW_CTL },
		{ "TX_DMA_LIST", EMAC_TX_DMA_LIST },
		{ "RX_CTL_0", EMAC_RX_CTL_0 },
		{ "RX_CTL_1", EMAC_RX_CTL_1 },
		{ "RX_DMA_LIST", EMAC_RX_DMA_LIST },
		{ "RX_FRM_FLT", EMAC_RX_FRM_FLT },
		{ "RX_HASH_0", EMAC_RX_HASH_0 },
		{ "RX_HASH_1", EMAC_RX_HASH_1 },
		{ "MII_CMD", EMAC_MII_CMD },
		{ "ADDR_HIGH0", EMAC_ADDR_HIGH(0) },
		{ "ADDR_LOW0", EMAC_ADDR_LOW(0) },
		{ "TX_DMA_STA", EMAC_TX_DMA_STA },
		{ "TX_DMA_CUR_DESC", EMAC_TX_DMA_CUR_DESC },
		{ "TX_DMA_CUR_BUF", EMAC_TX_DMA_CUR_BUF },
		{ "RX_DMA_STA", EMAC_RX_DMA_STA },
		{ "RX_DMA_CUR_DESC", EMAC_RX_DMA_CUR_DESC },
		{ "RX_DMA_CUR_BUF", EMAC_RX_DMA_CUR_BUF },
		{ "RGMII_STA", EMAC_RGMII_STA },
	};
	u_int n;

	for (n = 0; n < __arraycount(regs); n++)
		device_printf(dev, "  %-20s %08x\n", regs[n].name,
		    RD4(sc, regs[n].reg));
}
#endif

static int
sunxi_emac_phy_reset(struct sunxi_emac_softc *sc)
{
	uint32_t delay_prop[3];
	int pin_value;

	if (sc->pin_reset == NULL)
		return 0;

	if (OF_getprop(sc->phandle, "allwinner,reset-delays-us", delay_prop,
	    sizeof(delay_prop)) <= 0)
		return ENXIO;

	pin_value = of_hasprop(sc->phandle, "allwinner,reset-active-low");

	fdtbus_gpio_write(sc->pin_reset, pin_value);
	delay(htole32(delay_prop[0]));
	fdtbus_gpio_write(sc->pin_reset, !pin_value);
	delay(htole32(delay_prop[1]));
	fdtbus_gpio_write(sc->pin_reset, pin_value);
	delay(htole32(delay_prop[2]));

	return 0;
}

static int
sunxi_emac_reset(struct sunxi_emac_softc *sc)
{
	int retry;

	/* Reset PHY if necessary */
	if (sunxi_emac_phy_reset(sc) != 0) {
		aprint_error_dev(sc->dev, "failed to reset PHY\n");
		return ENXIO;
	}

	/* Soft reset all registers and logic */
	WR4(sc, EMAC_BASIC_CTL_1, BASIC_CTL_SOFT_RST);

	/* Wait for soft reset bit to self-clear */
	for (retry = SOFT_RST_RETRY; retry > 0; retry--) {
		if ((RD4(sc, EMAC_BASIC_CTL_1) & BASIC_CTL_SOFT_RST) == 0)
			break;
		delay(10);
	}
	if (retry == 0) {
		aprint_error_dev(sc->dev, "soft reset timed out\n");
#ifdef SUNXI_EMAC_DEBUG
		sunxi_emac_dump_regs(sc);
#endif
		return ETIMEDOUT;
	}

	return 0;
}

static int
sunxi_emac_setup_dma(struct sunxi_emac_softc *sc)
{
	struct mbuf *m;
	int error, nsegs, i;

	/* Setup TX ring */
	sc->tx.buf_tag = sc->tx.desc_tag = sc->dmat;
	error = bus_dmamap_create(sc->dmat, TX_DESC_SIZE, 1, TX_DESC_SIZE, 0,
	    BUS_DMA_WAITOK, &sc->tx.desc_map);
	if (error)
		return error;
	error = bus_dmamem_alloc(sc->dmat, TX_DESC_SIZE, DESC_ALIGN, 0,
	    &sc->tx.desc_dmaseg, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->dmat, &sc->tx.desc_dmaseg, nsegs,
	    TX_DESC_SIZE, (void *)&sc->tx.desc_ring,
	    BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamap_load(sc->dmat, sc->tx.desc_map, sc->tx.desc_ring,
	    TX_DESC_SIZE, NULL, BUS_DMA_WAITOK);
	if (error)
		return error;
	sc->tx.desc_ring_paddr = sc->tx.desc_map->dm_segs[0].ds_addr;

	memset(sc->tx.desc_ring, 0, TX_DESC_SIZE);
	bus_dmamap_sync(sc->dmat, sc->tx.desc_map, 0, TX_DESC_SIZE,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	for (i = 0; i < TX_DESC_COUNT; i++)
		sc->tx.desc_ring[i].next =
		    htole32(sc->tx.desc_ring_paddr + DESC_OFF(TX_NEXT(i)));

	sc->tx.queued = TX_DESC_COUNT;
	for (i = 0; i < TX_DESC_COUNT; i++) {
		error = bus_dmamap_create(sc->tx.buf_tag, MCLBYTES,
		    TX_MAX_SEGS, MCLBYTES, 0, BUS_DMA_WAITOK,
		    &sc->tx.buf_map[i].map);
		if (error != 0) {
			device_printf(sc->dev, "cannot create TX buffer map\n");
			return error;
		}
		sunxi_emac_setup_txdesc(sc, i, 0, 0, 0);
	}

	/* Setup RX ring */
	sc->rx.buf_tag = sc->rx.desc_tag = sc->dmat;
	error = bus_dmamap_create(sc->dmat, RX_DESC_SIZE, 1, RX_DESC_SIZE, 0,
	    BUS_DMA_WAITOK, &sc->rx.desc_map);
	if (error)
		return error;
	error = bus_dmamem_alloc(sc->dmat, RX_DESC_SIZE, DESC_ALIGN, 0,
	    &sc->rx.desc_dmaseg, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->dmat, &sc->rx.desc_dmaseg, nsegs,
	    RX_DESC_SIZE, (void *)&sc->rx.desc_ring,
	    BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamap_load(sc->dmat, sc->rx.desc_map, sc->rx.desc_ring,
	    RX_DESC_SIZE, NULL, BUS_DMA_WAITOK);
	if (error)
		return error;
	sc->rx.desc_ring_paddr = sc->rx.desc_map->dm_segs[0].ds_addr;

	memset(sc->rx.desc_ring, 0, RX_DESC_SIZE);

	for (i = 0; i < RX_DESC_COUNT; i++) {
		error = bus_dmamap_create(sc->rx.buf_tag, MCLBYTES,
		    RX_DESC_COUNT, MCLBYTES, 0, BUS_DMA_WAITOK,
		    &sc->rx.buf_map[i].map);
		if (error != 0) {
			device_printf(sc->dev, "cannot create RX buffer map\n");
			return error;
		}
		if ((m = sunxi_emac_alloc_mbufcl(sc)) == NULL) {
			device_printf(sc->dev, "cannot allocate RX mbuf\n");
			return ENOMEM;
		}
		error = sunxi_emac_setup_rxbuf(sc, i, m);
		if (error != 0) {
			device_printf(sc->dev, "cannot create RX buffer\n");
			return error;
		}
	}
	bus_dmamap_sync(sc->rx.desc_tag, sc->rx.desc_map,
	    0, sc->rx.desc_map->dm_mapsize,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/* Write transmit and receive descriptor base address registers */
	WR4(sc, EMAC_TX_DMA_LIST, sc->tx.desc_ring_paddr);
	WR4(sc, EMAC_RX_DMA_LIST, sc->rx.desc_ring_paddr);

	return 0;
}

static int
sunxi_emac_get_resources(struct sunxi_emac_softc *sc)
{
	const int phandle = sc->phandle;
	bus_addr_t addr, size;

	/* Map EMAC registers */
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0)
		return ENXIO;
	if (bus_space_map(sc->bst, addr, size, 0, &sc->bsh[_RES_EMAC]) != 0)
		return ENXIO;

	/* Map SYSCON registers */
	if (of_hasprop(phandle, "syscon")) {
		const int syscon_phandle = fdtbus_get_phandle(phandle,
		    "syscon");
		if (syscon_phandle == -1)
			return ENXIO;
		if (fdtbus_get_reg(syscon_phandle, 0, &addr, &size) != 0)
			return ENXIO;
		if (size < EMAC_CLK_REG + 4)
			return ENXIO;
		addr += EMAC_CLK_REG;
		size -= EMAC_CLK_REG;
	} else {
		if (fdtbus_get_reg(phandle, 1, &addr, &size) != 0)
			return ENXIO;
	}
	if (bus_space_map(sc->bst, addr, size, 0, &sc->bsh[_RES_SYSCON]) != 0)
		return ENXIO;

	/* The "ahb"/"stmmaceth" clock and reset is required */
	if ((sc->clk_ahb = fdtbus_clock_get(phandle, "ahb")) == NULL &&
	    (sc->clk_ahb = fdtbus_clock_get(phandle, "stmmaceth")) == NULL)
		return ENXIO;
	if ((sc->rst_ahb = fdtbus_reset_get(phandle, "ahb")) == NULL &&
	    (sc->rst_ahb = fdtbus_reset_get(phandle, "stmmaceth")) == NULL)
		return ENXIO;

	/* Internal PHY clock and reset are optional properties. */
	sc->clk_ephy = fdtbus_clock_get(phandle, "ephy");
	if (sc->clk_ephy == NULL) {
		int phy_phandle = fdtbus_get_phandle(phandle, "phy-handle");
		if (phy_phandle != -1)
			sc->clk_ephy = fdtbus_clock_get_index(phy_phandle, 0);
	}
	sc->rst_ephy = fdtbus_reset_get(phandle, "ephy");
	if (sc->rst_ephy == NULL) {
		int phy_phandle = fdtbus_get_phandle(phandle, "phy-handle");
		if (phy_phandle != -1)
			sc->rst_ephy = fdtbus_reset_get_index(phy_phandle, 0);
	}

	/* Regulator is optional */
	sc->reg_phy = fdtbus_regulator_acquire(phandle, "phy-supply");

	/* Reset GPIO is optional */
	sc->pin_reset = fdtbus_gpio_acquire(sc->phandle,
	    "allwinner,reset-gpio", GPIO_PIN_OUTPUT);

	return 0;
}

static int
sunxi_emac_get_phyid(struct sunxi_emac_softc *sc)
{
	bus_addr_t addr;
	int phy_phandle;

	phy_phandle = fdtbus_get_phandle(sc->phandle, "phy");
	if (phy_phandle == -1)
		phy_phandle = fdtbus_get_phandle(sc->phandle, "phy-handle");
	if (phy_phandle == -1)
		return MII_PHY_ANY;

	if (fdtbus_get_reg(phy_phandle, 0, &addr, NULL) != 0)
		return MII_PHY_ANY;

	return (int)addr;
}

static int
sunxi_emac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_emac_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	struct sunxi_emac_softc * const sc = device_private(self);
	const int phandle = faa->faa_phandle;
	struct mii_data *mii = &sc->mii;
	struct ifnet *ifp = &sc->ec.ec_if;
	uint8_t eaddr[ETHER_ADDR_LEN];
	char intrstr[128];

	sc->dev = self;
	sc->phandle = phandle;
	sc->bst = faa->faa_bst;
	sc->dmat = faa->faa_dmat;
	sc->type = of_search_compatible(phandle, compat_data)->data;
	sc->phy_id = sunxi_emac_get_phyid(sc);

	if (sunxi_emac_get_resources(sc) != 0) {
		aprint_error(": cannot allocate resources for device\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": cannot decode interrupt\n");
		return;
	}

	mutex_init(&sc->mtx, MUTEX_DEFAULT, IPL_NET);
	callout_init(&sc->stat_ch, CALLOUT_FLAGS);
	callout_setfunc(&sc->stat_ch, sunxi_emac_tick, sc);

	aprint_naive("\n");
	aprint_normal(": EMAC\n");

	/* Setup clocks and regulators */
	if (sunxi_emac_setup_resources(sc) != 0)
		return;

	/* Read MAC address before resetting the chip */
	sunxi_emac_get_eaddr(sc, eaddr);

	/* Soft reset EMAC core */
	if (sunxi_emac_reset(sc) != 0)
		return;

	/* Setup DMA descriptors */
	if (sunxi_emac_setup_dma(sc) != 0) {
		aprint_error_dev(self, "failed to setup DMA descriptors\n");
		return;
	}

	/* Install interrupt handler */
	sc->ih = fdtbus_intr_establish(phandle, 0, IPL_NET,
	    FDT_INTR_FLAGS, sunxi_emac_intr, sc);
	if (sc->ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/* Setup ethernet interface */
	ifp->if_softc = sc;
	snprintf(ifp->if_xname, IFNAMSIZ, EMAC_IFNAME, device_unit(self));
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef EMAC_MPSAFE
	ifp->if_extflags = IFEF_MPSAFE;
#endif
	ifp->if_start = sunxi_emac_start;
	ifp->if_ioctl = sunxi_emac_ioctl;
	ifp->if_init = sunxi_emac_init;
	ifp->if_stop = sunxi_emac_stop;
	ifp->if_capabilities = IFCAP_CSUM_IPv4_Rx |
			       IFCAP_CSUM_IPv4_Tx |
			       IFCAP_CSUM_TCPv4_Rx |
			       IFCAP_CSUM_TCPv4_Tx |
			       IFCAP_CSUM_UDPv4_Rx |
			       IFCAP_CSUM_UDPv4_Tx;
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifp->if_snd);

	/* 802.1Q VLAN-sized frames are supported */
	sc->ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

	/* Attach MII driver */
	sc->ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
	mii->mii_ifp = ifp;
	mii->mii_readreg = sunxi_emac_mii_readreg;
	mii->mii_writereg = sunxi_emac_mii_writereg;
	mii->mii_statchg = sunxi_emac_mii_statchg;
	mii_attach(self, mii, 0xffffffff, sc->phy_id, MII_OFFSET_ANY,
	    MIIF_DOPAUSE);

	if (LIST_EMPTY(&mii->mii_phys)) {
		aprint_error_dev(self, "no PHY found!\n");
		return;
	}
	ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);

	/* Attach interface */
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);

	/* Attach ethernet interface */
	ether_ifattach(ifp, eaddr);
}

CFATTACH_DECL_NEW(sunxi_emac, sizeof(struct sunxi_emac_softc),
    sunxi_emac_match, sunxi_emac_attach, NULL, NULL);
