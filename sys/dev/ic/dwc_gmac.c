/*-
 * Copyright (c) 2013, 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry and Martin Husemann.
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

/*
 * This driver supports the Synopsis Designware GMAC core, as found
 * on Allwinner A20 cores and others.
 *
 * Real documentation seems to not be available, the marketing product
 * documents could be found here:
 *
 *  http://www.synopsys.com/dw/ipdir.php?ds=dwc_ether_mac10_100_1000_unive
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: dwc_gmac.c,v 1.8 2014/10/08 18:24:21 martin Exp $");

/* #define	DWC_GMAC_DEBUG	1 */

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>
#ifdef INET
#include <netinet/if_inarp.h>
#endif

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_gmac_reg.h>
#include <dev/ic/dwc_gmac_var.h>

static int dwc_gmac_miibus_read_reg(device_t, int, int);
static void dwc_gmac_miibus_write_reg(device_t, int, int, int);
static void dwc_gmac_miibus_statchg(struct ifnet *);

static int dwc_gmac_reset(struct dwc_gmac_softc *sc);
static void dwc_gmac_write_hwaddr(struct dwc_gmac_softc *sc,
			 uint8_t enaddr[ETHER_ADDR_LEN]);
static int dwc_gmac_alloc_dma_rings(struct dwc_gmac_softc *sc);
static void dwc_gmac_free_dma_rings(struct dwc_gmac_softc *sc);
static int dwc_gmac_alloc_rx_ring(struct dwc_gmac_softc *sc, struct dwc_gmac_rx_ring *);
static void dwc_gmac_reset_rx_ring(struct dwc_gmac_softc *sc, struct dwc_gmac_rx_ring *);
static void dwc_gmac_free_rx_ring(struct dwc_gmac_softc *sc, struct dwc_gmac_rx_ring *);
static int dwc_gmac_alloc_tx_ring(struct dwc_gmac_softc *sc, struct dwc_gmac_tx_ring *);
static void dwc_gmac_reset_tx_ring(struct dwc_gmac_softc *sc, struct dwc_gmac_tx_ring *);
static void dwc_gmac_free_tx_ring(struct dwc_gmac_softc *sc, struct dwc_gmac_tx_ring *);
static void dwc_gmac_txdesc_sync(struct dwc_gmac_softc *sc, int start, int end, int ops);
static int dwc_gmac_init(struct ifnet *ifp);
static void dwc_gmac_stop(struct ifnet *ifp, int disable);
static void dwc_gmac_start(struct ifnet *ifp);
static int dwc_gmac_queue(struct dwc_gmac_softc *sc, struct mbuf *m0);
static int dwc_gmac_ioctl(struct ifnet *, u_long, void *);
static void dwc_gmac_tx_intr(struct dwc_gmac_softc *sc);
static void dwc_gmac_rx_intr(struct dwc_gmac_softc *sc);

#define	TX_DESC_OFFSET(N)	((AWGE_RX_RING_COUNT+(N)) \
				    *sizeof(struct dwc_gmac_dev_dmadesc))
#define	TX_NEXT(N)		(((N)+1) & (AWGE_TX_RING_COUNT-1))

#define RX_DESC_OFFSET(N)	((N)*sizeof(struct dwc_gmac_dev_dmadesc))
#define	RX_NEXT(N)		(((N)+1) & (AWGE_RX_RING_COUNT-1))



#define	GMAC_DEF_DMA_INT_MASK	(GMAC_DMA_INT_TIE|GMAC_DMA_INT_RIE| \
				GMAC_DMA_INT_NIE|GMAC_DMA_INT_AIE| \
				GMAC_DMA_INT_FBE|GMAC_DMA_INT_UNE)

#define	GMAC_DMA_INT_ERRORS	(GMAC_DMA_INT_AIE|GMAC_DMA_INT_ERE| \
				GMAC_DMA_INT_FBE|GMAC_DMA_INT_ETE| \
				GMAC_DMA_INT_RWE|GMAC_DMA_INT_RUE| \
				GMAC_DMA_INT_UNE|GMAC_DMA_INT_OVE| \
				GMAC_DMA_INT_TJE|GMAC_DMA_INT_TUE)

#define	AWIN_DEF_MAC_INTRMASK	\
	(AWIN_GMAC_MAC_INT_TSI | AWIN_GMAC_MAC_INT_ANEG |	\
	AWIN_GMAC_MAC_INT_LINKCHG | AWIN_GMAC_MAC_INT_RGSMII)


