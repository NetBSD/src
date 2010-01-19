/* $NetBSD: gemini_gmac.c,v 1.5 2010/01/19 22:06:19 pooka Exp $ */
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

#include "locators.h"
#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bus.h>

#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_obiovar.h>
#include <arm/gemini/gemini_gmacvar.h>
#include <arm/gemini/gemini_gpiovar.h>

#include <dev/mii/mii.h>
#include <dev/mii/mii_bitbang.h>

#include <sys/gpio.h>

__KERNEL_RCSID(0, "$NetBSD: gemini_gmac.c,v 1.5 2010/01/19 22:06:19 pooka Exp $");

#define	SWFREEQ_DESCS	256	/* one page worth */
#define	HWFREEQ_DESCS	256	/* one page worth */

static int geminigmac_match(device_t, cfdata_t, void *);
static void geminigmac_attach(device_t, device_t, void *);
static int geminigmac_find(device_t, cfdata_t, const int *, void *);
static int geminigmac_print(void *aux, const char *name);

static int geminigmac_mii_readreg(device_t, int, int);
static void geminigmac_mii_writereg(device_t, int, int, int);

#define	GPIO_MDIO	21
#define	GPIO_MDCLK	22

#define	MDIN		__BIT(3)
#define	MDOUT		__BIT(2)
#define	MDCLK		__BIT(1)
#define	MDTOPHY		__BIT(0)

CFATTACH_DECL_NEW(geminigmac, sizeof(struct gmac_softc),
    geminigmac_match, geminigmac_attach, NULL, NULL);

extern struct cfdriver geminigmac_cd;
extern struct cfdriver geminigpio_cd;

void
gmac_swfree_min_update(struct gmac_softc *sc)
{
	uint32_t v;

	if (sc->sc_swfreeq != NULL
	    && sc->sc_swfree_min > sc->sc_swfreeq->hwq_size - 1)
		sc->sc_swfree_min = sc->sc_swfreeq->hwq_size - 1;

	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_QFE_THRESHOLD);
	v &= ~QFE_SWFQ_THRESHOLD_MASK;
	v |= QFE_SWFQ_THRESHOLD(sc->sc_swfree_min);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_QFE_THRESHOLD, v);
}

void
gmac_intr_update(struct gmac_softc *sc)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_MASK,
	    ~sc->sc_int_enabled[0]);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_MASK,
	    ~sc->sc_int_enabled[1]);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_MASK,
	    ~sc->sc_int_enabled[2]);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_MASK,
	    ~sc->sc_int_enabled[3]);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_MASK,
	    ~sc->sc_int_enabled[4]);
}

static void
gmac_init(struct gmac_softc *sc)
{
	gmac_hwqmem_t *hqm;

	/*
	 * This shouldn't be needed.
	 */
	for (bus_size_t i = 0; i < GMAC_TOE_QH_SIZE; i += 4) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    GMAC_TOE_QH_OFFSET + i, 0);
	}
#if 0
	{
	bus_space_handle_t global_ioh;
	int error;

	error = bus_space_map(sc->sc_iot, GEMINI_GLOBAL_BASE, 4, 0,
	    &global_ioh);
	KASSERT(error == 0);
	aprint_normal_dev(sc->sc_dev, "gmac_init: global_ioh=%#zx\n", global_ioh);
	bus_space_write_4(sc->sc_iot, global_ioh, GEMINI_GLOBAL_RESET_CTL, 
	    GLOBAL_RESET_GMAC0|GLOBAL_RESET_GMAC1);
	do {
		v = bus_space_read_4(sc->sc_iot, global_ioh,
		    GEMINI_GLOBAL_RESET_CTL);
	} while (v & (GLOBAL_RESET_GMAC0|GLOBAL_RESET_GMAC1));
	bus_space_unmap(sc->sc_iot, global_ioh, 4);
	DELAY(1000);
	}
