/* $NetBSD: bcmgenet.c,v 1.11 2021/12/31 14:25:22 riastradh Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
 * Broadcom GENETv5
 */

#include "opt_net_mpsafe.h"
#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcmgenet.c,v 1.11 2021/12/31 14:25:22 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/callout.h>
#include <sys/cprng.h>

#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#include <dev/mii/miivar.h>

#include <dev/ic/bcmgenetreg.h>
#include <dev/ic/bcmgenetvar.h>

CTASSERT(MCLBYTES == 2048);

#ifdef GENET_DEBUG
#define	DPRINTF(...)	printf(##__VA_ARGS__)
#else
#define	DPRINTF(...)	((void)0)
#endif

#ifdef NET_MPSAFE
#define	GENET_MPSAFE		1
#define	CALLOUT_FLAGS		CALLOUT_MPSAFE
#else
#define	CALLOUT_FLAGS		0
#endif

#define	TX_MAX_SEGS		128
#define	TX_DESC_COUNT		256 /* GENET_DMA_DESC_COUNT */
#define	RX_DESC_COUNT		256 /* GENET_DMA_DESC_COUNT */
#define	MII_BUSY_RETRY		1000
#define	GENET_MAX_MDF_FILTER	17

#define	TX_SKIP(n, o)		(((n) + (o)) % TX_DESC_COUNT)
#define	TX_NEXT(n)		TX_SKIP(n, 1)
#define	RX_NEXT(n)		(((n) + 1) % RX_DESC_COUNT)

#define	GENET_LOCK(sc)		mutex_enter(&(sc)->sc_lock)
#define	GENET_UNLOCK(sc)	mutex_exit(&(sc)->sc_lock)
#define	GENET_ASSERT_LOCKED(sc)	KASSERT(mutex_owned(&(sc)->sc_lock))