#ifdef DWC_GMAC_DEBUG
static void dwc_gmac_dump_dma(struct dwc_gmac_softc *sc);
static void dwc_gmac_dump_tx_desc(struct dwc_gmac_softc *sc);
static void dwc_dump_and_abort(struct dwc_gmac_softc *sc, const char *msg);
#endif

void
dwc_gmac_attach(struct dwc_gmac_softc *sc, uint32_t mii_clk)
{
	uint8_t enaddr[ETHER_ADDR_LEN];
	uint32_t maclo, machi;
	struct mii_data * const mii = &sc->sc_mii;
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	prop_dictionary_t dict;

	mutex_init(&sc->sc_mdio_lock, MUTEX_DEFAULT, IPL_NET);
	sc->sc_mii_clk = mii_clk & 7;

	dict = device_properties(sc->sc_dev);
	prop_data_t ea = dict ? prop_dictionary_get(dict, "mac-address") : NULL;
	if (ea != NULL) {
		/*
		 * If the MAC address is overriden by a device property,
		 * use that.
		 */
		KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
		memcpy(enaddr, prop_data_data_nocopy(ea), ETHER_ADDR_LEN);
	} else {
		/*
		 * If we did not get an externaly configure address,
		 * try to read one from the current filter setup,
		 * before resetting the chip.
		 */
		maclo = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    AWIN_GMAC_MAC_ADDR0LO);
		machi = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    AWIN_GMAC_MAC_ADDR0HI);
		enaddr[0] = maclo & 0x0ff;
		enaddr[1] = (maclo >> 8) & 0x0ff;
		enaddr[2] = (maclo >> 16) & 0x0ff;
		enaddr[3] = (maclo >> 24) & 0x0ff;
		enaddr[4] = machi & 0x0ff;
		enaddr[5] = (machi >> 8) & 0x0ff;
	}

	/*
	 * Init chip and do intial setup
	 */
	if (dwc_gmac_reset(sc) != 0)
		return;	/* not much to cleanup, haven't attached yet */
	dwc_gmac_write_hwaddr(sc, enaddr);
	aprint_normal_dev(sc->sc_dev, "Ethernet address: %s\n",
	    ether_sprintf(enaddr));

	/*
	 * Allocate Tx and Rx rings
	 */
	if (dwc_gmac_alloc_dma_rings(sc) != 0) {
		aprint_error_dev(sc->sc_dev, "could not allocate DMA rings\n");
		goto fail;
	}
		
	if (dwc_gmac_alloc_tx_ring(sc, &sc->sc_txq) != 0) {
		aprint_error_dev(sc->sc_dev, "could not allocate Tx ring\n");
		goto fail;
	}

	mutex_init(&sc->sc_rxq.r_mtx, MUTEX_DEFAULT, IPL_NET);
	if (dwc_gmac_alloc_rx_ring(sc, &sc->sc_rxq) != 0) {
		aprint_error_dev(sc->sc_dev, "could not allocate Rx ring\n");
		goto fail;
	}

	/*
	 * Prepare interface data
	 */
	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = dwc_gmac_ioctl;
	ifp->if_start = dwc_gmac_start;
	ifp->if_init = dwc_gmac_init;
	ifp->if_stop = dwc_gmac_stop;
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifp->if_snd);

	/*
	 * Attach MII subdevices
	 */
	sc->sc_ec.ec_mii = &sc->sc_mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
        mii->mii_ifp = ifp;
        mii->mii_readreg = dwc_gmac_miibus_read_reg;
        mii->mii_writereg = dwc_gmac_miibus_write_reg;
        mii->mii_statchg = dwc_gmac_miibus_statchg;
        mii_attach(sc->sc_dev, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY, 0);

        if (LIST_EMPTY(&mii->mii_phys)) { 
                aprint_error_dev(sc->sc_dev, "no PHY found!\n");
                ifmedia_add(&mii->mii_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
                ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_MANUAL);
        } else {
                ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);
        }

	/*
	 * Ready, attach interface
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	/*
	 * Enable interrupts
	 */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_MAC_INTR,
	    AWIN_DEF_MAC_INTRMASK);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_INTENABLE,
	    GMAC_DEF_DMA_INT_MASK);

	return;