#endif

	sc->sc_swfree_min = 4; /* MIN_RXMAPS; */

	gmac_swfree_min_update(sc);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_SKBSIZE,
	    SKB_SIZE_SET(PAGE_SIZE, MCLBYTES));

	sc->sc_int_select[0] = INT0_GMAC1;
	sc->sc_int_select[1] = INT1_GMAC1;
	sc->sc_int_select[2] = INT2_GMAC1;
	sc->sc_int_select[3] = INT3_GMAC1;
	sc->sc_int_select[4] = INT4_GMAC1;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_SELECT, INT0_GMAC1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_SELECT, INT1_GMAC1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_SELECT, INT2_GMAC1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_SELECT, INT3_GMAC1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_SELECT, INT4_GMAC1);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_STATUS, ~0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_STATUS, ~0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_STATUS, ~0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_STATUS, ~0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_STATUS, ~0);

	gmac_intr_update(sc);

	aprint_debug_dev(sc->sc_dev, "gmac_init: sts=%#x/%#x/%#x/%#x/%#x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_STATUS),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_STATUS));

	aprint_debug_dev(sc->sc_dev, "gmac_init: mask=%#x/%#x/%#x/%#x/%#x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_MASK),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_MASK),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_MASK),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_MASK),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_MASK));

	aprint_debug_dev(sc->sc_dev, "gmac_init: select=%#x/%#x/%#x/%#x/%#x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT0_SELECT),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT1_SELECT),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT2_SELECT),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT3_SELECT),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GMAC_INT4_SELECT));

	aprint_debug_dev(sc->sc_dev, "gmac_init: create rx dmamap cache\n");
	/*
	 * Allocate the cache for receive dmamaps.
	 */
	sc->sc_rxmaps = gmac_mapcache_create(sc->sc_dmat, MAX_RXMAPS,
	    MCLBYTES, 1);
	KASSERT(sc->sc_rxmaps != NULL);

	aprint_debug_dev(sc->sc_dev, "gmac_init: create tx dmamap cache\n");
	/*
	 * Allocate the cache for transmit dmamaps.
	 */
	sc->sc_txmaps = gmac_mapcache_create(sc->sc_dmat, MAX_TXMAPS,
	    ETHERMTU_JUMBO + ETHER_HDR_LEN, 16);
	KASSERT(sc->sc_txmaps != NULL);

	aprint_debug_dev(sc->sc_dev, "gmac_init: create sw freeq\n");
	/*
	 * Allocate the memory for sw (receive) free queue
	 */
	hqm = gmac_hwqmem_create(sc->sc_rxmaps, 32 /*SWFREEQ_DESCS*/, 1,
	    HQM_PRODUCER|HQM_RX);
	sc->sc_swfreeq = gmac_hwqueue_create(hqm, sc->sc_iot, sc->sc_ioh,
	    GMAC_SWFREEQ_RWPTR, GMAC_SWFREEQ_BASE, 0);
	KASSERT(sc->sc_swfreeq != NULL);

	aprint_debug_dev(sc->sc_dev, "gmac_init: create hw freeq\n");
	/*
	 * Allocate the memory for hw (receive) free queue
	 */
	hqm = gmac_hwqmem_create(sc->sc_rxmaps, HWFREEQ_DESCS, 1,
	    HQM_PRODUCER|HQM_RX);
	sc->sc_hwfreeq = gmac_hwqueue_create(hqm, sc->sc_iot, sc->sc_ioh,
	    GMAC_HWFREEQ_RWPTR, GMAC_HWFREEQ_BASE, 0);
	KASSERT(sc->sc_hwfreeq != NULL);

	aprint_debug_dev(sc->sc_dev, "gmac_init: done\n");
}

int
geminigmac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr != GEMINI_GMAC_BASE)
		return 0;

	return 1;
}

void
geminigmac_attach(device_t parent, device_t self, void *aux)
{
	struct gmac_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	struct gmac_attach_args gma;
	cfdata_t cf;
	uint32_t v;
	int error;

	sc->sc_dev = self;
	sc->sc_iot = obio->obio_iot;
	sc->sc_dmat = obio->obio_dmat;
	sc->sc_gpio_dev = geminigpio_cd.cd_devs[0];
	sc->sc_gpio_mdclk = GPIO_MDCLK;
	sc->sc_gpio_mdout = GPIO_MDIO;
	sc->sc_gpio_mdin = GPIO_MDIO;
	KASSERT(sc->sc_gpio_dev != NULL);

	error = bus_space_map(sc->sc_iot, obio->obio_addr, obio->obio_size, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error(": error mapping registers: %d", error);
		return;
	}

	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, 0);
	aprint_normal(": devid %d rev %d\n", GMAC_TOE_DEVID(v),
	    GMAC_TOE_REVID(v));
	aprint_naive("\n");

	mutex_init(&sc->sc_mdiolock, MUTEX_DEFAULT, IPL_NET);

	/*
	 * Initialize the GPIO pins
	 */
	geminigpio_pin_ctl(sc->sc_gpio_dev, sc->sc_gpio_mdclk, GPIO_PIN_OUTPUT);
	geminigpio_pin_ctl(sc->sc_gpio_dev, sc->sc_gpio_mdout, GPIO_PIN_OUTPUT);
	if (sc->sc_gpio_mdout != sc->sc_gpio_mdin)
		geminigpio_pin_ctl(sc->sc_gpio_dev, sc->sc_gpio_mdin,
		    GPIO_PIN_INPUT);

	/*
	 * Set the MDIO GPIO pins to a known state.
	 */
	geminigpio_pin_write(sc->sc_gpio_dev, sc->sc_gpio_mdclk, 0);
	geminigpio_pin_write(sc->sc_gpio_dev, sc->sc_gpio_mdout, 0);
	sc->sc_mdiobits = MDCLK;

	gmac_init(sc);

	gma.gma_iot = sc->sc_iot;
	gma.gma_ioh = sc->sc_ioh;
	gma.gma_dmat = sc->sc_dmat;

	gma.gma_mii_readreg = geminigmac_mii_readreg;
	gma.gma_mii_writereg = geminigmac_mii_writereg;

	gma.gma_port = 0;
	gma.gma_phy = -1;
	gma.gma_intr = 1;

	cf = config_search_ia(geminigmac_find, sc->sc_dev,
	    geminigmac_cd.cd_name, &gma);
	if (cf != NULL)
		config_attach(sc->sc_dev, cf, &gma, geminigmac_print);

	gma.gma_port = 1;
	gma.gma_phy = -1;
	gma.gma_intr = 2;

	cf = config_search_ia(geminigmac_find, sc->sc_dev,
	    geminigmac_cd.cd_name, &gma);
	if (cf != NULL)
		config_attach(sc->sc_dev, cf, &gma, geminigmac_print);
}

