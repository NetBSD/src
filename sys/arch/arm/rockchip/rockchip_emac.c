/* $NetBSD: rockchip_emac.c,v 1.17.12.2 2017/12/03 11:35:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_rkemac.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rockchip_emac.c,v 1.17.12.2 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/cprng.h>

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_var.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#include <dev/mii/miivar.h>

#include <arm/rockchip/rockchip_emacreg.h>

#define RKEMAC_SOC_CON1_EMAC_SPEED_MASK	__BIT(17)
#define RKEMAC_SOC_CON1_EMAC_SPEED	__BIT(2)

#define RKEMAC_ENABLE_INTR	\
	(EMAC_ENABLE_TXINT|EMAC_ENABLE_RXINT|EMAC_ENABLE_ERR)

#define RKEMAC_RX_RING_COUNT	128
#define RKEMAC_TX_RING_COUNT	128

#define RKEMAC_MAX_PACKET	1536
#define RKEMAC_POLLRATE		200

#define RKEMAC_CLKRATE		50000000

#define RX_DESC_OFFSET(n)	\
	((n) * sizeof(struct rkemac_rxdesc))
#define RX_NEXT(n)		(((n) + 1) & (RKEMAC_RX_RING_COUNT - 1))
	
#define TX_DESC_OFFSET(n)	\
	((RKEMAC_RX_RING_COUNT+(n)) * sizeof(struct rkemac_txdesc))
#define TX_NEXT(n)		(((n) + 1) & (RKEMAC_TX_RING_COUNT - 1))

struct rkemac_rxdata {
	bus_dmamap_t rd_map;
	struct mbuf *rd_m;
};

struct rkemac_txdata {
	bus_dmamap_t td_map;
	bus_dmamap_t td_active;
	struct mbuf *td_m;
	int td_nbufs;
};

struct rkemac_txring {
	bus_addr_t t_physaddr;
	struct rkemac_txdesc *t_desc;
	struct rkemac_txdata t_data[RKEMAC_TX_RING_COUNT];
	int t_cur, t_next, t_queued;
};

struct rkemac_rxring {
	bus_addr_t r_physaddr;
	struct rkemac_rxdesc *r_desc;
	struct rkemac_rxdata r_data[RKEMAC_RX_RING_COUNT];
	int r_cur, r_next;
};

struct rkemac_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_soc_con1_bsh;
	bus_dma_tag_t sc_dmat;
	void *sc_ih;

	bus_size_t sc_soc_con1_reg;

	struct ethercom sc_ec;
	struct mii_data sc_mii;
	callout_t sc_mii_tick;
	kmutex_t sc_lock;

	bus_dmamap_t sc_ring_dmamap;
	bus_dma_segment_t sc_ring_dmaseg;
	struct rkemac_txring sc_txq;
	struct rkemac_rxring sc_rxq;
	bus_addr_t sc_pad_physaddr;
};

static int	rkemac_match(device_t, cfdata_t, void *);
static void	rkemac_attach(device_t, device_t, void *);

static int	rkemac_dma_init(struct rkemac_softc *);

static int	rkemac_intr(void *);

static int	rkemac_mii_readreg(device_t, int, int);
static void	rkemac_mii_writereg(device_t, int, int, int);
static void	rkemac_mii_statchg(struct ifnet *);

static int	rkemac_init(struct ifnet *);
static void	rkemac_start(struct ifnet *);
static void	rkemac_stop(struct ifnet *, int);
static int	rkemac_ioctl(struct ifnet *, u_long, void *);
static void	rkemac_watchdog(struct ifnet *);
static void	rkemac_tick(void *);

static int	rkemac_queue(struct rkemac_softc *, struct mbuf *);
static void	rkemac_txdesc_sync(struct rkemac_softc *, int, int, int);
static void	rkemac_txintr(struct rkemac_softc *);
static void	rkemac_rxintr(struct rkemac_softc *);
static void	rkemac_setmulti(struct rkemac_softc *);

CFATTACH_DECL_NEW(rkemac, sizeof(struct rkemac_softc),
	rkemac_match, rkemac_attach, NULL, NULL);

#define EMAC_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define EMAC_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
rkemac_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
rkemac_attach(device_t parent, device_t self, void *aux)
{
	struct rkemac_softc *sc = device_private(self);
	struct obio_attach_args * const obio = aux;
	prop_dictionary_t cfg = device_properties(self);
	struct mii_data *mii = &sc->sc_mii;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint8_t enaddr[ETHER_ADDR_LEN];
	bus_size_t soc_con1_reg;
	prop_data_t ea;

	sc->sc_dev = self;
	sc->sc_bst = obio->obio_bst;
	sc->sc_dmat = obio->obio_dmat;
	bus_space_subregion(obio->obio_bst, obio->obio_bsh, obio->obio_offset,
	    obio->obio_size, &sc->sc_bsh);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NET);

	callout_init(&sc->sc_mii_tick, 0);
	callout_setfunc(&sc->sc_mii_tick, rkemac_tick, sc);

	if (rockchip_chip_id() == ROCKCHIP_CHIP_ID_RK3188 ||
	    rockchip_chip_id() == ROCKCHIP_CHIP_ID_RK3188PLUS) {
		soc_con1_reg = 0x00a4;
	} else {
		soc_con1_reg = 0x0154;
	}
	bus_space_subregion(obio->obio_bst, obio->obio_grf_bsh, soc_con1_reg,
	    4, &sc->sc_soc_con1_bsh);

	aprint_naive("\n");
	aprint_normal(": Ethernet controller\n");

	rockchip_mac_set_rate(RKEMAC_CLKRATE);

#ifdef RKEMAC_DEBUG
	aprint_normal_dev(sc->sc_dev, "ID %#x\n",
	    EMAC_READ(sc, EMAC_ID_REG));
#endif

	sc->sc_ih = intr_establish(obio->obio_intr, IPL_NET,
	    IST_LEVEL, rkemac_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}

	if ((ea = prop_dictionary_get(cfg, "mac-address")) != NULL) {
		KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
		memcpy(enaddr, prop_data_data_nocopy(ea), ETHER_ADDR_LEN);
	} else {
		uint32_t addrl, addrh;

		addrl = EMAC_READ(sc, EMAC_ADDRL_REG);
		addrh = EMAC_READ(sc, EMAC_ADDRH_REG);

		if (addrl == 0 && addrh == 0) {
			/* fake MAC address */
			addrl = 0x00f2 | (cprng_strong32() << 16);
			addrh = cprng_strong32();
		}

		enaddr[0] = addrl & 0xff;
		enaddr[1] = (addrl >> 8) & 0xff;
		enaddr[2] = (addrl >> 16) & 0xff;
		enaddr[3] = (addrl >> 24) & 0xff;
		enaddr[4] = addrh & 0xff;
		enaddr[5] = (addrh >> 8) & 0xff;
	}

	const uint32_t addrl = enaddr[0] | (enaddr[1] << 8) |
	    (enaddr[2] << 16) | (enaddr[3] << 24);
	const uint32_t addrh = enaddr[4] | (enaddr[5] << 8);
	EMAC_WRITE(sc, EMAC_ADDRL_REG, addrl);
	EMAC_WRITE(sc, EMAC_ADDRH_REG, addrh);

	aprint_normal_dev(sc->sc_dev, "Ethernet address: %s\n",
	    ether_sprintf(enaddr));

	EMAC_WRITE(sc, EMAC_CONTROL_REG, 0);
	EMAC_WRITE(sc, EMAC_ENABLE_REG, 0);
	EMAC_WRITE(sc, EMAC_STAT_REG, EMAC_READ(sc, EMAC_STAT_REG));

	rkemac_dma_init(sc);

	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = rkemac_ioctl;
	ifp->if_start = rkemac_start;
	ifp->if_init = rkemac_init;
	ifp->if_stop = rkemac_stop;
	ifp->if_watchdog = rkemac_watchdog;
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
	mii->mii_ifp = ifp;
	mii->mii_readreg = rkemac_mii_readreg;
	mii->mii_writereg = rkemac_mii_writereg;
	mii->mii_statchg = rkemac_mii_statchg;
	mii_attach(sc->sc_dev, mii, 0xffffffff, 0/* XXX */, MII_OFFSET_ANY, 0);

	if (LIST_EMPTY(&mii->mii_phys)) {
		aprint_error_dev(sc->sc_dev, "no PHY found!\n");
		return;
	}

	ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, enaddr);

	EMAC_WRITE(sc, EMAC_ENABLE_REG, RKEMAC_ENABLE_INTR);
}