fail:
	dwc_gmac_free_rx_ring(sc, &sc->sc_rxq);
	dwc_gmac_free_tx_ring(sc, &sc->sc_txq);
}



static int
dwc_gmac_reset(struct dwc_gmac_softc *sc)
{
	size_t cnt;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_BUSMODE,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_BUSMODE) | GMAC_BUSMODE_RESET);
	for (cnt = 0; cnt < 3000; cnt++) {
		if ((bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_BUSMODE)
		    & GMAC_BUSMODE_RESET) == 0)
			return 0;
		delay(10);
	}

	aprint_error_dev(sc->sc_dev, "reset timed out\n");
	return EIO;
}

static void
dwc_gmac_write_hwaddr(struct dwc_gmac_softc *sc,
    uint8_t enaddr[ETHER_ADDR_LEN])
{
	uint32_t lo, hi;

	lo = enaddr[0] | (enaddr[1] << 8) | (enaddr[2] << 16)
	    | (enaddr[3] << 24);
	hi = enaddr[4] | (enaddr[5] << 8);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_MAC_ADDR0LO, lo);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_MAC_ADDR0HI, hi);
}

static int
dwc_gmac_miibus_read_reg(device_t self, int phy, int reg)
{
	struct dwc_gmac_softc * const sc = device_private(self);
	uint16_t mii;
	size_t cnt;
	int rv = 0;

	mii = __SHIFTIN(phy,GMAC_MII_PHY_MASK)
	    | __SHIFTIN(reg,GMAC_MII_REG_MASK)
	    | __SHIFTIN(sc->sc_mii_clk,GMAC_MII_CLKMASK)
	    | GMAC_MII_BUSY;

	mutex_enter(&sc->sc_mdio_lock);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_MAC_MIIADDR, mii);

	for (cnt = 0; cnt < 1000; cnt++) {
		if (!(bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    AWIN_GMAC_MAC_MIIADDR) & GMAC_MII_BUSY)) {
			rv = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
			    AWIN_GMAC_MAC_MIIDATA);
			break;
		}
		delay(10);
	}

	mutex_exit(&sc->sc_mdio_lock);

	return rv;
}

static void
dwc_gmac_miibus_write_reg(device_t self, int phy, int reg, int val)
{
	struct dwc_gmac_softc * const sc = device_private(self);
	uint16_t mii;
	size_t cnt;

	mii = __SHIFTIN(phy,GMAC_MII_PHY_MASK)
	    | __SHIFTIN(reg,GMAC_MII_REG_MASK)
	    | __SHIFTIN(sc->sc_mii_clk,GMAC_MII_CLKMASK)
	    | GMAC_MII_BUSY | GMAC_MII_WRITE;

	mutex_enter(&sc->sc_mdio_lock);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_MAC_MIIDATA, val);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_MAC_MIIADDR, mii);

	for (cnt = 0; cnt < 1000; cnt++) {
		if (!(bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    AWIN_GMAC_MAC_MIIADDR) & GMAC_MII_BUSY))
			break;
		delay(10);
	}
	
	mutex_exit(&sc->sc_mdio_lock);
}

static int
dwc_gmac_alloc_rx_ring(struct dwc_gmac_softc *sc,
	struct dwc_gmac_rx_ring *ring)
{
	struct dwc_gmac_rx_data *data;
	bus_addr_t physaddr;
	const size_t descsize = AWGE_RX_RING_COUNT * sizeof(*ring->r_desc);
	int error, i, next;

	ring->r_cur = ring->r_next = 0;
	memset(ring->r_desc, 0, descsize);