static int
geminigmac_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct gmac_attach_args * const gma = aux;

	if (gma->gma_port != cf->cf_loc[GEMINIGMACCF_PORT])
		return 0;
	if (gma->gma_intr != cf->cf_loc[GEMINIGMACCF_INTR])
		return 0;

	gma->gma_phy = cf->cf_loc[GEMINIGMACCF_PHY];
	gma->gma_intr = cf->cf_loc[GEMINIGMACCF_INTR];

	return config_match(parent, cf, gma);
}

static int
geminigmac_print(void *aux, const char *name)
{
	struct gmac_attach_args * const gma = aux;

	aprint_normal(" port %d", gma->gma_port);
	aprint_normal(" phy %d", gma->gma_phy);
	aprint_normal(" intr %d", gma->gma_intr);

	return UNCONF;
}

static uint32_t
gemini_gmac_gpio_read(device_t dv)
{
	struct gmac_softc * const sc = device_private(dv);
	int value = geminigpio_pin_read(sc->sc_gpio_dev, GPIO_MDIO);

	KASSERT((sc->sc_mdiobits & MDTOPHY) == 0);

	return value ? MDIN : 0;
}

static void
gemini_gmac_gpio_write(device_t dv, uint32_t bits)
{
	struct gmac_softc * const sc = device_private(dv);

	if ((sc->sc_mdiobits ^ bits) & MDTOPHY) {
		int flags = (bits & MDTOPHY) ? GPIO_PIN_OUTPUT : GPIO_PIN_INPUT;
		geminigpio_pin_ctl(sc->sc_gpio_dev, GPIO_MDIO, flags);
	}

	if ((sc->sc_mdiobits ^ bits) & MDOUT) {
		int flags = ((bits & MDOUT) != 0);
		geminigpio_pin_write(sc->sc_gpio_dev, GPIO_MDIO, flags);
	}

	if ((sc->sc_mdiobits ^ bits) & MDCLK) {
		int flags = ((bits & MDCLK) != 0);
		geminigpio_pin_write(sc->sc_gpio_dev, GPIO_MDCLK, flags);
	}

	sc->sc_mdiobits = bits;
}

static const struct mii_bitbang_ops geminigmac_mii_bitbang_ops = {
	.mbo_read = gemini_gmac_gpio_read,
	.mbo_write = gemini_gmac_gpio_write,
	.mbo_bits[MII_BIT_MDO] = MDOUT,
	.mbo_bits[MII_BIT_MDI] = MDIN,
	.mbo_bits[MII_BIT_MDC] = MDCLK,
	.mbo_bits[MII_BIT_DIR_HOST_PHY] = MDTOPHY,
};

int
geminigmac_mii_readreg(device_t dv, int phy, int reg)
{
	device_t parent = device_parent(dv);
	struct gmac_softc * const sc = device_private(parent);
	int rv;

	mutex_enter(&sc->sc_mdiolock);
	rv = mii_bitbang_readreg(parent, &geminigmac_mii_bitbang_ops, phy, reg);
	mutex_exit(&sc->sc_mdiolock);

	//aprint_debug_dev(dv, "mii_readreg(%d, %d): %#x\n", phy, reg, rv);

	return rv;
}

void
geminigmac_mii_writereg(device_t dv, int phy, int reg, int val)
{
	device_t parent = device_parent(dv);
	struct gmac_softc * const sc = device_private(parent);

	//aprint_debug_dev(dv, "mii_writereg(%d, %d, %#x)\n", phy, reg, val);

	mutex_enter(&sc->sc_mdiolock);
	mii_bitbang_writereg(parent, &geminigmac_mii_bitbang_ops, phy, reg, val);
	mutex_exit(&sc->sc_mdiolock);
}


gmac_mapcache_t *
gmac_mapcache_create(bus_dma_tag_t dmat, size_t maxmaps, bus_size_t mapsize,
    int nsegs)
{
	gmac_mapcache_t *mc;

	mc = kmem_zalloc(offsetof(gmac_mapcache_t, mc_maps[maxmaps]),
	    KM_SLEEP);
	if (mc == NULL)
		return NULL;

	mc->mc_max = maxmaps;
	mc->mc_dmat = dmat;
	mc->mc_mapsize = mapsize;
	mc->mc_nsegs = nsegs;
	return mc;
}

void
gmac_mapcache_destroy(gmac_mapcache_t **mc_p)
{
	gmac_mapcache_t *mc = *mc_p;

	if (mc == NULL)
		return;

	KASSERT(mc->mc_used == 0);
	while (mc->mc_free-- > 0) {
		KASSERT(mc->mc_maps[mc->mc_free] != NULL);
		bus_dmamap_destroy(mc->mc_dmat, mc->mc_maps[mc->mc_free]);
		mc->mc_maps[mc->mc_free] = NULL;
	}

	kmem_free(mc, offsetof(gmac_mapcache_t, mc_maps[mc->mc_max]));
	*mc_p = NULL;
}

int
gmac_mapcache_fill(gmac_mapcache_t *mc, size_t limit)
{
	int error;

	KASSERT(limit <= mc->mc_max);
	aprint_debug("gmac_mapcache_fill(%p): limit=%zu used=%zu free=%zu\n",
	    mc, limit, mc->mc_used, mc->mc_free);

	for (error = 0; mc->mc_free + mc->mc_used < limit; mc->mc_free++) {
		KASSERT(mc->mc_maps[mc->mc_free] == NULL);
		error = bus_dmamap_create(mc->mc_dmat, mc->mc_mapsize,
		    mc->mc_nsegs, mc->mc_mapsize, 0,
		    BUS_DMA_ALLOCNOW|BUS_DMA_WAITOK,
		    &mc->mc_maps[mc->mc_free]);
		if (error)
			break;
	}
	aprint_debug("gmac_mapcache_fill(%p): limit=%zu used=%zu free=%zu\n",
	    mc, limit, mc->mc_used, mc->mc_free);

	return error;
}