static int
rkemac_dma_init(struct rkemac_softc *sc)
{
	size_t descsize = RKEMAC_RX_RING_COUNT * sizeof(struct rkemac_rxdesc) +
			  RKEMAC_TX_RING_COUNT * sizeof(struct rkemac_txdesc) +
			  ETHER_MIN_LEN;
	bus_addr_t physaddr;
	int error, nsegs;
	void *descs;

	/*
	 * Allocate TX / RX descriptors
	 */
	error = bus_dmamap_create(sc->sc_dmat, descsize, 1, descsize, 0,
	    BUS_DMA_NOWAIT, &sc->sc_ring_dmamap);
	if (error)
		return error;
	error = bus_dmamem_alloc(sc->sc_dmat, descsize, 8, 0,
	    &sc->sc_ring_dmaseg, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_ring_dmaseg, nsegs,
	    descsize, &descs, BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error)
		return error;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_ring_dmamap, descs,
	    descsize, NULL, BUS_DMA_NOWAIT);
	if (error)
		return error;

	memset(descs, 0, descsize);

	sc->sc_rxq.r_desc = descs;
	sc->sc_rxq.r_physaddr = sc->sc_ring_dmamap->dm_segs[0].ds_addr;

	sc->sc_txq.t_desc =
	     (struct rkemac_txdesc *)(sc->sc_rxq.r_desc + RKEMAC_RX_RING_COUNT);
	sc->sc_txq.t_physaddr = sc->sc_rxq.r_physaddr +
	    RKEMAC_RX_RING_COUNT * sizeof(struct rkemac_rxdesc);

	sc->sc_pad_physaddr = sc->sc_txq.t_physaddr +
	    RKEMAC_TX_RING_COUNT * sizeof(struct rkemac_txdesc);

	/*
	 * Setup RX ring
	 */
	sc->sc_rxq.r_cur = sc->sc_rxq.r_next = 0;
	for (int i = 0; i < RKEMAC_RX_RING_COUNT; i++) {
		struct rkemac_rxdata *rd = &sc->sc_rxq.r_data[i];
		struct rkemac_rxdesc *rx = &sc->sc_rxq.r_desc[i];

		MGETHDR(rd->rd_m, M_DONTWAIT, MT_DATA);
		if (rd->rd_m == NULL)
			return ENOMEM;
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_NOWAIT, &rd->rd_map);
		if (error)
			return error;
		MCLGET(rd->rd_m, M_DONTWAIT);
		if ((rd->rd_m->m_flags & M_EXT) == 0)
			return ENOMEM;
		error = bus_dmamap_load(sc->sc_dmat, rd->rd_map,
		    mtod(rd->rd_m, void *), MCLBYTES, NULL,
		    BUS_DMA_READ | BUS_DMA_NOWAIT);
		if (error)
			return error;
		physaddr = rd->rd_map->dm_segs[0].ds_addr;

		rx->rx_ptr = htole32(physaddr);
		rx->rx_info = htole32(EMAC_RXDESC_OWN |
		    __SHIFTIN(RKEMAC_MAX_PACKET, EMAC_RXDESC_RXLEN));
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
	    0, RKEMAC_RX_RING_COUNT * sizeof(struct rkemac_rxdesc),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	/*
	 * Setup TX ring
	 */ 
	sc->sc_txq.t_queued = sc->sc_txq.t_cur = sc->sc_txq.t_next = 0;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
	    RKEMAC_RX_RING_COUNT,
	    RKEMAC_TX_RING_COUNT * sizeof(struct rkemac_txdesc),
	    BUS_DMASYNC_POSTWRITE);
	for (int i = 0; i < RKEMAC_TX_RING_COUNT; i++) {
		struct rkemac_txdata *td = &sc->sc_txq.t_data[i];
		struct rkemac_txdesc *tx = &sc->sc_txq.t_desc[i];
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    RKEMAC_TX_RING_COUNT, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &td->td_map);
		if (error)
			return error;
		tx->tx_ptr = htole32(sc->sc_txq.t_physaddr +
		    RKEMAC_RX_RING_COUNT +
		    sizeof(struct rkemac_txdesc) * i);
	}

	return 0;
}