	/*
	 * Pre-allocate Rx buffers and populate Rx ring.
	 */
	for (i = 0; i < AWGE_RX_RING_COUNT; i++) {
		struct dwc_gmac_dev_dmadesc *desc;

		data = &sc->sc_rxq.r_data[i];

		MGETHDR(data->rd_m, M_DONTWAIT, MT_DATA);
		if (data->rd_m == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "could not allocate rx mbuf #%d\n", i);
			error = ENOMEM;
			goto fail;
		}
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_NOWAIT, &data->rd_map);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev,
			    "could not create DMA map\n");
			data->rd_map = NULL;
			goto fail;
		}
		MCLGET(data->rd_m, M_DONTWAIT);
		if (!(data->rd_m->m_flags & M_EXT)) {
			aprint_error_dev(sc->sc_dev,
			    "could not allocate mbuf cluster #%d\n", i);
			error = ENOMEM;
			goto fail;
		}

		error = bus_dmamap_load(sc->sc_dmat, data->rd_map,
		    mtod(data->rd_m, void *), MCLBYTES, NULL,
		    BUS_DMA_READ | BUS_DMA_NOWAIT);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev,
			    "could not load rx buf DMA map #%d", i);
			goto fail;
		}
		physaddr = data->rd_map->dm_segs[0].ds_addr;

		desc = &sc->sc_rxq.r_desc[i];
		desc->ddesc_data = htole32(physaddr);
		next = RX_NEXT(i);
		desc->ddesc_next = htole32(ring->r_physaddr 
		    + next * sizeof(*desc));
		desc->ddesc_cntl = htole32(
		    __SHIFTIN(AWGE_MAX_PACKET,DDESC_CNTL_SIZE1MASK));
		desc->ddesc_status = htole32(DDESC_STATUS_OWNEDBYDEV);
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_ring_map, 0,
	    AWGE_RX_RING_COUNT*sizeof(struct dwc_gmac_dev_dmadesc),
	    BUS_DMASYNC_PREREAD);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_RX_ADDR,
	    ring->r_physaddr);

	return 0;

fail:
	dwc_gmac_free_rx_ring(sc, ring);
	return error;
}

static void
dwc_gmac_reset_rx_ring(struct dwc_gmac_softc *sc,
	struct dwc_gmac_rx_ring *ring)
{
	struct dwc_gmac_dev_dmadesc *desc;
	int i;

	for (i = 0; i < AWGE_RX_RING_COUNT; i++) {
		desc = &sc->sc_rxq.r_desc[i];
		desc->ddesc_cntl = htole32(
		    __SHIFTIN(AWGE_MAX_PACKET,DDESC_CNTL_SIZE1MASK));
		desc->ddesc_status = htole32(DDESC_STATUS_OWNEDBYDEV);
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_ring_map, 0,
	    AWGE_RX_RING_COUNT*sizeof(struct dwc_gmac_dev_dmadesc),
	    BUS_DMASYNC_PREWRITE);

	ring->r_cur = ring->r_next = 0;
}

static int
dwc_gmac_alloc_dma_rings(struct dwc_gmac_softc *sc)
{
	const size_t descsize = AWGE_TOTAL_RING_COUNT *
		sizeof(struct dwc_gmac_dev_dmadesc);
	int error, nsegs;
	void *rings;

	error = bus_dmamap_create(sc->sc_dmat, descsize, 1, descsize, 0,
	    BUS_DMA_NOWAIT, &sc->sc_dma_ring_map);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "could not create desc DMA map\n");
		sc->sc_dma_ring_map = NULL;
		goto fail;
	}

	error = bus_dmamem_alloc(sc->sc_dmat, descsize, PAGE_SIZE, 0,
	    &sc->sc_dma_ring_seg, 1, &nsegs, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "could not map DMA memory\n");
		goto fail;
	}

	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_dma_ring_seg, nsegs,
	    descsize, &rings, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate DMA memory\n");
		goto fail;
	}

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dma_ring_map, rings,
	    descsize, NULL, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "could not load desc DMA map\n");
		goto fail;
	}

	/* give first AWGE_RX_RING_COUNT to the RX side */
	sc->sc_rxq.r_desc = rings;
	sc->sc_rxq.r_physaddr = sc->sc_dma_ring_map->dm_segs[0].ds_addr;

	/* and next rings to the TX side */
	sc->sc_txq.t_desc = sc->sc_rxq.r_desc + AWGE_RX_RING_COUNT;
	sc->sc_txq.t_physaddr = sc->sc_rxq.r_physaddr + 
	    AWGE_RX_RING_COUNT*sizeof(struct dwc_gmac_dev_dmadesc);

	return 0;

fail:
	dwc_gmac_free_dma_rings(sc);
	return error;
}

static void
dwc_gmac_free_dma_rings(struct dwc_gmac_softc *sc)
{
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_ring_map, 0,
	    sc->sc_dma_ring_map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dma_ring_map);
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_rxq.r_desc,
	    AWGE_TOTAL_RING_COUNT * sizeof(struct dwc_gmac_dev_dmadesc));
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_ring_seg, 1);
}