bus_dmamap_t
gmac_mapcache_get(gmac_mapcache_t *mc)
{
	bus_dmamap_t map;

	KASSERT(mc != NULL);

	if (mc->mc_free == 0) {
		int error;
		if (mc->mc_used == mc->mc_max)
			return NULL;
		error = bus_dmamap_create(mc->mc_dmat, mc->mc_mapsize,
		    mc->mc_nsegs, mc->mc_mapsize, 0,
		    BUS_DMA_ALLOCNOW|BUS_DMA_NOWAIT,
		    &map);
		if (error)
			return NULL;
		KASSERT(mc->mc_maps[mc->mc_free] == NULL);
	} else {
		KASSERT(mc->mc_free <= mc->mc_max);
		map = mc->mc_maps[--mc->mc_free];
		mc->mc_maps[mc->mc_free] = NULL;
	}
	mc->mc_used++;
	KASSERT(map != NULL);

	return map;
}

void
gmac_mapcache_put(gmac_mapcache_t *mc, bus_dmamap_t map)
{
	KASSERT(mc->mc_free + mc->mc_used < mc->mc_max);
	KASSERT(mc->mc_maps[mc->mc_free] == NULL);

	mc->mc_maps[mc->mc_free++] = map;
	mc->mc_used--;
}

gmac_desc_t *
gmac_hwqueue_desc(gmac_hwqueue_t *hwq, size_t i)
{
	i += hwq->hwq_wptr;
	if (i >= hwq->hwq_size)
		i -= hwq->hwq_size;
	return hwq->hwq_base + i; 
}

static void
gmac_hwqueue_txconsume(gmac_hwqueue_t *hwq, const gmac_desc_t *d)
{
	gmac_hwqmem_t * const hqm = hwq->hwq_hqm;
	struct ifnet *ifp;
	bus_dmamap_t map;
	struct mbuf *m;

	IF_DEQUEUE(&hwq->hwq_ifq, m);
	KASSERT(m != NULL);
	map = M_GETCTX(m, bus_dmamap_t);

	bus_dmamap_sync(hqm->hqm_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(hqm->hqm_dmat, map);
	M_SETCTX(m, NULL);
	gmac_mapcache_put(hqm->hqm_mc, map);

	ifp = hwq->hwq_ifp;
	ifp->if_opackets++;
	ifp->if_obytes += m->m_pkthdr.len;

	aprint_debug("gmac_hwqueue_txconsume(%p): %zu@%p: %s m=%p\n",
	    hwq, d - hwq->hwq_base, d, ifp->if_xname, m);

	if (ifp->if_bpf)
		bpf_ops->bpf_mtap(ifp->if_bpf, m);
	m_freem(m);
}

void
gmac_hwqueue_sync(gmac_hwqueue_t *hwq)
{
	gmac_hwqmem_t * const hqm = hwq->hwq_hqm;
	uint32_t v;
	uint16_t old_rptr;
	size_t rptr;

	KASSERT(hqm->hqm_flags & HQM_PRODUCER);

	old_rptr = hwq->hwq_rptr;
	v = bus_space_read_4(hwq->hwq_iot, hwq->hwq_qrwptr_ioh, 0);
	hwq->hwq_rptr = (v >>  0) & 0xffff;
	hwq->hwq_wptr = (v >> 16) & 0xffff;

	if (old_rptr == hwq->hwq_rptr)
		return;

	aprint_debug("gmac_hwqueue_sync(%p): entry rptr old=%u new=%u free=%u(%u)\n",
	    hwq, old_rptr, hwq->hwq_rptr, hwq->hwq_free,
	    hwq->hwq_size - hwq->hwq_free - 1);

	hwq->hwq_free += (hwq->hwq_rptr - old_rptr) & (hwq->hwq_size - 1);
	for (rptr = old_rptr;
	     rptr != hwq->hwq_rptr;
	     rptr = (rptr + 1) & (hwq->hwq_size - 1)) {
		gmac_desc_t * const d = hwq->hwq_base + rptr;
		if (hqm->hqm_flags & HQM_TX) {
			bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap, 
			    sizeof(gmac_desc_t [hwq->hwq_qoff + rptr]),
			    sizeof(gmac_desc_t),
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
			if (d->d_desc3 & htole32(DESC3_EOF))
				gmac_hwqueue_txconsume(hwq, d);
		} else {
			bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap, 
			    sizeof(gmac_desc_t [hwq->hwq_qoff + rptr]),
			    sizeof(gmac_desc_t),
			    BUS_DMASYNC_POSTWRITE);

			aprint_debug("gmac_hwqueue_sync(%p): %zu@%p=%#x/%#x/%#x/%#x\n",
			    hwq, rptr, d, d->d_desc0, d->d_desc1,
			    d->d_bufaddr, d->d_desc3);
			bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap, 
			    sizeof(gmac_desc_t [hwq->hwq_qoff + rptr]),
			    sizeof(gmac_desc_t),
			    BUS_DMASYNC_PREWRITE);
		}
	}

	aprint_debug("gmac_hwqueue_sync(%p): exit rptr old=%u new=%u free=%u(%u)\n",
	    hwq, old_rptr, hwq->hwq_rptr, hwq->hwq_free,
	    hwq->hwq_size - hwq->hwq_free - 1);
}