static int
rkemac_intr(void *priv)
{
	struct rkemac_softc *sc = priv;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint32_t stat;

	stat = EMAC_READ(sc, EMAC_STAT_REG);
	if (!stat)
		return 0;

	EMAC_WRITE(sc, EMAC_STAT_REG, stat & ~EMAC_STAT_MDIO);

	if (stat & EMAC_STAT_TXINT)
		rkemac_txintr(sc);
	if (stat & EMAC_STAT_RXINT)
		rkemac_rxintr(sc);
	if (stat & EMAC_STAT_ERR)
		ifp->if_oerrors++;

	if (stat & (EMAC_STAT_TXINT|EMAC_STAT_RXINT))
		if_schedule_deferred_start(ifp);

	return 1;
}

static int
rkemac_mii_readreg(device_t dev, int phy, int reg)
{
	struct rkemac_softc *sc = device_private(dev);
	uint32_t mdio;
	int retry = 1000;

	mdio = __SHIFTIN(EMAC_MDIO_DATA_SFD_SFD, EMAC_MDIO_DATA_SFD);
	mdio |= __SHIFTIN(EMAC_MDIO_DATA_OP_READ, EMAC_MDIO_DATA_OP);
	mdio |= __SHIFTIN(phy, EMAC_MDIO_DATA_PHYADDR);
	mdio |= __SHIFTIN(reg, EMAC_MDIO_DATA_PHYREG);
	mdio |= __SHIFTIN(EMAC_MDIO_DATA_TA_TA, EMAC_MDIO_DATA_TA);

	mutex_enter(&sc->sc_lock);
	EMAC_WRITE(sc, EMAC_STAT_REG, EMAC_STAT_MDIO);
	EMAC_WRITE(sc, EMAC_MDIO_DATA_REG, mdio);
	while (--retry > 0) {
		if (EMAC_READ(sc, EMAC_STAT_REG) & EMAC_STAT_MDIO) {
			mdio = EMAC_READ(sc, EMAC_MDIO_DATA_REG);
			break;
		}
		delay(10);
	}
	mutex_exit(&sc->sc_lock);

#ifdef RKEMAC_DEBUG
	printf("%s: phy %d reg %#x data %#x\n", __func__, phy, reg, mdio);
	if (retry == 0)
		printf("%s: timed out\n", __func__);
#endif

	return __SHIFTOUT(mdio, EMAC_MDIO_DATA_DATA);
}