static void
dwc_gmac_free_rx_ring(struct dwc_gmac_softc *sc, struct dwc_gmac_rx_ring *ring)
{
	struct dwc_gmac_rx_data *data;
	int i;

	if (ring->r_desc == NULL)
		return;


	for (i = 0; i < AWGE_RX_RING_COUNT; i++) {
		data = &ring->r_data[i];

		if (data->rd_map != NULL) {
			bus_dmamap_sync(sc->sc_dmat, data->rd_map, 0,
			    AWGE_RX_RING_COUNT
				*sizeof(struct dwc_gmac_dev_dmadesc),
			    BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(sc->sc_dmat, data->rd_map);
			bus_dmamap_destroy(sc->sc_dmat, data->rd_map);
		}
		if (data->rd_m != NULL)
			m_freem(data->rd_m);
	}
}

static int
dwc_gmac_alloc_tx_ring(struct dwc_gmac_softc *sc,
	struct dwc_gmac_tx_ring *ring)
{
	int i, error = 0;

	ring->t_queued = 0;
	ring->t_cur = ring->t_next = 0;

	memset(ring->t_desc, 0, AWGE_TX_RING_COUNT*sizeof(*ring->t_desc));
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_ring_map,
	    TX_DESC_OFFSET(0),
	    AWGE_TX_RING_COUNT*sizeof(struct dwc_gmac_dev_dmadesc),
	    BUS_DMASYNC_POSTWRITE);

	for (i = 0; i < AWGE_TX_RING_COUNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    AWGE_TX_RING_COUNT, MCLBYTES, 0,
		    BUS_DMA_NOWAIT|BUS_DMA_COHERENT,
		    &ring->t_data[i].td_map);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev,
			    "could not create TX DMA map #%d\n", i);
			ring->t_data[i].td_map = NULL;
			goto fail;
		}
		ring->t_desc[i].ddesc_next = htole32(
		    ring->t_physaddr + sizeof(struct dwc_gmac_dev_dmadesc)
		    *TX_NEXT(i));
	}

	return 0;

fail:
	dwc_gmac_free_tx_ring(sc, ring);
	return error;
}

static void
dwc_gmac_txdesc_sync(struct dwc_gmac_softc *sc, int start, int end, int ops)
{
	/* 'end' is pointing one descriptor beyound the last we want to sync */
	if (end > start) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_ring_map,
		    TX_DESC_OFFSET(start),
		    TX_DESC_OFFSET(end)-TX_DESC_OFFSET(start),
		    ops);
		return;
	}
	/* sync from 'start' to end of ring */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_ring_map,
	    TX_DESC_OFFSET(start),
	    TX_DESC_OFFSET(AWGE_TX_RING_COUNT+1)-TX_DESC_OFFSET(start),
	    ops);
	/* sync from start of ring to 'end' */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_ring_map,
	    TX_DESC_OFFSET(0),
	    TX_DESC_OFFSET(end)-TX_DESC_OFFSET(0),
	    ops);
}

static void
dwc_gmac_reset_tx_ring(struct dwc_gmac_softc *sc,
	struct dwc_gmac_tx_ring *ring)
{
	int i;

	for (i = 0; i < AWGE_TX_RING_COUNT; i++) {
		struct dwc_gmac_tx_data *data = &ring->t_data[i];

		if (data->td_m != NULL) {
			bus_dmamap_sync(sc->sc_dmat, data->td_active,
			    0, data->td_active->dm_mapsize,
			    BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, data->td_active);
			m_freem(data->td_m);
			data->td_m = NULL;
		}
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_ring_map,
	    TX_DESC_OFFSET(0),
	    AWGE_TX_RING_COUNT*sizeof(struct dwc_gmac_dev_dmadesc),
	    BUS_DMASYNC_PREWRITE);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_TX_ADDR,
	    sc->sc_txq.t_physaddr);

	ring->t_queued = 0;
	ring->t_cur = ring->t_next = 0;
}

static void
dwc_gmac_free_tx_ring(struct dwc_gmac_softc *sc,
	struct dwc_gmac_tx_ring *ring)
{
	int i;

	/* unload the maps */
	for (i = 0; i < AWGE_TX_RING_COUNT; i++) {
		struct dwc_gmac_tx_data *data = &ring->t_data[i];

		if (data->td_m != NULL) {
			bus_dmamap_sync(sc->sc_dmat, data->td_active,
			    0, data->td_map->dm_mapsize,
			    BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, data->td_active);
			m_freem(data->td_m);
			data->td_m = NULL;
		}
	}