void
gmac_hwqueue_produce(gmac_hwqueue_t *hwq, size_t count)
{
	gmac_hwqmem_t * const hqm = hwq->hwq_hqm;
	uint16_t wptr;
	uint16_t rptr = bus_space_read_4(hwq->hwq_iot, hwq->hwq_qrwptr_ioh, 0);

	KASSERT(count < hwq->hwq_free);
	KASSERT(hqm->hqm_flags & HQM_PRODUCER);
	KASSERT(hwq->hwq_wptr == bus_space_read_4(hwq->hwq_iot, hwq->hwq_qrwptr_ioh, 0) >> 16);

	aprint_debug("gmac_hwqueue_produce(%p, %zu): rptr=%u(%u) wptr old=%u",
	    hwq, count, hwq->hwq_rptr, rptr, hwq->hwq_wptr);

	hwq->hwq_free -= count;
#if 1
	for (wptr = hwq->hwq_wptr;
	     count > 0;
	     count--, wptr = (wptr + 1) & (hwq->hwq_size - 1)) {
		KASSERT(((wptr + 1) & (hwq->hwq_size - 1)) != hwq->hwq_rptr);
		bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap, 
		    sizeof(gmac_desc_t [hwq->hwq_qoff + wptr]),
		    sizeof(gmac_desc_t),
		    BUS_DMASYNC_PREWRITE);
	}
	KASSERT(count == 0);
	hwq->hwq_wptr = wptr;
#else
	if (hwq->hwq_wptr + count >= hwq->hwq_size) {
		bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap, 
		    sizeof(gmac_desc_t [hwq->hwq_qoff + hwq->hwq_wptr]),
		    sizeof(gmac_desc_t [hwq->hwq_size - hwq->hwq_wptr]),
		    BUS_DMASYNC_PREWRITE);
		count -= hwq->hwq_size - hwq->hwq_wptr;
		hwq->hwq_wptr = 0;
	}
	if (count > 0) {
		bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap, 
		    sizeof(gmac_desc_t [hwq->hwq_qoff + hwq->hwq_wptr]),
		    sizeof(gmac_desc_t [count]),
		    BUS_DMASYNC_PREWRITE);
		hwq->hwq_wptr += count;
		hwq->hwq_wptr &= (hwq->hwq_size - 1);
	}
#endif

	/*
	 * Tell the h/w we've produced a few more descriptors.
	 * (don't bother writing the rptr since it's RO).
	 */
	bus_space_write_4(hwq->hwq_iot, hwq->hwq_qrwptr_ioh, 0,
	    hwq->hwq_wptr << 16);

	aprint_debug(" new=%u\n", hwq->hwq_wptr);
}

size_t
gmac_rxproduce(gmac_hwqueue_t *hwq, size_t free_min)
{
	gmac_hwqmem_t * const hqm = hwq->hwq_hqm;
	size_t i;

	aprint_debug("gmac_rxproduce(%p): entry free=%u(%u) free_min=%zu ifq_len=%d\n",
	    hwq, hwq->hwq_free, hwq->hwq_size - hwq->hwq_free - 1,
	    free_min, hwq->hwq_ifq.ifq_len);

	gmac_hwqueue_sync(hwq);

	aprint_debug("gmac_rxproduce(%p): postsync free=%u(%u)\n",
	    hwq, hwq->hwq_free, hwq->hwq_size - hwq->hwq_free - 1);

	for (i = 0; hwq->hwq_free > 0 && hwq->hwq_size - hwq->hwq_free - 1 < free_min; i++) {
		bus_dmamap_t map;
		gmac_desc_t * const d = gmac_hwqueue_desc(hwq, 0);
		struct mbuf *m, *m0;
		int error;

		if (d->d_bufaddr && (le32toh(d->d_bufaddr) >> 16) != 0xdead) {
			gmac_hwqueue_produce(hwq, 1);
			continue;
		}

		map = gmac_mapcache_get(hqm->hqm_mc);
		if (map == NULL)
			break;

		KASSERT(map->dm_mapsize == 0);

		m = m_gethdr(MT_DATA, M_DONTWAIT);
		if (m == NULL) {
			gmac_mapcache_put(hqm->hqm_mc, map);
			break;
		}

		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			gmac_mapcache_put(hqm->hqm_mc, map);
			break;
		}
		error = bus_dmamap_load(hqm->hqm_dmat, map, m->m_data,
		    MCLBYTES, NULL, BUS_DMA_READ|BUS_DMA_NOWAIT);
		if (error) {
			m_free(m);
			gmac_mapcache_put(hqm->hqm_mc, map);
			aprint_error("gmac0: "
			    "map %p(%zu): can't map rx mbuf(%p) wptr=%u: %d\n",
			    map, map->_dm_size, m, hwq->hwq_wptr, error);
			Debugger();
			break;
		}
		bus_dmamap_sync(hqm->hqm_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_PREREAD);
		m->m_pkthdr.len = 0;
		M_SETCTX(m, map);
#if 0
		d->d_desc0   = htole32(map->dm_segs->ds_len);
#endif
		d->d_bufaddr = htole32(map->dm_segs->ds_addr);
		for (m0 = hwq->hwq_ifq.ifq_head; m0 != NULL; m0 = m0->m_nextpkt)
			KASSERT(m0 != m);
		IF_ENQUEUE(&hwq->hwq_ifq, m);
		m->m_len = d - hwq->hwq_base;
		aprint_debug(
		    "gmac_rxproduce(%p): m=%p %zu@%p=%#x/%#x/%#x/%#x\n", hwq,
		    m, d - hwq->hwq_base, d, d->d_desc0, d->d_desc1,
		    d->d_bufaddr, d->d_desc3);
		gmac_hwqueue_produce(hwq, 1);
	}

	aprint_debug("gmac_rxproduce(%p): exit free=%u(%u) free_min=%zu ifq_len=%d\n",
	    hwq, hwq->hwq_free, hwq->hwq_size - hwq->hwq_free - 1,
	    free_min, hwq->hwq_ifq.ifq_len);

	return i;
}