static void
rkemac_mii_writereg(device_t dev, int phy, int reg, int val)
{
	struct rkemac_softc *sc = device_private(dev);
	uint32_t mdio;
	int retry = 1000;

	mdio = __SHIFTIN(EMAC_MDIO_DATA_SFD_SFD, EMAC_MDIO_DATA_SFD);
	mdio |= __SHIFTIN(EMAC_MDIO_DATA_OP_WRITE, EMAC_MDIO_DATA_OP);
	mdio |= __SHIFTIN(phy, EMAC_MDIO_DATA_PHYADDR);
	mdio |= __SHIFTIN(reg, EMAC_MDIO_DATA_PHYREG);
	mdio |= __SHIFTIN(EMAC_MDIO_DATA_TA_TA, EMAC_MDIO_DATA_TA);
	mdio |= __SHIFTIN(val, EMAC_MDIO_DATA_DATA);

#ifdef RKEMAC_DEBUG
	printf("%s: phy %d reg %#x data %#x\n", __func__, phy, reg, mdio);
#endif

	mutex_enter(&sc->sc_lock);
	EMAC_WRITE(sc, EMAC_STAT_REG, EMAC_STAT_MDIO);
	EMAC_WRITE(sc, EMAC_MDIO_DATA_REG, mdio);
	while (--retry > 0) {
		if (EMAC_READ(sc, EMAC_STAT_REG) & EMAC_STAT_MDIO) {
			break;
		}
		delay(10);
	}
	mutex_exit(&sc->sc_lock);

#ifdef RKEMAC_DEBUG
	if (retry == 0)
		printf("%s: timed out\n", __func__);
#endif
}