	/* and actually free them */
	for (i = 0; i < AWGE_TX_RING_COUNT; i++) {
		struct dwc_gmac_tx_data *data = &ring->t_data[i];

		bus_dmamap_destroy(sc->sc_dmat, data->td_map);
	}
}

static void
dwc_gmac_miibus_statchg(struct ifnet *ifp)
{
	struct dwc_gmac_softc * const sc = ifp->if_softc;
	struct mii_data * const mii = &sc->sc_mii;

	/*
	 * Set MII or GMII interface based on the speed
	 * negotiated by the PHY.                                           
	 */                                                                 
	switch (IFM_SUBTYPE(mii->mii_media_active)) {
	case IFM_10_T:
	case IFM_100_TX:
		/* XXX */
		break;
	case IFM_1000_T:
		/* XXX */
		break;
	}
}

static int
dwc_gmac_init(struct ifnet *ifp)
{
	struct dwc_gmac_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_RUNNING)
		return 0;

	dwc_gmac_stop(ifp, 0);

	/*
	 * Set up dma pointer for RX and TX ring
	 */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_RX_ADDR,
	    sc->sc_rxq.r_physaddr);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_TX_ADDR,
	    sc->sc_txq.t_physaddr);

	/*
	 * Start RX part
	 */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh,
	    AWIN_GMAC_DMA_OPMODE, GMAC_DMA_OP_RXSTART);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return 0;
}

static void
dwc_gmac_start(struct ifnet *ifp)
{
	struct dwc_gmac_softc *sc = ifp->if_softc;
	int old = sc->sc_txq.t_queued;
	struct mbuf *m0;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
		if (dwc_gmac_queue(sc, m0) != 0) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		bpf_mtap(ifp, m0);
	}

	if (sc->sc_txq.t_queued != old) {
		/* packets have been queued, kick it off */
		dwc_gmac_txdesc_sync(sc, old, sc->sc_txq.t_cur,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AWIN_GMAC_DMA_OPMODE,
		    bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		        AWIN_GMAC_DMA_OPMODE) | GMAC_DMA_OP_FLUSHTX);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AWIN_GMAC_DMA_OPMODE,
		    bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		        AWIN_GMAC_DMA_OPMODE) | GMAC_DMA_OP_TXSTART);
	}
}

static void
dwc_gmac_stop(struct ifnet *ifp, int disable)
{
	struct dwc_gmac_softc *sc = ifp->if_softc;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh,
	    AWIN_GMAC_DMA_OPMODE,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	        AWIN_GMAC_DMA_OPMODE)
		& ~(GMAC_DMA_OP_TXSTART|GMAC_DMA_OP_RXSTART));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh,
	    AWIN_GMAC_DMA_OPMODE,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	        AWIN_GMAC_DMA_OPMODE) | GMAC_DMA_OP_FLUSHTX);

	mii_down(&sc->sc_mii);
	dwc_gmac_reset_tx_ring(sc, &sc->sc_txq);
	dwc_gmac_reset_rx_ring(sc, &sc->sc_rxq);
}

/*
 * Add m0 to the TX ring
 */