static bool
gmac_hwqueue_rxconsume(gmac_hwqueue_t *hwq, const gmac_desc_t *d)
{
	gmac_hwqmem_t * const hqm = hwq->hwq_hqm;
	struct ifnet * const ifp = hwq->hwq_ifp;
	size_t buflen = d->d_desc1 & 0xffff;
	bus_dmamap_t map;
	struct mbuf *m, *last_m, **mp;
	size_t depth;

	KASSERT(ifp != NULL);

	aprint_debug("gmac_hwqueue_rxconsume(%p): entry\n", hwq);

	aprint_debug("gmac_hwqueue_rxconsume(%p): ifp=%p(%s): %#x/%#x/%#x/%#x\n",
	    hwq, hwq->hwq_ifp, hwq->hwq_ifp->if_xname,
	    d->d_desc0, d->d_desc1, d->d_bufaddr, d->d_desc3);

	if (d->d_bufaddr == 0 || d->d_bufaddr == 0xdeadbeef)
		return false;

	/*
	 * First we have to find this mbuf in the software free queue
	 * (the producer of the mbufs) and remove it.
	 */
	KASSERT(hwq->hwq_producer->hwq_free != hwq->hwq_producer->hwq_size - 1);
	for (mp = &hwq->hwq_producer->hwq_ifq.ifq_head, last_m = NULL, depth=0;
	     (m = *mp) != NULL;
	     last_m = m, mp = &m->m_nextpkt, depth++) {
		map = M_GETCTX(m, bus_dmamap_t);
		KASSERT(map != NULL);
		KASSERT(map->dm_nsegs == 1);
		aprint_debug("gmac_hwqueue_rxconsume(%p): ifq[%zu]=%p(@%#zx) %d@swfq\n",
		    hwq, depth, m, map->dm_segs->ds_addr, m->m_len);
		if (le32toh(d->d_bufaddr) == map->dm_segs->ds_addr) {
			*mp = m->m_nextpkt;
			if (hwq->hwq_producer->hwq_ifq.ifq_tail == m)
				hwq->hwq_producer->hwq_ifq.ifq_tail = last_m;
			hwq->hwq_producer->hwq_ifq.ifq_len--;
			break;
		}
	}
	aprint_debug("gmac_hwqueue_rxconsume(%p): ifp=%p(%s) m=%p@%zu",
	    hwq, hwq->hwq_ifp, hwq->hwq_ifp->if_xname, m, depth);
	if (m)
		aprint_debug(" swfq[%d]=%#x\n", m->m_len,
		    hwq->hwq_producer->hwq_base[m->m_len].d_bufaddr);
	aprint_debug("\n");
	KASSERT(m != NULL);

	{
		struct mbuf *m0;
		for (m0 = hwq->hwq_producer->hwq_ifq.ifq_head; m0 != NULL; m0 = m0->m_nextpkt)
			KASSERT(m0 != m);
	}

	KASSERT(hwq->hwq_producer->hwq_base[m->m_len].d_bufaddr == d->d_bufaddr);
	hwq->hwq_producer->hwq_base[m->m_len].d_bufaddr = htole32(0xdead0000 | m->m_len);

	m->m_len = buflen;
	if (d->d_desc3 & DESC3_SOF) {
		KASSERT(hwq->hwq_rxmbuf == NULL);
		m->m_pkthdr.len = buflen;
		buflen += 2;	/* account for the pad */
		/* only modify m->m_data after we know mbuf is good. */
	} else {
		KASSERT(hwq->hwq_rxmbuf != NULL);
		hwq->hwq_rxmbuf->m_pkthdr.len += buflen;
	}

	map = M_GETCTX(m, bus_dmamap_t);

	/*
	 * Sync the buffer contents, unload the dmamap, and save it away.
	 */
	bus_dmamap_sync(hqm->hqm_dmat, map, 0, buflen, BUS_DMASYNC_POSTREAD);
	bus_dmamap_unload(hqm->hqm_dmat, map);
	M_SETCTX(m, NULL);
	gmac_mapcache_put(hqm->hqm_mc, map);

	/*
	 * Now we build our new packet chain by tacking this on the end.
	 */
	*hwq->hwq_mp = m;
	if ((d->d_desc3 & DESC3_EOF) == 0) {
		/*
		 * Not last frame, so make sure the next gets appended right.
		 */
		hwq->hwq_mp = &m->m_next;
		return true;
	}

#if 0
	/*
	 * We have a complete frame, let's try to deliver it.
	 */
	m->m_len -= ETHER_CRC_LEN;	/* remove the CRC from the end */
#endif

	/*
	 * Now get the whole chain.
	 */
	m = hwq->hwq_rxmbuf;
	m->m_pkthdr.rcvif = ifp;	/* set receive interface */
	ifp->if_ipackets++;
	ifp->if_ibytes += m->m_pkthdr.len;
	switch (DESC0_RXSTS_GET(d->d_desc0)) {
	case DESC0_RXSTS_GOOD:
	case DESC0_RXSTS_LONG:
		m->m_data += 2;
		KASSERT(m_length(m) == m->m_pkthdr.len);
		if (ifp->if_bpf)
			bpf_ops->bpf_mtap(ifp->if_bpf, m);
		(*ifp->if_input)(ifp, m);
		break;
	default:
		ifp->if_ierrors++;
		m_freem(m);
		break;
	}
	hwq->hwq_rxmbuf = NULL;
	hwq->hwq_mp = &hwq->hwq_rxmbuf;

	return true;
}