static void
rkemac_mii_statchg(struct ifnet *ifp)
{
	struct rkemac_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;
	uint32_t control, soc_con1;

	control = EMAC_READ(sc, EMAC_CONTROL_REG);
	soc_con1 = bus_space_read_4(sc->sc_bst, sc->sc_soc_con1_bsh, 0);

	switch (IFM_SUBTYPE(mii->mii_media_active)) {
	case IFM_10_T:
		soc_con1 |= RKEMAC_SOC_CON1_EMAC_SPEED_MASK;
		soc_con1 &= ~RKEMAC_SOC_CON1_EMAC_SPEED;
		break;
	case IFM_100_TX:
		soc_con1 |= RKEMAC_SOC_CON1_EMAC_SPEED_MASK;
		soc_con1 |= RKEMAC_SOC_CON1_EMAC_SPEED;
		break;
	}

	if (mii->mii_media_active & IFM_FDX) {
		control |= EMAC_CONTROL_ENFL;
	} else {
		control &= ~EMAC_CONTROL_ENFL;
	}

	bus_space_write_4(sc->sc_bst, sc->sc_soc_con1_bsh, 0, soc_con1);
	EMAC_WRITE(sc, EMAC_CONTROL_REG, control);
}

static int
rkemac_init(struct ifnet *ifp)
{
	struct rkemac_softc *sc = ifp->if_softc;
	uint32_t control;

	if (ifp->if_flags & IFF_RUNNING)
		return 0;

	rkemac_stop(ifp, 0);

	EMAC_WRITE(sc, EMAC_POLLRATE_REG, RKEMAC_POLLRATE);
	EMAC_WRITE(sc, EMAC_RXRINGPTR_REG, sc->sc_rxq.r_physaddr);
	EMAC_WRITE(sc, EMAC_TXRINGPTR_REG, sc->sc_txq.t_physaddr);

	control = EMAC_READ(sc, EMAC_CONTROL_REG);
	if (ifp->if_flags & IFF_PROMISC) {
		control |= EMAC_CONTROL_PROM;
	} else {
		control &= ~EMAC_CONTROL_PROM;
	}
	if (ifp->if_flags & IFF_BROADCAST) {
		control &= ~EMAC_CONTROL_DISBC;
	} else {
		control |= EMAC_CONTROL_DISBC;
	}

	control &= ~EMAC_CONTROL_RXBDTLEN;
	control |= __SHIFTIN(RKEMAC_RX_RING_COUNT, EMAC_CONTROL_RXBDTLEN);
	control &= ~EMAC_CONTROL_TXBDTLEN;
	control |= __SHIFTIN(RKEMAC_TX_RING_COUNT, EMAC_CONTROL_TXBDTLEN);
	control |= EMAC_CONTROL_RXRN;
	control |= EMAC_CONTROL_TXRN;
	EMAC_WRITE(sc, EMAC_CONTROL_REG, control);

	rkemac_setmulti(sc);

	control |= EMAC_CONTROL_EN;
	EMAC_WRITE(sc, EMAC_CONTROL_REG, control);

	callout_schedule(&sc->sc_mii_tick, hz);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return 0;
}