static int
dwc_gmac_queue(struct dwc_gmac_softc *sc, struct mbuf *m0)
{
	struct dwc_gmac_dev_dmadesc *desc = NULL;
	struct dwc_gmac_tx_data *data = NULL;
	bus_dmamap_t map;
	uint32_t flags, len;
	int error, i, first;

#ifdef DWC_GMAC_DEBUG
	aprint_normal_dev(sc->sc_dev,
	    "dwc_gmac_queue: adding mbuf chain %p\n", m0);
#endif

	first = sc->sc_txq.t_cur;
	map = sc->sc_txq.t_data[first].td_map;
	flags = 0;

	error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m0,
	    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "could not map mbuf "
		    "(len: %d, error %d)\n", m0->m_pkthdr.len, error);
		return error;
	}

	if (sc->sc_txq.t_queued + map->dm_nsegs >= AWGE_TX_RING_COUNT - 1) {
		bus_dmamap_unload(sc->sc_dmat, map);
		return ENOBUFS;
	}

	data = NULL;
	flags = DDESC_CNTL_TXFIRST|DDESC_CNTL_TXCHAIN;
	for (i = 0; i < map->dm_nsegs; i++) {
		data = &sc->sc_txq.t_data[sc->sc_txq.t_cur];
		desc = &sc->sc_txq.t_desc[sc->sc_txq.t_cur];

		desc->ddesc_data = htole32(map->dm_segs[i].ds_addr);
		len = __SHIFTIN(map->dm_segs[i].ds_len,DDESC_CNTL_SIZE1MASK);
		if (i == map->dm_nsegs-1)
			flags |= DDESC_CNTL_TXLAST|DDESC_CNTL_TXINT;

#ifdef DWC_GMAC_DEBUG
		aprint_normal_dev(sc->sc_dev, "enqueing desc #%d data %08lx "
		    "len %lu (flags: %08x, len: %08x)\n", sc->sc_txq.t_cur,
		    (unsigned long)map->dm_segs[i].ds_addr,
		    (unsigned long)map->dm_segs[i].ds_len,
		    flags, len);
#endif

		desc->ddesc_cntl = htole32(len|flags);
		flags &= ~DDESC_CNTL_TXFIRST;

		/*
		 * Defer passing ownership of the first descriptor
		 * untill we are done.
		 */
		if (i)
			desc->ddesc_status = htole32(DDESC_STATUS_OWNEDBYDEV);

		sc->sc_txq.t_queued++;
		sc->sc_txq.t_cur = TX_NEXT(sc->sc_txq.t_cur);
	}

	/* Pass first to device */
	sc->sc_txq.t_desc[first].ddesc_status
	    = htole32(DDESC_STATUS_OWNEDBYDEV);

	data->td_m = m0;
	data->td_active = map;

	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	return 0;
}

static int
dwc_gmac_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	// struct dwc_gmac_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;
		dwc_gmac_init(ifp);
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			break;
		}
	default:
		if ((error = ether_ioctl(ifp, cmd, data)) != ENETRESET)
			break;
		error = 0;
		if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			;
		else if (ifp->if_flags & IFF_RUNNING)
			/* setmulti */;
		break;
	}

	splx(s);

	return error;
}

static void
dwc_gmac_tx_intr(struct dwc_gmac_softc *sc)
{
	struct dwc_gmac_tx_data *data;
	struct dwc_gmac_dev_dmadesc *desc;
	uint32_t flags;
	int i;

	for (i = sc->sc_txq.t_next; sc->sc_txq.t_queued > 0;
	    i = TX_NEXT(i), sc->sc_txq.t_queued--) {

#ifdef DWC_GMAC_DEBUG
		aprint_normal_dev(sc->sc_dev,
		    "dwc_gmac_tx_intr: checking desc #%d (t_queued: %d)\n",
		    i, sc->sc_txq.t_queued);
#endif

		desc = &sc->sc_txq.t_desc[i];
		dwc_gmac_txdesc_sync(sc, i, i+1,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		flags = le32toh(desc->ddesc_status);
		if (flags & DDESC_STATUS_OWNEDBYDEV)
			break;
		data = &sc->sc_txq.t_data[i];
		if (data->td_m == NULL)
			continue;
		sc->sc_ec.ec_if.if_opackets++;
		bus_dmamap_sync(sc->sc_dmat, data->td_active, 0,
		    data->td_active->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, data->td_active);

#ifdef DWC_GMAC_DEBUG
		aprint_normal_dev(sc->sc_dev,
		    "dwc_gmac_tx_intr: done with packet at desc #%d, "
		    "freeing mbuf %p\n", i, data->td_m);
#endif

		m_freem(data->td_m);
		data->td_m = NULL;
	}

	sc->sc_txq.t_next = i;

	if (sc->sc_txq.t_queued < AWGE_TX_RING_COUNT) {
		sc->sc_ec.ec_if.if_flags &= ~IFF_OACTIVE;
	}
}

static void
dwc_gmac_rx_intr(struct dwc_gmac_softc *sc)
{
#ifdef DWC_GMAC_DEBUG
	aprint_normal_dev(sc->sc_dev, "rx intr\n");
	/* XXX */
#endif
}

int
dwc_gmac_intr(struct dwc_gmac_softc *sc)
{
	uint32_t status, dma_status;
	int rv = 0;

	status = bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_MAC_INTR);
	if (status & AWIN_GMAC_MII_IRQ) {
		(void)bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    AWIN_GMAC_MII_STATUS);
		rv = 1;
		mii_pollstat(&sc->sc_mii);
	}

	dma_status = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	    AWIN_GMAC_DMA_STATUS);

	if (dma_status & (GMAC_DMA_INT_NIE|GMAC_DMA_INT_AIE))
		rv = 1;

	if (dma_status & GMAC_DMA_INT_TIE)
		dwc_gmac_tx_intr(sc);

	if (dma_status & GMAC_DMA_INT_RIE)
		dwc_gmac_rx_intr(sc);

	/*
	 * Check error conditions
	 */
	if (dma_status & GMAC_DMA_INT_ERRORS) {
		sc->sc_ec.ec_if.if_oerrors++;
#ifdef DWC_GMAC_DEBUG
		dwc_dump_and_abort(sc, "interrupt error condition");
#endif
	}

	/* ack interrupt */
	if (dma_status)
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AWIN_GMAC_DMA_STATUS, dma_status & GMAC_DMA_INT_MASK);

	return rv;
}