size_t
gmac_hwqueue_consume(gmac_hwqueue_t *hwq, size_t free_min)
{
	gmac_hwqmem_t * const hqm = hwq->hwq_hqm;
	gmac_desc_t d;
	uint32_t v;
	uint16_t rptr;
	size_t i;

	KASSERT((hqm->hqm_flags & HQM_PRODUCER) == 0);

	aprint_debug("gmac_hwqueue_consume(%p): entry\n", hwq);


	v = bus_space_read_4(hwq->hwq_iot, hwq->hwq_qrwptr_ioh, 0);
	rptr = (v >>  0) & 0xffff; 
	hwq->hwq_wptr = (v >> 16) & 0xffff; 
	KASSERT(rptr == hwq->hwq_rptr);
	if (rptr == hwq->hwq_wptr)
		return 0;

	i = 0;
	for (; rptr != hwq->hwq_wptr; rptr = (rptr + 1) & (hwq->hwq_size - 1)) {
		bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap,
		    sizeof(gmac_desc_t [hwq->hwq_qoff + rptr]),
		    sizeof(gmac_desc_t),
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		d.d_desc0   = le32toh(hwq->hwq_base[rptr].d_desc0);
		d.d_desc1   = le32toh(hwq->hwq_base[rptr].d_desc1);
		d.d_bufaddr = le32toh(hwq->hwq_base[rptr].d_bufaddr);
		d.d_desc3   = le32toh(hwq->hwq_base[rptr].d_desc3);
		hwq->hwq_base[rptr].d_desc0 = 0;
		hwq->hwq_base[rptr].d_desc1 = 0;
		hwq->hwq_base[rptr].d_bufaddr = 0xdeadbeef;
		hwq->hwq_base[rptr].d_desc3 = 0;
		bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap,
		    sizeof(gmac_desc_t [hwq->hwq_qoff + rptr]),
		    sizeof(gmac_desc_t),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		aprint_debug("gmac_hwqueue_consume(%p): rptr=%u\n",
		    hwq, rptr);
		if (!gmac_hwqueue_rxconsume(hwq, &d)) {
			rptr = (rptr + 1) & (hwq->hwq_size - 1);
			i += gmac_rxproduce(hwq->hwq_producer, free_min);
			break;
		}
	}

	/*
	 * Update hardware's copy of rptr.  (wptr is RO).
	 */
	aprint_debug("gmac_hwqueue_consume(%p): rptr old=%u new=%u wptr=%u\n",
	    hwq, hwq->hwq_rptr, rptr, hwq->hwq_wptr);
	bus_space_write_4(hwq->hwq_iot, hwq->hwq_qrwptr_ioh, 0, rptr);
	hwq->hwq_rptr = rptr;

	aprint_debug("gmac_hwqueue_consume(%p): exit\n", hwq);

	return i;
}

void
gmac_hwqmem_destroy(gmac_hwqmem_t *hqm)
{
	if (hqm->hqm_nsegs) {
		if (hqm->hqm_base) {
			if (hqm->hqm_dmamap) {
				if (hqm->hqm_dmamap->dm_mapsize) {
					bus_dmamap_unload(hqm->hqm_dmat,
					    hqm->hqm_dmamap);
				}
				bus_dmamap_destroy(hqm->hqm_dmat,
				     hqm->hqm_dmamap);
			}
			bus_dmamem_unmap(hqm->hqm_dmat, hqm->hqm_base,
			    hqm->hqm_memsize);
		}
		bus_dmamem_free(hqm->hqm_dmat, hqm->hqm_segs, hqm->hqm_nsegs);
	}

	kmem_free(hqm, sizeof(*hqm));
}