#define	GENET_TXLOCK(sc)		mutex_enter(&(sc)->sc_txlock)
#define	GENET_TXUNLOCK(sc)		mutex_exit(&(sc)->sc_txlock)
#define	GENET_ASSERT_TXLOCKED(sc)	KASSERT(mutex_owned(&(sc)->sc_txlock))

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
genet_mii_readreg(device_t dev, int phy, int reg, uint16_t *val)
{
	struct genet_softc *sc = device_private(dev);
	int retry;

	WR4(sc, GENET_MDIO_CMD,
	    GENET_MDIO_READ | GENET_MDIO_START_BUSY |
	    __SHIFTIN(phy, GENET_MDIO_PMD) |
	    __SHIFTIN(reg, GENET_MDIO_REG));
	for (retry = MII_BUSY_RETRY; retry > 0; retry--) {
		if ((RD4(sc, GENET_MDIO_CMD) & GENET_MDIO_START_BUSY) == 0) {
			*val = RD4(sc, GENET_MDIO_CMD) & 0xffff;
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
genet_mii_writereg(device_t dev, int phy, int reg, uint16_t val)
{
	struct genet_softc *sc = device_private(dev);
	int retry;

	WR4(sc, GENET_MDIO_CMD,
	    val | GENET_MDIO_WRITE | GENET_MDIO_START_BUSY |
	    __SHIFTIN(phy, GENET_MDIO_PMD) |
	    __SHIFTIN(reg, GENET_MDIO_REG));
	for (retry = MII_BUSY_RETRY; retry > 0; retry--) {
		if ((RD4(sc, GENET_MDIO_CMD) & GENET_MDIO_START_BUSY) == 0)
			break;
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
genet_update_link(struct genet_softc *sc)
{
	struct mii_data *mii = &sc->sc_mii;
	uint32_t val;
	u_int speed;

	if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T ||
	    IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_SX)
		speed = GENET_UMAC_CMD_SPEED_1000;
	else if (IFM_SUBTYPE(mii->mii_media_active) == IFM_100_TX)
		speed = GENET_UMAC_CMD_SPEED_100;
	else
		speed = GENET_UMAC_CMD_SPEED_10;

	val = RD4(sc, GENET_EXT_RGMII_OOB_CTRL);
	val &= ~GENET_EXT_RGMII_OOB_OOB_DISABLE;
	val |= GENET_EXT_RGMII_OOB_RGMII_LINK;
	val |= GENET_EXT_RGMII_OOB_RGMII_MODE_EN;
	if (sc->sc_phy_mode == GENET_PHY_MODE_RGMII)
		val |= GENET_EXT_RGMII_OOB_ID_MODE_DISABLE;
	else
		val &= ~GENET_EXT_RGMII_OOB_ID_MODE_DISABLE;
	WR4(sc, GENET_EXT_RGMII_OOB_CTRL, val);

	val = RD4(sc, GENET_UMAC_CMD);
	val &= ~GENET_UMAC_CMD_SPEED;
	val |= __SHIFTIN(speed, GENET_UMAC_CMD_SPEED);
	WR4(sc, GENET_UMAC_CMD, val);
}

static void
genet_mii_statchg(struct ifnet *ifp)
{
	struct genet_softc * const sc = ifp->if_softc;

	genet_update_link(sc);
}

static void
genet_setup_txdesc(struct genet_softc *sc, int index, int flags,
    bus_addr_t paddr, u_int len)
{
	uint32_t status;

	status = flags | __SHIFTIN(len, GENET_TX_DESC_STATUS_BUFLEN);

	WR4(sc, GENET_TX_DESC_ADDRESS_LO(index), (uint32_t)paddr);
	WR4(sc, GENET_TX_DESC_ADDRESS_HI(index), (uint32_t)(paddr >> 32));
	WR4(sc, GENET_TX_DESC_STATUS(index), status);
}

static int
genet_setup_txbuf(struct genet_softc *sc, int index, struct mbuf *m)
{
	bus_dma_segment_t *segs;
	int error, nsegs, cur, i;
	uint32_t flags;
	bool nospace;

	/* at least one descriptor free ? */
	if (sc->sc_tx.queued >= TX_DESC_COUNT - 1)
		return -1;

	error = bus_dmamap_load_mbuf(sc->sc_tx.buf_tag,
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
		bus_dmamap_unload(sc->sc_tx.buf_tag,
		    sc->sc_tx.buf_map[index].map);
		/* XXX coalesce and retry ? */
		return -1;
	}

	bus_dmamap_sync(sc->sc_tx.buf_tag, sc->sc_tx.buf_map[index].map,
	    0, sc->sc_tx.buf_map[index].map->dm_mapsize, BUS_DMASYNC_PREWRITE);

	/* stored in same index as loaded map */
	sc->sc_tx.buf_map[index].mbuf = m;

	flags = GENET_TX_DESC_STATUS_SOP |
		GENET_TX_DESC_STATUS_CRC |
		GENET_TX_DESC_STATUS_QTAG;

	for (cur = index, i = 0; i < nsegs; i++) {
		if (i == nsegs - 1)
			flags |= GENET_TX_DESC_STATUS_EOP;

		genet_setup_txdesc(sc, cur, flags, segs[i].ds_addr,
		    segs[i].ds_len);

		if (i == 0)
			flags &= ~GENET_TX_DESC_STATUS_SOP;
		cur = TX_NEXT(cur);
	}

	return nsegs;
}

static void
genet_setup_rxdesc(struct genet_softc *sc, int index,
    bus_addr_t paddr, bus_size_t len)
{
	WR4(sc, GENET_RX_DESC_ADDRESS_LO(index), (uint32_t)paddr);
	WR4(sc, GENET_RX_DESC_ADDRESS_HI(index), (uint32_t)(paddr >> 32));
}

static int
genet_setup_rxbuf(struct genet_softc *sc, int index, struct mbuf *m)
{
	int error;

	error = bus_dmamap_load_mbuf(sc->sc_rx.buf_tag,
	    sc->sc_rx.buf_map[index].map, m, BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error != 0)
		return error;

	bus_dmamap_sync(sc->sc_rx.buf_tag, sc->sc_rx.buf_map[index].map,
	    0, sc->sc_rx.buf_map[index].map->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	sc->sc_rx.buf_map[index].mbuf = m;
	genet_setup_rxdesc(sc, index,
	    sc->sc_rx.buf_map[index].map->dm_segs[0].ds_addr,
	    sc->sc_rx.buf_map[index].map->dm_segs[0].ds_len);

	return 0;
}

static struct mbuf *
genet_alloc_mbufcl(struct genet_softc *sc)
{
	struct mbuf *m;

	m = m_getcl(M_NOWAIT, MT_DATA, M_PKTHDR);
	if (m != NULL)
		m->m_pkthdr.len = m->m_len = m->m_ext.ext_size;

	return m;
}

static void
genet_enable_intr(struct genet_softc *sc)
{
	WR4(sc, GENET_INTRL2_CPU_CLEAR_MASK,
	    GENET_IRQ_TXDMA_DONE | GENET_IRQ_RXDMA_DONE);
}

static void
genet_disable_intr(struct genet_softc *sc)
{
	/* Disable interrupts */
	WR4(sc, GENET_INTRL2_CPU_SET_MASK, 0xffffffff);
	WR4(sc, GENET_INTRL2_CPU_CLEAR, 0xffffffff);
}

static void
genet_tick(void *softc)
{
	struct genet_softc *sc = softc;
	struct mii_data *mii = &sc->sc_mii;
#ifndef GENET_MPSAFE
	int s = splnet();
#endif

	GENET_LOCK(sc);
	mii_tick(mii);
	callout_schedule(&sc->sc_stat_ch, hz);
	GENET_UNLOCK(sc);

#ifndef GENET_MPSAFE
	splx(s);
#endif
}

static void
genet_setup_rxfilter_mdf(struct genet_softc *sc, u_int n, const uint8_t *ea)
{
	uint32_t addr0 = (ea[0] << 8) | ea[1];
	uint32_t addr1 = (ea[2] << 24) | (ea[3] << 16) | (ea[4] << 8) | ea[5];

	WR4(sc, GENET_UMAC_MDF_ADDR0(n), addr0);
	WR4(sc, GENET_UMAC_MDF_ADDR1(n), addr1);
}

static void
genet_setup_rxfilter(struct genet_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multistep step;
	struct ether_multi *enm;
	uint32_t cmd, mdf_ctrl;
	u_int n;

	GENET_ASSERT_LOCKED(sc);

	ETHER_LOCK(ec);

	cmd = RD4(sc, GENET_UMAC_CMD);

	/*
	 * Count the required number of hardware filters. We need one
	 * for each multicast address, plus one for our own address and
	 * the broadcast address.
	 */
	ETHER_FIRST_MULTI(step, ec, enm);
	for (n = 2; enm != NULL; n++)
		ETHER_NEXT_MULTI(step, enm);

	if (n > GENET_MAX_MDF_FILTER)
		ifp->if_flags |= IFF_ALLMULTI;
	else
		ifp->if_flags &= ~IFF_ALLMULTI;

	if ((ifp->if_flags & (IFF_PROMISC|IFF_ALLMULTI)) != 0) {
		cmd |= GENET_UMAC_CMD_PROMISC;
		mdf_ctrl = 0;
	} else {
		cmd &= ~GENET_UMAC_CMD_PROMISC;
		genet_setup_rxfilter_mdf(sc, 0, ifp->if_broadcastaddr);
		genet_setup_rxfilter_mdf(sc, 1, CLLADDR(ifp->if_sadl));
		ETHER_FIRST_MULTI(step, ec, enm);
		for (n = 2; enm != NULL; n++) {
			genet_setup_rxfilter_mdf(sc, n, enm->enm_addrlo);
			ETHER_NEXT_MULTI(step, enm);
		}
		mdf_ctrl = __BITS(GENET_MAX_MDF_FILTER - 1,
				  GENET_MAX_MDF_FILTER - n);
	}

	WR4(sc, GENET_UMAC_CMD, cmd);
	WR4(sc, GENET_UMAC_MDF_CTRL, mdf_ctrl);

	ETHER_UNLOCK(ec);
}

static int
genet_reset(struct genet_softc *sc)
{
	uint32_t val;

	val = RD4(sc, GENET_SYS_RBUF_FLUSH_CTRL);
	val |= GENET_SYS_RBUF_FLUSH_RESET;
	WR4(sc, GENET_SYS_RBUF_FLUSH_CTRL, val);
	delay(10);

	val &= ~GENET_SYS_RBUF_FLUSH_RESET;
	WR4(sc, GENET_SYS_RBUF_FLUSH_CTRL, val);
	delay(10);

	WR4(sc, GENET_SYS_RBUF_FLUSH_CTRL, 0);
	delay(10);

	WR4(sc, GENET_UMAC_CMD, 0);
	WR4(sc, GENET_UMAC_CMD,
	    GENET_UMAC_CMD_LCL_LOOP_EN | GENET_UMAC_CMD_SW_RESET);
	delay(10);
	WR4(sc, GENET_UMAC_CMD, 0);

	WR4(sc, GENET_UMAC_MIB_CTRL, GENET_UMAC_MIB_RESET_RUNT |
	    GENET_UMAC_MIB_RESET_RX | GENET_UMAC_MIB_RESET_TX);
	WR4(sc, GENET_UMAC_MIB_CTRL, 0);

	WR4(sc, GENET_UMAC_MAX_FRAME_LEN, 1536);

	val = RD4(sc, GENET_RBUF_CTRL);
	val |= GENET_RBUF_ALIGN_2B;
	WR4(sc, GENET_RBUF_CTRL, val);

	WR4(sc, GENET_RBUF_TBUF_SIZE_CTRL, 1);

	return 0;
}

static void
genet_set_rxthresh(struct genet_softc *sc, int qid, int usecs, int count)
{
	int ticks;
	uint32_t val;

	/* convert to 125MHz/1024 ticks */
	ticks = howmany(usecs * 125, 1024);

	if (count < 1)
		count = 1;
	if (count > GENET_INTR_THRESHOLD_MASK)
		count = GENET_INTR_THRESHOLD_MASK;
	if (ticks < 0)
		ticks = 0;
	if (ticks > GENET_DMA_RING_TIMEOUT_MASK)
		ticks = GENET_DMA_RING_TIMEOUT_MASK;

	WR4(sc, GENET_RX_DMA_MBUF_DONE_THRES(qid), count);

	val = RD4(sc, GENET_RX_DMA_RING_TIMEOUT(qid));
	val &= ~GENET_DMA_RING_TIMEOUT_MASK;
	val |= ticks;
	WR4(sc, GENET_RX_DMA_RING_TIMEOUT(qid), val);
}

static void
genet_set_txthresh(struct genet_softc *sc, int qid, int count)
{
	if (count < 1)
		count = 1;
	if (count > GENET_INTR_THRESHOLD_MASK)
		count = GENET_INTR_THRESHOLD_MASK;

	WR4(sc, GENET_TX_DMA_MBUF_DONE_THRES(qid), count);
}

static void
genet_init_rings(struct genet_softc *sc, int qid)
{
	uint32_t val;

	/* TX ring */

	sc->sc_tx.queued = 0;
	sc->sc_tx.cidx = sc->sc_tx.pidx = 0;

	WR4(sc, GENET_TX_SCB_BURST_SIZE, 0x08);

	WR4(sc, GENET_TX_DMA_READ_PTR_LO(qid), 0);
	WR4(sc, GENET_TX_DMA_READ_PTR_HI(qid), 0);
	WR4(sc, GENET_TX_DMA_CONS_INDEX(qid), 0);
	WR4(sc, GENET_TX_DMA_PROD_INDEX(qid), 0);
	WR4(sc, GENET_TX_DMA_RING_BUF_SIZE(qid),
	    __SHIFTIN(TX_DESC_COUNT, GENET_TX_DMA_RING_BUF_SIZE_DESC_COUNT) |
	    __SHIFTIN(MCLBYTES, GENET_TX_DMA_RING_BUF_SIZE_BUF_LENGTH));
	WR4(sc, GENET_TX_DMA_START_ADDR_LO(qid), 0);
	WR4(sc, GENET_TX_DMA_START_ADDR_HI(qid), 0);
	WR4(sc, GENET_TX_DMA_END_ADDR_LO(qid),
	    TX_DESC_COUNT * GENET_DMA_DESC_SIZE / 4 - 1);
	WR4(sc, GENET_TX_DMA_END_ADDR_HI(qid), 0);
	WR4(sc, GENET_TX_DMA_FLOW_PERIOD(qid), 0);
	WR4(sc, GENET_TX_DMA_WRITE_PTR_LO(qid), 0);
	WR4(sc, GENET_TX_DMA_WRITE_PTR_HI(qid), 0);

	/* interrupt after 10 packets or when ring empty */
	genet_set_txthresh(sc, qid, 10);

	WR4(sc, GENET_TX_DMA_RING_CFG, __BIT(qid));	/* enable */

	/* Enable transmit DMA */
	val = RD4(sc, GENET_TX_DMA_CTRL);
	val |= GENET_TX_DMA_CTRL_EN;
	val |= GENET_TX_DMA_CTRL_RBUF_EN(GENET_DMA_DEFAULT_QUEUE);
	WR4(sc, GENET_TX_DMA_CTRL, val);

	/* RX ring */

	sc->sc_rx.cidx = sc->sc_rx.pidx = 0;

	WR4(sc, GENET_RX_SCB_BURST_SIZE, 0x08);

	WR4(sc, GENET_RX_DMA_WRITE_PTR_LO(qid), 0);
	WR4(sc, GENET_RX_DMA_WRITE_PTR_HI(qid), 0);
	WR4(sc, GENET_RX_DMA_PROD_INDEX(qid), 0);
	WR4(sc, GENET_RX_DMA_CONS_INDEX(qid), 0);
	WR4(sc, GENET_RX_DMA_RING_BUF_SIZE(qid),
	    __SHIFTIN(RX_DESC_COUNT, GENET_RX_DMA_RING_BUF_SIZE_DESC_COUNT) |
	    __SHIFTIN(MCLBYTES, GENET_RX_DMA_RING_BUF_SIZE_BUF_LENGTH));
	WR4(sc, GENET_RX_DMA_START_ADDR_LO(qid), 0);
	WR4(sc, GENET_RX_DMA_START_ADDR_HI(qid), 0);
	WR4(sc, GENET_RX_DMA_END_ADDR_LO(qid),
	    RX_DESC_COUNT * GENET_DMA_DESC_SIZE / 4 - 1);
	WR4(sc, GENET_RX_DMA_END_ADDR_HI(qid), 0);
	WR4(sc, GENET_RX_DMA_XON_XOFF_THRES(qid),
	    __SHIFTIN(5, GENET_RX_DMA_XON_XOFF_THRES_LO) |
	    __SHIFTIN(RX_DESC_COUNT >> 4, GENET_RX_DMA_XON_XOFF_THRES_HI));
	WR4(sc, GENET_RX_DMA_READ_PTR_LO(qid), 0);
	WR4(sc, GENET_RX_DMA_READ_PTR_HI(qid), 0);

	/*
	 * interrupt on first packet,
	 * mitigation timeout timeout 57 us (~84 minimal packets at 1Gbit/s)
	 */
	genet_set_rxthresh(sc, qid, 57, 10);

	WR4(sc, GENET_RX_DMA_RING_CFG, __BIT(qid));	/* enable */

	/* Enable receive DMA */
	val = RD4(sc, GENET_RX_DMA_CTRL);
	val |= GENET_RX_DMA_CTRL_EN;
	val |= GENET_RX_DMA_CTRL_RBUF_EN(GENET_DMA_DEFAULT_QUEUE);
	WR4(sc, GENET_RX_DMA_CTRL, val);
}

static int
genet_init_locked(struct genet_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct mii_data *mii = &sc->sc_mii;
	uint32_t val;
	const uint8_t *enaddr = CLLADDR(ifp->if_sadl);

	GENET_ASSERT_LOCKED(sc);
	GENET_ASSERT_TXLOCKED(sc);

	if ((ifp->if_flags & IFF_RUNNING) != 0)
		return 0;

	if (sc->sc_phy_mode == GENET_PHY_MODE_RGMII ||
	    sc->sc_phy_mode == GENET_PHY_MODE_RGMII_ID ||
	    sc->sc_phy_mode == GENET_PHY_MODE_RGMII_RXID ||
	    sc->sc_phy_mode == GENET_PHY_MODE_RGMII_TXID)
		WR4(sc, GENET_SYS_PORT_CTRL,
		    GENET_SYS_PORT_MODE_EXT_GPHY);
	else
		WR4(sc, GENET_SYS_PORT_CTRL, 0);

	/* Write hardware address */
	val = enaddr[3] | (enaddr[2] << 8) | (enaddr[1] << 16) |
	    (enaddr[0] << 24);
	WR4(sc, GENET_UMAC_MAC0, val);
	val = enaddr[5] | (enaddr[4] << 8);
	WR4(sc, GENET_UMAC_MAC1, val);

	/* Setup RX filter */
	genet_setup_rxfilter(sc);

	/* Setup TX/RX rings */
	genet_init_rings(sc, GENET_DMA_DEFAULT_QUEUE);

	/* Enable transmitter and receiver */
	val = RD4(sc, GENET_UMAC_CMD);
	val |= GENET_UMAC_CMD_TXEN;
	val |= GENET_UMAC_CMD_RXEN;
	WR4(sc, GENET_UMAC_CMD, val);

	/* Enable interrupts */
	genet_enable_intr(sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	mii_mediachg(mii);
	callout_schedule(&sc->sc_stat_ch, hz);

	return 0;
}

static int
genet_init(struct ifnet *ifp)
{
	struct genet_softc *sc = ifp->if_softc;
	int error;

	GENET_LOCK(sc);
	GENET_TXLOCK(sc);
	error = genet_init_locked(sc);
	GENET_TXUNLOCK(sc);
	GENET_UNLOCK(sc);

	return error;
}

static void
genet_stop_locked(struct genet_softc *sc, int disable)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint32_t val;

	GENET_ASSERT_LOCKED(sc);

	callout_stop(&sc->sc_stat_ch);

	mii_down(&sc->sc_mii);

	/* Disable receiver */
	val = RD4(sc, GENET_UMAC_CMD);
	val &= ~GENET_UMAC_CMD_RXEN;
	WR4(sc, GENET_UMAC_CMD, val);

	/* Stop receive DMA */
	val = RD4(sc, GENET_RX_DMA_CTRL);
	val &= ~GENET_RX_DMA_CTRL_EN;
	WR4(sc, GENET_RX_DMA_CTRL, val);

	/* Stop transmit DMA */
	val = RD4(sc, GENET_TX_DMA_CTRL);
	val &= ~GENET_TX_DMA_CTRL_EN;
	WR4(sc, GENET_TX_DMA_CTRL, val);

	/* Flush data in the TX FIFO */
	WR4(sc, GENET_UMAC_TX_FLUSH, 1);
	delay(10);
	WR4(sc, GENET_UMAC_TX_FLUSH, 0);

	/* Disable transmitter */
	val = RD4(sc, GENET_UMAC_CMD);
	val &= ~GENET_UMAC_CMD_TXEN;
	WR4(sc, GENET_UMAC_CMD, val);

	/* Disable interrupts */
	genet_disable_intr(sc);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static void
genet_stop(struct ifnet *ifp, int disable)
{
	struct genet_softc * const sc = ifp->if_softc;

	GENET_LOCK(sc);
	genet_stop_locked(sc, disable);
	GENET_UNLOCK(sc);
}

static void
genet_rxintr(struct genet_softc *sc, int qid)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int error, index, len, n;
	struct mbuf *m, *m0;
	uint32_t status, pidx, total;
	int pkts = 0;

	pidx = RD4(sc, GENET_RX_DMA_PROD_INDEX(qid)) & 0xffff;
	total = (pidx - sc->sc_rx.cidx) & 0xffff;

	DPRINTF("RX pidx=%08x total=%d\n", pidx, total);

	index = sc->sc_rx.cidx % RX_DESC_COUNT;
	for (n = 0; n < total; n++) {
		status = RD4(sc, GENET_RX_DESC_STATUS(index));

		if (status & GENET_RX_DESC_STATUS_ALL_ERRS) {
			if (status & GENET_RX_DESC_STATUS_OVRUN_ERR)
				device_printf(sc->sc_dev, "overrun\n");
			if (status & GENET_RX_DESC_STATUS_CRC_ERR)
				device_printf(sc->sc_dev, "CRC error\n");
			if (status & GENET_RX_DESC_STATUS_RX_ERR)
				device_printf(sc->sc_dev, "receive error\n");
			if (status & GENET_RX_DESC_STATUS_FRAME_ERR)
				device_printf(sc->sc_dev, "frame error\n");
			if (status & GENET_RX_DESC_STATUS_LEN_ERR)
				device_printf(sc->sc_dev, "length error\n");
			if_statinc(ifp, if_ierrors);
			goto next;
		}

		if (status & GENET_RX_DESC_STATUS_OWN)
			device_printf(sc->sc_dev, "OWN %d of %d\n",n,total);

		len = __SHIFTOUT(status, GENET_RX_DESC_STATUS_BUFLEN);
		if (len < ETHER_ALIGN) {
			if_statinc(ifp, if_ierrors);
			goto next;
		}

		m = sc->sc_rx.buf_map[index].mbuf;

		if ((m0 = genet_alloc_mbufcl(sc)) == NULL) {
			if_statinc(ifp, if_ierrors);
			goto next;
		}

		/* unload map before it gets loaded in setup_rxbuf */
		if (sc->sc_rx.buf_map[index].map->dm_mapsize > 0) {
			bus_dmamap_sync(sc->sc_rx.buf_tag, sc->sc_rx.buf_map[index].map,
			    0, sc->sc_rx.buf_map[index].map->dm_mapsize,
			    BUS_DMASYNC_POSTREAD);
		}
		bus_dmamap_unload(sc->sc_rx.buf_tag, sc->sc_rx.buf_map[index].map);
		sc->sc_rx.buf_map[index].mbuf = NULL;

		error = genet_setup_rxbuf(sc, index, m0);
		if (error != 0) {
			m_freem(m0);
			if_statinc(ifp, if_ierrors);

			/* XXX mbuf is unloaded but load failed */
			m_freem(m);
			device_printf(sc->sc_dev,
			    "cannot load RX mbuf. panic?\n");
			goto next;
		}

		DPRINTF("RX [#%d] index=%02x status=%08x len=%d adj_len=%d\n",
		    n, index, status, len, len - ETHER_ALIGN);

		m_set_rcvif(m, ifp);
		m->m_len = m->m_pkthdr.len = len;
		m_adj(m, ETHER_ALIGN);

		if_percpuq_enqueue(ifp->if_percpuq, m);
		++pkts;

next:
		index = RX_NEXT(index);

		sc->sc_rx.cidx = (sc->sc_rx.cidx + 1) & 0xffff;
		WR4(sc, GENET_RX_DMA_CONS_INDEX(qid), sc->sc_rx.cidx);
	}

	if (pkts != 0)
		rnd_add_uint32(&sc->sc_rndsource, pkts);
}

static void
genet_txintr(struct genet_softc *sc, int qid)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct genet_bufmap *bmap;
	int cidx, i, pkts = 0;

	cidx = RD4(sc, GENET_TX_DMA_CONS_INDEX(qid)) & 0xffff;
	i = sc->sc_tx.cidx % TX_DESC_COUNT;
	while (sc->sc_tx.cidx != cidx) {
		bmap = &sc->sc_tx.buf_map[i];
		if (bmap->mbuf != NULL) {
			/* XXX first segment already unloads */
			if (bmap->map->dm_mapsize > 0) {
				bus_dmamap_sync(sc->sc_tx.buf_tag, bmap->map,
				    0, bmap->map->dm_mapsize,
				    BUS_DMASYNC_POSTWRITE);
			}
			bus_dmamap_unload(sc->sc_tx.buf_tag, bmap->map);
			m_freem(bmap->mbuf);
			bmap->mbuf = NULL;
			++pkts;
		}

		ifp->if_flags &= ~IFF_OACTIVE;
		i = TX_NEXT(i);
		sc->sc_tx.cidx = (sc->sc_tx.cidx + 1) & 0xffff;
	}

	if_statadd(ifp, if_opackets, pkts);

	if (pkts != 0)
		rnd_add_uint32(&sc->sc_rndsource, pkts);
}

static void
genet_start_locked(struct genet_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct mbuf *m;
	int nsegs, index, cnt;

	GENET_ASSERT_TXLOCKED(sc);

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	const int qid = GENET_DMA_DEFAULT_QUEUE;

	index = sc->sc_tx.pidx % TX_DESC_COUNT;
	cnt = 0;

	sc->sc_tx.queued = (RD4(sc, GENET_TX_DMA_PROD_INDEX(qid))
	          - RD4(sc, GENET_TX_DMA_CONS_INDEX(qid))) & 0xffff;

	for (;;) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		nsegs = genet_setup_txbuf(sc, index, m);
		if (nsegs <= 0) {
			if (nsegs == -1) {
				ifp->if_flags |= IFF_OACTIVE;
			}
			else if (nsegs == -2) {
				IFQ_DEQUEUE(&ifp->if_snd, m);
				m_freem(m);
			}
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);
		bpf_mtap(ifp, m, BPF_D_OUT);

		index = TX_SKIP(index, nsegs);
		sc->sc_tx.queued += nsegs;
		sc->sc_tx.pidx = (sc->sc_tx.pidx + nsegs) & 0xffff;
		cnt++;
	}

	if (cnt != 0)
		WR4(sc, GENET_TX_DMA_PROD_INDEX(qid), sc->sc_tx.pidx);
}

static void
genet_start(struct ifnet *ifp)
{
	struct genet_softc *sc = ifp->if_softc;

	GENET_TXLOCK(sc);
	genet_start_locked(sc);
	GENET_TXUNLOCK(sc);
}

int
genet_intr(void *arg)
{
	struct genet_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint32_t val;
	bool dotx = false;

	val = RD4(sc, GENET_INTRL2_CPU_STAT);
	val &= ~RD4(sc, GENET_INTRL2_CPU_STAT_MASK);
	WR4(sc, GENET_INTRL2_CPU_CLEAR, val);

	if (val & GENET_IRQ_RXDMA_DONE) {
		GENET_LOCK(sc);
		genet_rxintr(sc, GENET_DMA_DEFAULT_QUEUE);
		GENET_UNLOCK(sc);
	}

	if (val & GENET_IRQ_TXDMA_DONE) {
		genet_txintr(sc, GENET_DMA_DEFAULT_QUEUE);
		dotx = true;
	}

	if (dotx)
		if_schedule_deferred_start(ifp);

	return 1;
}

static int
genet_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct genet_softc *sc = ifp->if_softc;
	int error, s;

#ifndef GENET_MPSAFE
	s = splnet();
#endif

	switch (cmd) {
	default:
#ifdef GENET_MPSAFE
		s = splnet();
#endif
		error = ether_ioctl(ifp, cmd, data);
#ifdef GENET_MPSAFE
		splx(s);
#endif
		if (error != ENETRESET)
			break;

		error = 0;

		if (cmd == SIOCSIFCAP)
			error = if_init(ifp);
		else if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			;
		else if ((ifp->if_flags & IFF_RUNNING) != 0) {
			GENET_LOCK(sc);
			genet_setup_rxfilter(sc);
			GENET_UNLOCK(sc);
		}
		break;
	}

#ifndef GENET_MPSAFE
	splx(s);
#endif

	return error;
}

static void
genet_get_eaddr(struct genet_softc *sc, uint8_t *eaddr)
{
	prop_dictionary_t prop = device_properties(sc->sc_dev);
	uint32_t maclo, machi, val;
	prop_data_t eaprop;

	eaprop = prop_dictionary_get(prop, "mac-address");
	if (eaprop != NULL) {
		KASSERT(prop_object_type(eaprop) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(eaprop) == ETHER_ADDR_LEN);
		memcpy(eaddr, prop_data_value(eaprop),
		    ETHER_ADDR_LEN);
		return;
	}

	maclo = machi = 0;

	val = RD4(sc, GENET_SYS_RBUF_FLUSH_CTRL);
	if ((val & GENET_SYS_RBUF_FLUSH_RESET) == 0) {
		maclo = htobe32(RD4(sc, GENET_UMAC_MAC0));
		machi = htobe16(RD4(sc, GENET_UMAC_MAC1) & 0xffff);
	}

	if (maclo == 0 && machi == 0) {
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

static int
genet_setup_dma(struct genet_softc *sc, int qid)
{
	struct mbuf *m;
	int error, i;

	/* Setup TX ring */
	sc->sc_tx.buf_tag = sc->sc_dmat;
	for (i = 0; i < TX_DESC_COUNT; i++) {
		error = bus_dmamap_create(sc->sc_tx.buf_tag, MCLBYTES,
		    TX_MAX_SEGS, MCLBYTES, 0, BUS_DMA_WAITOK,
		    &sc->sc_tx.buf_map[i].map);
		if (error != 0) {
			device_printf(sc->sc_dev,
			    "cannot create TX buffer map\n");
			return error;
		}
	}

	/* Setup RX ring */
	sc->sc_rx.buf_tag = sc->sc_dmat;
	for (i = 0; i < RX_DESC_COUNT; i++) {
		error = bus_dmamap_create(sc->sc_rx.buf_tag, MCLBYTES,
		    1, MCLBYTES, 0, BUS_DMA_WAITOK,
		    &sc->sc_rx.buf_map[i].map);
		if (error != 0) {
			device_printf(sc->sc_dev,
			    "cannot create RX buffer map\n");
			return error;
		}
		if ((m = genet_alloc_mbufcl(sc)) == NULL) {
			device_printf(sc->sc_dev, "cannot allocate RX mbuf\n");
			return ENOMEM;
		}
		error = genet_setup_rxbuf(sc, i, m);
		if (error != 0) {
			device_printf(sc->sc_dev, "cannot create RX buffer\n");
			return error;
		}
	}

	return 0;
}

int
genet_attach(struct genet_softc *sc)
{
	struct mii_data *mii = &sc->sc_mii;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint8_t eaddr[ETHER_ADDR_LEN];
	u_int maj, min;
	int mii_flags = 0;

	const uint32_t rev = RD4(sc, GENET_SYS_REV_CTRL);
	min = __SHIFTOUT(rev, SYS_REV_MINOR);
	maj = __SHIFTOUT(rev, SYS_REV_MAJOR);
	if (maj == 0)
		maj++;
	else if (maj == 5 || maj == 6)
		maj--;

	if (maj != 5) {
		aprint_error(": GENETv%d.%d not supported\n", maj, min);
		return ENXIO;
	}

	switch (sc->sc_phy_mode) {
	case GENET_PHY_MODE_RGMII_TXID:
		mii_flags |= MIIF_TXID;
		break;
	case GENET_PHY_MODE_RGMII_RXID:
		mii_flags |= MIIF_RXID;
		break;
	case GENET_PHY_MODE_RGMII_ID:
		mii_flags |= MIIF_RXID | MIIF_TXID;
		break;
	case GENET_PHY_MODE_RGMII:
	default:
		break;
	}

	aprint_naive("\n");
	aprint_normal(": GENETv%d.%d\n", maj, min);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_txlock, MUTEX_DEFAULT, IPL_NET);
	callout_init(&sc->sc_stat_ch, CALLOUT_FLAGS);
	callout_setfunc(&sc->sc_stat_ch, genet_tick, sc);

	genet_get_eaddr(sc, eaddr);
	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n", ether_sprintf(eaddr));

	/* Soft reset EMAC core */
	genet_reset(sc);

	/* Setup DMA descriptors */
	if (genet_setup_dma(sc, GENET_DMA_DEFAULT_QUEUE) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to setup DMA descriptors\n");
		return EINVAL;
	}

	/* Setup ethernet interface */
	ifp->if_softc = sc;
	snprintf(ifp->if_xname, IFNAMSIZ, "%s", device_xname(sc->sc_dev));
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef GENET_MPSAFE
	ifp->if_extflags = IFEF_MPSAFE;
#endif
	ifp->if_start = genet_start;
	ifp->if_ioctl = genet_ioctl;
	ifp->if_init = genet_init;
	ifp->if_stop = genet_stop;
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
	mii->mii_readreg = genet_mii_readreg;
	mii->mii_writereg = genet_mii_writereg;
	mii->mii_statchg = genet_mii_statchg;
	mii_attach(sc->sc_dev, mii, 0xffffffff, sc->sc_phy_id, MII_OFFSET_ANY,
	    mii_flags);

	if (LIST_EMPTY(&mii->mii_phys)) {
		aprint_error_dev(sc->sc_dev, "no PHY found!\n");
		return ENOENT;
	}
	ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);

	/* Attach interface */
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);

	/* Attach ethernet interface */
	ether_ifattach(ifp, eaddr);

	rnd_attach_source(&sc->sc_rndsource, ifp->if_xname, RND_TYPE_NET,
	    RND_FLAG_DEFAULT);

	return 0;
}

#ifdef DDB
void	genet_debug(void);

void
genet_debug(void)
{
	device_t dev = device_find_by_xname("genet0");
	if (dev == NULL)
		return;

	struct genet_softc * const sc = device_private(dev);
	const int qid = GENET_DMA_DEFAULT_QUEUE;

	printf("TX CIDX = %08x (soft)\n", sc->sc_tx.cidx);
	printf("TX CIDX = %08x\n", RD4(sc, GENET_TX_DMA_CONS_INDEX(qid)));
	printf("TX PIDX = %08x (soft)\n", sc->sc_tx.pidx);
	printf("TX PIDX = %08x\n", RD4(sc, GENET_TX_DMA_PROD_INDEX(qid)));

	printf("RX CIDX = %08x (soft)\n", sc->sc_rx.cidx);
	printf("RX CIDX = %08x\n", RD4(sc, GENET_RX_DMA_CONS_INDEX(qid)));
	printf("RX PIDX = %08x (soft)\n", sc->sc_rx.pidx);
	printf("RX PIDX = %08x\n", RD4(sc, GENET_RX_DMA_PROD_INDEX(qid)));
}
#endif