#ifdef DWC_GMAC_DEBUG
static void
dwc_gmac_dump_dma(struct dwc_gmac_softc *sc)
{
	aprint_normal_dev(sc->sc_dev, "busmode: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_BUSMODE));
	aprint_normal_dev(sc->sc_dev, "tx poll: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_TXPOLL));
	aprint_normal_dev(sc->sc_dev, "rx poll: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_RXPOLL));
	aprint_normal_dev(sc->sc_dev, "rx descriptors: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_RX_ADDR));
	aprint_normal_dev(sc->sc_dev, "tx descriptors: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_TX_ADDR));
	aprint_normal_dev(sc->sc_dev, "status: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_STATUS));
	aprint_normal_dev(sc->sc_dev, "op mode: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_OPMODE));
	aprint_normal_dev(sc->sc_dev, "int enable: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_INTENABLE));
	aprint_normal_dev(sc->sc_dev, "cur tx: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_CUR_TX_DESC));
	aprint_normal_dev(sc->sc_dev, "cur rx: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_CUR_RX_DESC));
	aprint_normal_dev(sc->sc_dev, "cur tx buffer: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_CUR_TX_BUFADDR));
	aprint_normal_dev(sc->sc_dev, "cur rx buffer: %08x\n",
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_GMAC_DMA_CUR_RX_BUFADDR));
}

static void
dwc_gmac_dump_tx_desc(struct dwc_gmac_softc *sc)
{
	int i;

	aprint_normal_dev(sc->sc_dev, "TX queue: cur=%d, next=%d, queued=%d\n",
	    sc->sc_txq.t_cur, sc->sc_txq.t_next, sc->sc_txq.t_queued);
	aprint_normal_dev(sc->sc_dev, "TX DMA descriptors:\n");
	for (i = 0; i < AWGE_TX_RING_COUNT; i++) {
		struct dwc_gmac_dev_dmadesc *desc = &sc->sc_txq.t_desc[i];
		aprint_normal("#%d (%08lx): status: %08x cntl: %08x data: %08x next: %08x\n",
		    i, sc->sc_txq.t_physaddr + i*sizeof(struct dwc_gmac_dev_dmadesc),
		    le32toh(desc->ddesc_status), le32toh(desc->ddesc_cntl),
		    le32toh(desc->ddesc_data), le32toh(desc->ddesc_next));

	}
}

static void
dwc_dump_and_abort(struct dwc_gmac_softc *sc, const char *msg)
{
	uint32_t status = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	     AWIN_GMAC_MAC_INTR);
	uint32_t dma_status = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	     AWIN_GMAC_DMA_STATUS);
	char buf[200];

	/* print interrupt state */
	snprintb(buf, sizeof(buf), "\177\20"
	    "b\x10""NIE\0"
	    "b\x0f""AIE\0"
	    "b\x0e""ERE\0"
	    "b\x0d""FBE\0"
	    "b\x0a""ETE\0"
	    "b\x09""RWE\0"
	    "b\x08""RSE\0"
	    "b\x07""RUE\0"
	    "b\x06""RIE\0"
	    "b\x05""UNE\0"
	    "b\x04""OVE\0"
	    "b\x03""TJE\0"
	    "b\x02""TUE\0"
	    "b\x01""TSE\0"
	    "b\x00""TIE\0"
	    "\0", dma_status);
	printf("%s: INTR status: %08x, DMA status: %s\n",
	    device_xname(sc->sc_dev),
	    status, buf);

	dwc_gmac_dump_dma(sc);
	dwc_gmac_dump_tx_desc(sc);

	panic(msg);
}
#endif