static void
rkemac_start(struct ifnet *ifp)
{
	struct rkemac_softc *sc = ifp->if_softc;
	const int old = sc->sc_txq.t_queued;
	const int start = sc->sc_txq.t_cur;
	struct mbuf *m0;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
		if (rkemac_queue(sc, m0) != 0) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		bpf_mtap(ifp, m0);
		if (sc->sc_txq.t_queued == RKEMAC_TX_RING_COUNT) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
	}

	if (sc->sc_txq.t_queued != old) {
		rkemac_txdesc_sync(sc, start, sc->sc_txq.t_cur,
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		EMAC_WRITE(sc, EMAC_STAT_REG, EMAC_STAT_TXPL);
	}
}

static void
rkemac_stop(struct ifnet *ifp, int disable)
{
	struct rkemac_softc *sc = ifp->if_softc;
	uint32_t control;

	callout_halt(&sc->sc_mii_tick, NULL);

	control = EMAC_READ(sc, EMAC_CONTROL_REG);
	control &= ~EMAC_CONTROL_EN;
	EMAC_WRITE(sc, EMAC_CONTROL_REG, control);

	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	mii_down(&sc->sc_mii);

	/* reset TX/RX rings */
	for (int i = 0; i < RKEMAC_RX_RING_COUNT; i++) {
		struct rkemac_rxdesc *rx = &sc->sc_rxq.r_desc[i];
		rx->rx_info = htole32(EMAC_RXDESC_OWN |
		    __SHIFTIN(RKEMAC_MAX_PACKET, EMAC_RXDESC_RXLEN));
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
	    0, RKEMAC_RX_RING_COUNT * sizeof(struct rkemac_rxdesc),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	sc->sc_rxq.r_cur = sc->sc_rxq.r_next = 0;
	for (int i = 0; i < RKEMAC_TX_RING_COUNT; i++) {
		struct rkemac_txdata *td = &sc->sc_txq.t_data[i];
		if (td->td_m) {
			bus_dmamap_sync(sc->sc_dmat, td->td_active,
			    0, td->td_active->dm_mapsize,
			    BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, td->td_active);
			m_freem(td->td_m);
			td->td_m = NULL;
		}
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
	    RKEMAC_RX_RING_COUNT,
	    RKEMAC_TX_RING_COUNT * sizeof(struct rkemac_txdesc),
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	sc->sc_txq.t_queued = sc->sc_txq.t_cur = sc->sc_txq.t_next = 0;
}

static int
rkemac_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct rkemac_softc *sc = ifp->if_softc;
	int s, error;

	s = splnet();

	if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
		error = 0;
		if (ifp->if_flags & IFF_RUNNING) {
			if (cmd == SIOCADDMULTI || cmd == SIOCDELMULTI) {
				rkemac_setmulti(sc);
			}
		}
	}

	if (ifp->if_flags & IFF_UP)
		rkemac_start(ifp);

	splx(s);

	return error;
}

static void
rkemac_watchdog(struct ifnet *ifp)
{
	struct rkemac_softc *sc = ifp->if_softc;

	device_printf(sc->sc_dev, "watchdog timeout\n");
	++ifp->if_oerrors;
	rkemac_init(ifp);
}

static void
rkemac_tick(void *priv)
{
	struct rkemac_softc *sc = priv;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_schedule(&sc->sc_mii_tick, hz);
}