gmac_hwqmem_t *
gmac_hwqmem_create(gmac_mapcache_t *mc, size_t ndesc, size_t nqueue, int flags)
{
	gmac_hwqmem_t *hqm; 
	int error;

	KASSERT(ndesc > 0 && ndesc <= 2048);
	KASSERT((ndesc & (ndesc - 1)) == 0);

	hqm = kmem_zalloc(sizeof(*hqm), KM_SLEEP);
	if (hqm == NULL)
		return NULL;

	hqm->hqm_memsize = nqueue * sizeof(gmac_desc_t [ndesc]);
	hqm->hqm_mc = mc;
	hqm->hqm_dmat = mc->mc_dmat;
	hqm->hqm_ndesc = ndesc;
	hqm->hqm_nqueue = nqueue;
	hqm->hqm_flags = flags;

	error = bus_dmamem_alloc(hqm->hqm_dmat, hqm->hqm_memsize, 0, 0,
	    hqm->hqm_segs, 1, &hqm->hqm_nsegs, BUS_DMA_WAITOK);
	if (error) {
		KASSERT(error == 0);
		goto failed;
	}
	KASSERT(hqm->hqm_nsegs == 1);
	error = bus_dmamem_map(hqm->hqm_dmat, hqm->hqm_segs, hqm->hqm_nsegs,
	    hqm->hqm_memsize, (void **)&hqm->hqm_base, BUS_DMA_WAITOK);
	if (error) {
		KASSERT(error == 0);
		goto failed;
	}
	error = bus_dmamap_create(hqm->hqm_dmat, hqm->hqm_memsize,
	    hqm->hqm_nsegs, hqm->hqm_memsize, 0,
	    BUS_DMA_WAITOK|BUS_DMA_ALLOCNOW, &hqm->hqm_dmamap);
	if (error) {
		KASSERT(error == 0);
		goto failed;
	}
	error = bus_dmamap_load(hqm->hqm_dmat, hqm->hqm_dmamap, hqm->hqm_base,
	    hqm->hqm_memsize, NULL,
	    BUS_DMA_WAITOK|BUS_DMA_WRITE|BUS_DMA_READ|BUS_DMA_COHERENT); 
	if (error) {
		aprint_debug("gmac_hwqmem_create: ds_addr=%zu ds_len=%zu\n",
		    hqm->hqm_segs->ds_addr, hqm->hqm_segs->ds_len);
		aprint_debug("gmac_hwqmem_create: bus_dmamap_load: %d\n", error);
		KASSERT(error == 0);
		goto failed;
	}

	memset(hqm->hqm_base, 0, hqm->hqm_memsize);
	if ((flags & HQM_PRODUCER) == 0)
		bus_dmamap_sync(hqm->hqm_dmat, hqm->hqm_dmamap, 0,
		    hqm->hqm_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	return hqm;

failed:
	gmac_hwqmem_destroy(hqm);
	return NULL;
}

void
gmac_hwqueue_destroy(gmac_hwqueue_t *hwq)
{
	gmac_hwqmem_t * const hqm = hwq->hwq_hqm;
	KASSERT(hqm->hqm_refs & hwq->hwq_ref);
	hqm->hqm_refs &= ~hwq->hwq_ref;
	for (;;) {
		struct mbuf *m;
		bus_dmamap_t map;
		IF_DEQUEUE(&hwq->hwq_ifq, m);
		if (m == NULL)
			break;
		map = M_GETCTX(m, bus_dmamap_t);
		bus_dmamap_unload(hqm->hqm_dmat, map);
		gmac_mapcache_put(hqm->hqm_mc, map);
		m_freem(m);
	}
	kmem_free(hwq, sizeof(*hwq));
}

gmac_hwqueue_t *
gmac_hwqueue_create(gmac_hwqmem_t *hqm,
    bus_space_tag_t iot, bus_space_handle_t ioh,
    bus_size_t qrwptr, bus_size_t qbase,
    size_t qno)
{
	const size_t log2_memsize = ffs(hqm->hqm_ndesc) - 1;
	gmac_hwqueue_t *hwq;
	uint32_t v;

	KASSERT(qno < hqm->hqm_nqueue);
	KASSERT((hqm->hqm_refs & (1 << qno)) == 0);

	hwq = kmem_zalloc(sizeof(*hwq), KM_SLEEP);
	if (hwq == NULL)
		return NULL;

	hwq->hwq_size = hqm->hqm_ndesc;

	hwq->hwq_iot = iot;
	bus_space_subregion(iot, ioh, qrwptr, sizeof(uint32_t),
	    &hwq->hwq_qrwptr_ioh);

	hwq->hwq_hqm = hqm;
	hwq->hwq_ref = 1 << qno;
	hqm->hqm_refs |= hwq->hwq_ref;
	hwq->hwq_qoff = hqm->hqm_ndesc * qno;
	hwq->hwq_base = hqm->hqm_base + hwq->hwq_qoff;

	if (qno == 0) {
		bus_space_write_4(hwq->hwq_iot, ioh, qbase,
		     hqm->hqm_dmamap->dm_segs[0].ds_addr | (log2_memsize));
	}

	v = bus_space_read_4(hwq->hwq_iot, hwq->hwq_qrwptr_ioh, 0);
	hwq->hwq_rptr = (v >>  0) & 0xffff;
	hwq->hwq_wptr = (v >> 16) & 0xffff;

	aprint_debug("gmac_hwqueue_create: %p: qrwptr=%zu(%#zx) wptr=%u rptr=%u"
	    " base=%p@%#zx(%#x) qno=%zu\n",
	    hwq, qrwptr, hwq->hwq_qrwptr_ioh, hwq->hwq_wptr, hwq->hwq_rptr,
	    hwq->hwq_base,
	    hqm->hqm_segs->ds_addr + sizeof(gmac_desc_t [hwq->hwq_qoff]),
	    bus_space_read_4(hwq->hwq_iot, ioh, qbase), qno);

	hwq->hwq_free = hwq->hwq_size - 1;
	hwq->hwq_ifq.ifq_maxlen = hwq->hwq_free;
	hwq->hwq_mp = &hwq->hwq_rxmbuf;

	return hwq;
}