static int
rkemac_queue(struct rkemac_softc *sc, struct mbuf *m0)
{
	struct rkemac_txdata *td = NULL;
	struct rkemac_txdesc *tx = NULL;
	bus_dmamap_t map;
	uint32_t info, len, padlen;
	int error, first;

	first = sc->sc_txq.t_cur;
	map = sc->sc_txq.t_data[first].td_map;

	error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m0,
	    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	if (error) {
		device_printf(sc->sc_dev, "couldn't map mbuf: %d\n", error);
		return error;
	}

	KASSERT(map->dm_nsegs > 0);

	int nbufs = map->dm_nsegs;

	if (m0->m_pkthdr.len < ETHER_MIN_LEN) {
		padlen = ETHER_MIN_LEN - m0->m_pkthdr.len;
		nbufs++;
	} else {
		padlen = 0;
	}

	if (sc->sc_txq.t_queued + nbufs > RKEMAC_TX_RING_COUNT) {
		bus_dmamap_unload(sc->sc_dmat, map);
		return ENOBUFS;
	}

	info = EMAC_TXDESC_FIRST;
	for (int i = 0; i < map->dm_nsegs; i++) {
		td = &sc->sc_txq.t_data[sc->sc_txq.t_cur];
		tx = &sc->sc_txq.t_desc[sc->sc_txq.t_cur];

		tx->tx_ptr = htole32(map->dm_segs[i].ds_addr);
		len = __SHIFTIN(map->dm_segs[i].ds_len, EMAC_TXDESC_TXLEN);
		tx->tx_info = htole32(info | len);
		info &= ~EMAC_TXDESC_FIRST;
		info |= EMAC_TXDESC_OWN;

		sc->sc_txq.t_queued++;
		sc->sc_txq.t_cur = TX_NEXT(sc->sc_txq.t_cur);
	}

	if (padlen) {
		td = &sc->sc_txq.t_data[sc->sc_txq.t_cur];
		tx = &sc->sc_txq.t_desc[sc->sc_txq.t_cur];

		tx->tx_ptr = htole32(sc->sc_pad_physaddr);
		len = __SHIFTIN(padlen, EMAC_TXDESC_TXLEN);
		tx->tx_info = htole32(info | len);

		sc->sc_txq.t_queued++;
		sc->sc_txq.t_cur = TX_NEXT(sc->sc_txq.t_cur);
	}

	tx->tx_info |= htole32(EMAC_TXDESC_LAST);

	td->td_m = m0;
	td->td_active = map;
	td->td_nbufs = nbufs;

	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	sc->sc_txq.t_desc[first].tx_info |= htole32(EMAC_TXDESC_OWN);

	return 0;
}

static void
rkemac_txdesc_sync(struct rkemac_softc *sc, int start, int end, int ops)
{
	if (end > start) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
		    TX_DESC_OFFSET(start),
		    TX_DESC_OFFSET(end) - TX_DESC_OFFSET(start), ops);
		return;
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
	    TX_DESC_OFFSET(start),
	    TX_DESC_OFFSET(RKEMAC_TX_RING_COUNT) - TX_DESC_OFFSET(start),
	    ops);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
	    TX_DESC_OFFSET(0), TX_DESC_OFFSET(end) - TX_DESC_OFFSET(0), ops);
}

static void
rkemac_txintr(struct rkemac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int i;

	for (i = sc->sc_txq.t_next; sc->sc_txq.t_queued > 0; i = TX_NEXT(i)) {
		struct rkemac_txdata *td = &sc->sc_txq.t_data[i];
		struct rkemac_txdesc *tx = &sc->sc_txq.t_desc[i];

		rkemac_txdesc_sync(sc, i, i + 1,
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		const uint32_t info = le32toh(tx->tx_info);
		if (info & EMAC_TXDESC_OWN)
			break;

		if (td->td_m == NULL)
			continue;

		ifp->if_opackets++;
		bus_dmamap_sync(sc->sc_dmat, td->td_active, 0,
		    td->td_active->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, td->td_active);

		m_freem(td->td_m);
		td->td_m = NULL;

		sc->sc_txq.t_queued -= td->td_nbufs;
	}

	sc->sc_txq.t_next = i;

	if (sc->sc_txq.t_queued < RKEMAC_TX_RING_COUNT) {
		ifp->if_flags &= ~IFF_OACTIVE;
	}
}

static void
rkemac_rxintr(struct rkemac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct mbuf *m, *mnew;
	int i, error;

	for (i = sc->sc_rxq.r_cur; ; i = RX_NEXT(i)) {
		struct rkemac_rxdata *rd = &sc->sc_rxq.r_data[i];
		struct rkemac_rxdesc *rx = &sc->sc_rxq.r_desc[i];

		bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
		    RX_DESC_OFFSET(i), sizeof(struct rkemac_rxdesc),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		const uint32_t info = le32toh(rx->rx_info);
		if (info & EMAC_RXDESC_OWN)
			break;

		if (info & EMAC_RXDESC_BUFF) {
			ifp->if_ierrors++;
			goto skip;
		}

		const u_int len = __SHIFTOUT(info, EMAC_RXDESC_RXLEN);

		MGETHDR(mnew, M_DONTWAIT, MT_DATA);
		if (mnew == NULL) {
			ifp->if_ierrors++;
			goto skip;
		}
		MCLGET(mnew, M_DONTWAIT);
		if ((mnew->m_flags & M_EXT) == 0) {
			m_freem(mnew);
			ifp->if_ierrors++;
			goto skip;
		}

		bus_dmamap_sync(sc->sc_dmat, rd->rd_map, 0,
		    rd->rd_map->dm_mapsize, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, rd->rd_map);

		error = bus_dmamap_load(sc->sc_dmat, rd->rd_map,
		    mtod(mnew, void *), MCLBYTES, NULL,
		    BUS_DMA_READ | BUS_DMA_NOWAIT);
		if (error) {
			m_freem(mnew);
			error = bus_dmamap_load(sc->sc_dmat, rd->rd_map,
			    mtod(rd->rd_m, void *), MCLBYTES, NULL,
			    BUS_DMA_READ | BUS_DMA_NOWAIT);
			if (error)
				panic("%s: could not load old rx mbuf",
				    device_xname(sc->sc_dev));
			ifp->if_ierrors++;
			goto skip;
		}

		m = rd->rd_m;
		rd->rd_m = mnew;
		rx->rx_ptr = htole32(rd->rd_map->dm_segs[0].ds_addr);

		m->m_pkthdr.len = m->m_len = len;
		m_set_rcvif(m, ifp);
		m->m_flags |= M_HASFCS;

		if_percpuq_enqueue(ifp->if_percpuq, m);

skip:
		bus_dmamap_sync(sc->sc_dmat, rd->rd_map, 0,
		    rd->rd_map->dm_mapsize, BUS_DMASYNC_PREREAD);
		rx->rx_info = htole32(EMAC_RXDESC_OWN |
		    __SHIFTIN(RKEMAC_MAX_PACKET, EMAC_RXDESC_RXLEN));
		bus_dmamap_sync(sc->sc_dmat, sc->sc_ring_dmamap,
		    RX_DESC_OFFSET(i), sizeof(struct rkemac_rxdesc),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	}

	sc->sc_rxq.r_cur = i;
}

static void
rkemac_setmulti(struct rkemac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t hashes[2] = { 0, 0 };
	uint32_t control, h;
	int s;

	s = splnet();

	control = EMAC_READ(sc, EMAC_CONTROL_REG);

	if (ifp->if_flags & IFF_PROMISC) {
		control |= EMAC_CONTROL_PROM;
		hashes[0] = hashes[1] = 0xffffffff;
		goto done;
	}

	ifp->if_flags &= ~IFF_ALLMULTI;
	control &= ~EMAC_CONTROL_PROM;

	ETHER_FIRST_MULTI(step, &sc->sc_ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			control |= EMAC_CONTROL_PROM;
			ifp->if_flags |= IFF_ALLMULTI;
			hashes[0] = hashes[1] = 0xffffffff;
			goto done;
		}
		h = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN) >> 26;
		hashes[h >> 5] |= (1 << (h & 0x1f));
		ETHER_NEXT_MULTI(step, enm);
	}

done:
	EMAC_WRITE(sc, EMAC_CONTROL_REG, control);
	EMAC_WRITE(sc, EMAC_LAFL_REG, hashes[0]);
	EMAC_WRITE(sc, EMAC_LAFH_REG, hashes[1]);

	splx(s);
}
