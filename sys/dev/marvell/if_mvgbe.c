/*	$NetBSD: if_mvgbe.c,v 1.2.2.3 2010/10/09 03:32:08 yamt Exp $	*/
/*
 * Copyright (c) 2007, 2008 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mvgbe.c,v 1.2.2.3 2010/10/09 03:32:08 yamt Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/sockio.h>

#include <dev/marvell/marvellreg.h>
#include <dev/marvell/marvellvar.h>
#include <dev/marvell/mvgbereg.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <net/bpf.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include "locators.h"

/* #define MVGBE_DEBUG 3 */
#ifdef MVGBE_DEBUG
#define DPRINTF(x)	if (mvgbe_debug) printf x
#define DPRINTFN(n,x)	if (mvgbe_debug >= (n)) printf x
int mvgbe_debug = MVGBE_DEBUG;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif


#define MVGBE_READ(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define MVGBE_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define MVGBE_READ_FILTER(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_dafh, (reg))
#define MVGBE_WRITE_FILTER(sc, reg, val, c) \
	bus_space_set_region_4((sc)->sc_iot, (sc)->sc_dafh, (reg), (val), (c))

#define MVGBE_TX_RING_CNT	256
#define MVGBE_RX_RING_CNT	256

#define MVGBE_JSLOTS		384	/* XXXX */
#define MVGBE_JLEN		(MVGBE_MRU + MVGBE_BUF_ALIGN)
#define MVGBE_NTXSEG		30
#define MVGBE_JPAGESZ		PAGE_SIZE
#define MVGBE_RESID \
    (MVGBE_JPAGESZ - (MVGBE_JLEN * MVGBE_JSLOTS) % MVGBE_JPAGESZ)
#define MVGBE_JMEM \
    ((MVGBE_JLEN * MVGBE_JSLOTS) + MVGBE_RESID)

#define MVGBE_TX_RING_ADDR(sc, i)		\
    ((sc)->sc_ring_map->dm_segs[0].ds_addr +	\
			offsetof(struct mvgbe_ring_data, mvgbe_tx_ring[(i)]))

#define MVGBE_RX_RING_ADDR(sc, i)		\
    ((sc)->sc_ring_map->dm_segs[0].ds_addr +	\
			offsetof(struct mvgbe_ring_data, mvgbe_rx_ring[(i)]))

#define MVGBE_CDOFF(x)		offsetof(struct mvgbe_ring_data, x)
#define MVGBE_CDTXOFF(x)	MVGBE_CDOFF(mvgbe_tx_ring[(x)])
#define MVGBE_CDRXOFF(x)	MVGBE_CDOFF(mvgbe_rx_ring[(x)])

#define MVGBE_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
	const int __descsize = sizeof(struct mvgbe_tx_desc);		\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > MVGBE_TX_RING_CNT) {				\
		bus_dmamap_sync((sc)->sc_dmat,				\
		    (sc)->sc_ring_map, MVGBE_CDTXOFF(__x),		\
		    __descsize * (MVGBE_TX_RING_CNT - __x), (ops));	\
		__n -= (MVGBE_TX_RING_CNT - __x);			\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_ring_map,		\
	    MVGBE_CDTXOFF((__x)), __descsize * __n, (ops));		\
} while (0 /*CONSTCOND*/)

#define MVGBE_CDRXSYNC(sc, x, ops)					\
do {									\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_ring_map,		\
	    MVGBE_CDRXOFF((x)), sizeof(struct mvgbe_rx_desc), (ops));	\
	} while (/*CONSTCOND*/0)


struct mvgbe_jpool_entry {
	int slot;
	LIST_ENTRY(mvgbe_jpool_entry) jpool_entries;
};

struct mvgbe_chain {
	void *mvgbe_desc;
	struct mbuf *mvgbe_mbuf;
	struct mvgbe_chain *mvgbe_next;
};

struct mvgbe_txmap_entry {
	bus_dmamap_t dmamap;
	SIMPLEQ_ENTRY(mvgbe_txmap_entry) link;
};

struct mvgbe_chain_data {
	struct mvgbe_chain mvgbe_tx_chain[MVGBE_TX_RING_CNT];
	struct mvgbe_txmap_entry *mvgbe_tx_map[MVGBE_TX_RING_CNT];
	int mvgbe_tx_prod;
	int mvgbe_tx_cons;
	int mvgbe_tx_cnt;

	struct mvgbe_chain mvgbe_rx_chain[MVGBE_RX_RING_CNT];
	bus_dmamap_t mvgbe_rx_map[MVGBE_RX_RING_CNT];
	bus_dmamap_t mvgbe_rx_jumbo_map;
	int mvgbe_rx_prod;
	int mvgbe_rx_cons;
	int mvgbe_rx_cnt;

	/* Stick the jumbo mem management stuff here too. */
	void *mvgbe_jslots[MVGBE_JSLOTS];
	void *mvgbe_jumbo_buf;
};

struct mvgbe_ring_data {
	struct mvgbe_tx_desc mvgbe_tx_ring[MVGBE_TX_RING_CNT];
	struct mvgbe_rx_desc mvgbe_rx_ring[MVGBE_RX_RING_CNT];
};

struct mvgbec_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	kmutex_t sc_mtx;

	int sc_fix_tqtb;
};

struct mvgbe_softc {
	device_t sc_dev;
	int sc_port;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_dafh;		/* dest address filter handle */
	bus_dma_tag_t sc_dmat;

	struct ethercom sc_ethercom;
	struct mii_data sc_mii;
	u_int8_t sc_enaddr[ETHER_ADDR_LEN];	/* station addr */

	struct mvgbe_chain_data sc_cdata;
	struct mvgbe_ring_data *sc_rdata;
	bus_dmamap_t sc_ring_map;
	int sc_if_flags;

	LIST_HEAD(__mvgbe_jfreehead, mvgbe_jpool_entry) sc_jfree_listhead;
	LIST_HEAD(__mvgbe_jinusehead, mvgbe_jpool_entry) sc_jinuse_listhead;
	SIMPLEQ_HEAD(__mvgbe_txmaphead, mvgbe_txmap_entry) sc_txmap_head;

#if NRND > 0
	rndsource_element_t sc_rnd_source;
#endif
};


/* Gigabit Ethernet Unit Global part functions */

static int mvgbec_match(device_t, struct cfdata *, void *);
static void mvgbec_attach(device_t, device_t, void *);

static int mvgbec_print(void *, const char *);
static int mvgbec_search(device_t, cfdata_t, const int *, void *);

/* MII funcstions */
static int mvgbec_miibus_readreg(device_t, int, int);
static void mvgbec_miibus_writereg(device_t, int, int, int);
static void mvgbec_miibus_statchg(device_t);

static void mvgbec_wininit(struct mvgbec_softc *);

/* Gigabit Ethernet Port part functions */

static int mvgbe_match(device_t, struct cfdata *, void *);
static void mvgbe_attach(device_t, device_t, void *);

static int mvgbe_intr(void *);

static void mvgbe_start(struct ifnet *);
static int mvgbe_ioctl(struct ifnet *, u_long, void *);
static int mvgbe_init(struct ifnet *);
static void mvgbe_stop(struct ifnet *, int);
static void mvgbe_watchdog(struct ifnet *);

/* MII funcstions */
static int mvgbe_ifmedia_upd(struct ifnet *);
static void mvgbe_ifmedia_sts(struct ifnet *, struct ifmediareq *);

static int mvgbe_init_rx_ring(struct mvgbe_softc *);
static int mvgbe_init_tx_ring(struct mvgbe_softc *);
static int mvgbe_newbuf(struct mvgbe_softc *, int, struct mbuf *, bus_dmamap_t);
static int mvgbe_alloc_jumbo_mem(struct mvgbe_softc *);
static void *mvgbe_jalloc(struct mvgbe_softc *);
static void mvgbe_jfree(struct mbuf *, void *, size_t, void *);
static int mvgbe_encap(struct mvgbe_softc *, struct mbuf *, uint32_t *);
static void mvgbe_rxeof(struct mvgbe_softc *);
static void mvgbe_txeof(struct mvgbe_softc *);
static void mvgbe_setmulti(struct mvgbe_softc *);
#ifdef MVGBE_DEBUG
static void mvgbe_dump_txdesc(struct mvgbe_tx_desc *, int);
#endif

CFATTACH_DECL_NEW(mvgbec_gt, sizeof(struct mvgbec_softc),
    mvgbec_match, mvgbec_attach, NULL, NULL);
CFATTACH_DECL_NEW(mvgbec_mbus, sizeof(struct mvgbec_softc),
    mvgbec_match, mvgbec_attach, NULL, NULL);

CFATTACH_DECL_NEW(mvgbe, sizeof(struct mvgbe_softc),
    mvgbe_match, mvgbe_attach, NULL, NULL);


struct mvgbe_port {
	int model;
	int unit;
	int ports;
	int irqs[3];
	int flags;
#define FLAGS_FIX_TQTB	(1 << 0)
} mvgbe_ports[] = {
	{ MARVELL_DISCOVERY_II,		0, 3, { 32, 33, 34 }, 0 },
	{ MARVELL_DISCOVERY_III,	0, 3, { 32, 33, 34 }, 0 },
#if 0
	{ MARVELL_DISCOVERY_LT,		0, ?, { }, 0 },
	{ MARVELL_DISCOVERY_V,		0, ?, { }, 0 },
	{ MARVELL_DISCOVERY_VI,		0, ?, { }, 0 },
#endif
	{ MARVELL_ORION_1_88F5082,	0, 1, { 21 }, 0 },
	{ MARVELL_ORION_1_88F5180N,	0, 1, { 21 }, 0 },
	{ MARVELL_ORION_1_88F5181,	0, 1, { 21 }, 0 },
	{ MARVELL_ORION_1_88F5182,	0, 1, { 21 }, 0 },
	{ MARVELL_ORION_2_88F5281,	0, 1, { 21 }, 0 },
	{ MARVELL_ORION_1_88F6082,	0, 1, { 21 }, 0 },
	{ MARVELL_ORION_1_88W8660,	0, 1, { 21 }, 0 },

	{ MARVELL_KIRKWOOD_88F6180,	0, 1, { 11 }, FLAGS_FIX_TQTB },
	{ MARVELL_KIRKWOOD_88F6192,	0, 1, { 11 }, FLAGS_FIX_TQTB },
	{ MARVELL_KIRKWOOD_88F6192,	1, 1, { 14 }, FLAGS_FIX_TQTB },
	{ MARVELL_KIRKWOOD_88F6281,	0, 1, { 11 }, FLAGS_FIX_TQTB },
	{ MARVELL_KIRKWOOD_88F6281,	1, 1, { 14 }, FLAGS_FIX_TQTB },

	{ MARVELL_MV78XX0_MV78100,	0, 1, { 40 }, FLAGS_FIX_TQTB },
	{ MARVELL_MV78XX0_MV78100,	1, 1, { 44 }, FLAGS_FIX_TQTB },
	{ MARVELL_MV78XX0_MV78200,	0, 1, { 40 }, FLAGS_FIX_TQTB },
	{ MARVELL_MV78XX0_MV78200,	1, 1, { 44 }, FLAGS_FIX_TQTB },
	{ MARVELL_MV78XX0_MV78200,	2, 1, { 48 }, FLAGS_FIX_TQTB },
	{ MARVELL_MV78XX0_MV78200,	3, 1, { 52 }, FLAGS_FIX_TQTB },
};


/* ARGSUSED */
static int
mvgbec_match(device_t parent, struct cfdata *match, void *aux)
{
	struct marvell_attach_args *mva = aux;
	int i;

	if (strcmp(mva->mva_name, match->cf_name) != 0)
		return 0;
	if (mva->mva_offset == MVA_OFFSET_DEFAULT)
		return 0;

	for (i = 0; i < __arraycount(mvgbe_ports); i++)
		if (mva->mva_model == mvgbe_ports[i].model) {
			mva->mva_size = MVGBE_SIZE;
			return 1;
		}
	return 0;
}

/* ARGSUSED */
static void
mvgbec_attach(device_t parent, device_t self, void *aux)
{
	struct mvgbec_softc *sc = device_private(self);
	struct marvell_attach_args *mva = aux, gbea;
	struct mvgbe_softc *port;
	struct mii_softc *mii;
	device_t child;
	uint32_t phyaddr;
	int i, j;

	aprint_naive("\n");
	aprint_normal(": Marvell Gigabit Ethernet Controller\n");

	sc->sc_dev = self;
	sc->sc_iot = mva->mva_iot;
	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh, mva->mva_offset,
	    mva->mva_size, &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}
	phyaddr = 0;
	MVGBE_WRITE(sc, MVGBE_PHYADDR, phyaddr);

	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NET);

	/* Disable and clear Gigabit Ethernet Unit interrupts */
	MVGBE_WRITE(sc, MVGBE_EUIM, 0);
	MVGBE_WRITE(sc, MVGBE_EUIC, 0);

	mvgbec_wininit(sc);

	memset(&gbea, 0, sizeof(gbea));
	for (i = 0; i < __arraycount(mvgbe_ports); i++) {
		if (mvgbe_ports[i].model != mva->mva_model ||
		    mvgbe_ports[i].unit != mva->mva_unit)
			continue;

		sc->sc_fix_tqtb = mvgbe_ports[i].flags & FLAGS_FIX_TQTB;

		for (j = 0; j < mvgbe_ports[i].ports; j++) {
			gbea.mva_name = "mvgbe";
			gbea.mva_model = mva->mva_model;
			gbea.mva_iot = sc->sc_iot;
			gbea.mva_ioh = sc->sc_ioh;
			gbea.mva_unit = j;
			gbea.mva_dmat = mva->mva_dmat;
			gbea.mva_irq = mvgbe_ports[i].irqs[j];
			child = config_found_sm_loc(sc->sc_dev, "mvgbec", NULL,
			    &gbea, mvgbec_print, mvgbec_search);
			if (child) {
				port = device_private(child);
				mii  = LIST_FIRST(&port->sc_mii.mii_phys);
				phyaddr |= MVGBE_PHYADDR_PHYAD(j, mii->mii_phy);
			}
		}
		break;
	}
	MVGBE_WRITE(sc, MVGBE_PHYADDR, phyaddr);
}

static int
mvgbec_print(void *aux, const char *pnp)
{
	struct marvell_attach_args *gbea = aux;

	if (pnp)
		aprint_normal("%s at %s port %d",
		    gbea->mva_name, pnp, gbea->mva_unit);
	else {
		if (gbea->mva_unit != MVGBECCF_PORT_DEFAULT)
			aprint_normal(" port %d", gbea->mva_unit);
		if (gbea->mva_irq != MVGBECCF_IRQ_DEFAULT)
			aprint_normal(" irq %d", gbea->mva_irq);
	}
	return UNCONF;
}

/* ARGSUSED */
static int
mvgbec_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct marvell_attach_args *gbea = aux;

	if (cf->cf_loc[MVGBECCF_PORT] == gbea->mva_unit &&
	    cf->cf_loc[MVGBECCF_IRQ] != MVGBECCF_IRQ_DEFAULT)
		gbea->mva_irq = cf->cf_loc[MVGBECCF_IRQ];

	return config_match(parent, cf, aux);
}

static int
mvgbec_miibus_readreg(device_t dev, int phy, int reg)
{
	struct mvgbe_softc *sc = device_private(dev);
	struct mvgbec_softc *csc = device_private(device_parent(dev));
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t smi, val;
	int i;

	mutex_enter(&csc->sc_mtx);

	for (i = 0; i < MVGBE_PHY_TIMEOUT; i++) {
		DELAY(1);
		if (!(MVGBE_READ(csc, MVGBE_SMI) & MVGBE_SMI_BUSY))
			break;
	}
	if (i == MVGBE_PHY_TIMEOUT) {
		aprint_error_ifnet(ifp, "SMI busy timeout\n");
		mutex_exit(&csc->sc_mtx);
		return -1;
	}

	smi =
	    MVGBE_SMI_PHYAD(phy) | MVGBE_SMI_REGAD(reg) | MVGBE_SMI_OPCODE_READ;
	MVGBE_WRITE(csc, MVGBE_SMI, smi);

	for (i = 0; i < MVGBE_PHY_TIMEOUT; i++) {
		DELAY(1);
		smi = MVGBE_READ(csc, MVGBE_SMI);
		if (smi & MVGBE_SMI_READVALID)
			break;
	}

	mutex_exit(&csc->sc_mtx);

	DPRINTFN(9, ("mvgbec_miibus_readreg: i=%d, timeout=%d\n",
	    i, MVGBE_PHY_TIMEOUT));

	val = smi & MVGBE_SMI_DATA_MASK;

	DPRINTFN(9, ("mvgbec_miibus_readreg phy=%d, reg=%#x, val=%#x\n",
	    phy, reg, val));

	return val;
}

static void
mvgbec_miibus_writereg(device_t dev, int phy, int reg, int val)
{
	struct mvgbe_softc *sc = device_private(dev);
	struct mvgbec_softc *csc = device_private(device_parent(dev));
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t smi;
	int i;

	DPRINTFN(9, ("mvgbec_miibus_writereg phy=%d reg=%#x val=%#x\n",
	     phy, reg, val));

	mutex_enter(&csc->sc_mtx);

	for (i = 0; i < MVGBE_PHY_TIMEOUT; i++) {
		DELAY(1);
		if (!(MVGBE_READ(csc, MVGBE_SMI) & MVGBE_SMI_BUSY))
			break;
	}
	if (i == MVGBE_PHY_TIMEOUT) {
		aprint_error_ifnet(ifp, "SMI busy timeout\n");
		mutex_exit(&csc->sc_mtx);
		return;
	}

	smi = MVGBE_SMI_PHYAD(phy) | MVGBE_SMI_REGAD(reg) |
	    MVGBE_SMI_OPCODE_WRITE | (val & MVGBE_SMI_DATA_MASK);
	MVGBE_WRITE(csc, MVGBE_SMI, smi);

	for (i = 0; i < MVGBE_PHY_TIMEOUT; i++) {
		DELAY(1);
		if (!(MVGBE_READ(csc, MVGBE_SMI) & MVGBE_SMI_BUSY))
			break;
	}

	mutex_exit(&csc->sc_mtx);

	if (i == MVGBE_PHY_TIMEOUT)
		aprint_error_ifnet(ifp, "phy write timed out\n");
}

static void
mvgbec_miibus_statchg(device_t dev)
{

	/* nothing to do */
}


static void
mvgbec_wininit(struct mvgbec_softc *sc)
{
	device_t pdev = device_parent(sc->sc_dev);
	uint64_t base;
	uint32_t en, ac, size;
	int window, target, attr, rv, i;
	static int tags[] = {
		MARVELL_TAG_SDRAM_CS0,
		MARVELL_TAG_SDRAM_CS1,
		MARVELL_TAG_SDRAM_CS2,
		MARVELL_TAG_SDRAM_CS3,

		MARVELL_TAG_UNDEFINED,
	};

	/* First disable all address decode windows */
	en = MVGBE_BARE_EN_MASK;
	MVGBE_WRITE(sc, MVGBE_BARE, en);

	ac = 0;
	for (window = 0, i = 0;
	    tags[i] != MARVELL_TAG_UNDEFINED && window < MVGBE_NWINDOW; i++) {
		rv = marvell_winparams_by_tag(pdev, tags[i],
		    &target, &attr, &base, &size);
		if (rv != 0 || size == 0)
			continue;

		if (base > 0xffffffffULL) {
			if (window >= MVGBE_NREMAP) {
				aprint_error_dev(sc->sc_dev,
				    "can't remap window %d\n", window);
				continue;
			}
			MVGBE_WRITE(sc, MVGBE_HA(window),
			    (base >> 32) & 0xffffffff);
		}

		MVGBE_WRITE(sc, MVGBE_BASEADDR(window),
		    MVGBE_BASEADDR_TARGET(target)	|
		    MVGBE_BASEADDR_ATTR(attr)		|
		    MVGBE_BASEADDR_BASE(base));
		MVGBE_WRITE(sc, MVGBE_S(window), MVGBE_S_SIZE(size));

		en &= ~(1 << window);
		/* set full access (r/w) */
		ac |= MVGBE_EPAP_EPAR(window, MVGBE_EPAP_AC_FA);
		window++;
	}
	/* allow to access decode window */
	MVGBE_WRITE(sc, MVGBE_EPAP, ac);

	MVGBE_WRITE(sc, MVGBE_BARE, en);
}


/* ARGSUSED */
static int
mvgbe_match(device_t parent, struct cfdata *match, void *aux)
{
	struct marvell_attach_args *mva = aux;
	uint32_t pbase, maddrh, maddrl;

	pbase = MVGBE_PORTR_BASE + mva->mva_unit * MVGBE_PORTR_SIZE;
	maddrh =
	    bus_space_read_4(mva->mva_iot, mva->mva_ioh, pbase + MVGBE_MACAH);
	maddrl =
	    bus_space_read_4(mva->mva_iot, mva->mva_ioh, pbase + MVGBE_MACAL);
	if ((maddrh | maddrl) == 0)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
mvgbe_attach(device_t parent, device_t self, void *aux)
{
	struct mvgbe_softc *sc = device_private(self);
	struct marvell_attach_args *mva = aux;
	struct mvgbe_txmap_entry *entry;
	struct ifnet *ifp;
	bus_dma_segment_t seg;
	bus_dmamap_t dmamap;
	int rseg, i;
	uint32_t maddrh, maddrl;
	void *kva;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_port = mva->mva_unit;
	sc->sc_iot = mva->mva_iot;
	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh,
	    MVGBE_PORTR_BASE + mva->mva_unit * MVGBE_PORTR_SIZE,
	    MVGBE_PORTR_SIZE, &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}
	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh,
	    MVGBE_PORTDAFR_BASE + mva->mva_unit * MVGBE_PORTDAFR_SIZE,
	    MVGBE_PORTDAFR_SIZE, &sc->sc_dafh)) {
		aprint_error_dev(self,
		    "Cannot map destination address filter registers\n");
		return;
	}
	sc->sc_dmat = mva->mva_dmat;

	maddrh = MVGBE_READ(sc, MVGBE_MACAH);
	maddrl = MVGBE_READ(sc, MVGBE_MACAL);
	sc->sc_enaddr[0] = maddrh >> 24;
	sc->sc_enaddr[1] = maddrh >> 16;
	sc->sc_enaddr[2] = maddrh >> 8;
	sc->sc_enaddr[3] = maddrh >> 0;
	sc->sc_enaddr[4] = maddrl >> 8;
	sc->sc_enaddr[5] = maddrl >> 0;
	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	/* clear all ethernet port interrupts */
	MVGBE_WRITE(sc, MVGBE_IC, 0);
	MVGBE_WRITE(sc, MVGBE_ICE, 0);

	marvell_intr_establish(mva->mva_irq, IPL_NET, mvgbe_intr, sc);

	/* Allocate the descriptor queues. */
	if (bus_dmamem_alloc(sc->sc_dmat, sizeof(struct mvgbe_ring_data),
	    PAGE_SIZE, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		aprint_error_dev(self, "can't alloc rx buffers\n");
		return;
	}
	if (bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    sizeof(struct mvgbe_ring_data), &kva, BUS_DMA_NOWAIT)) {
		aprint_error_dev(self, "can't map dma buffers (%lu bytes)\n",
		    (u_long)sizeof(struct mvgbe_ring_data));
		goto fail1;
	}
	if (bus_dmamap_create(sc->sc_dmat, sizeof(struct mvgbe_ring_data), 1,
	    sizeof(struct mvgbe_ring_data), 0, BUS_DMA_NOWAIT,
	    &sc->sc_ring_map)) {
		aprint_error_dev(self, "can't create dma map\n");
		goto fail2;
	}
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_ring_map, kva,
	    sizeof(struct mvgbe_ring_data), NULL, BUS_DMA_NOWAIT)) {
		aprint_error_dev(self, "can't load dma map\n");
		goto fail3;
	}
	for (i = 0; i < MVGBE_RX_RING_CNT; i++)
		sc->sc_cdata.mvgbe_rx_chain[i].mvgbe_mbuf = NULL;

	SIMPLEQ_INIT(&sc->sc_txmap_head);
	for (i = 0; i < MVGBE_TX_RING_CNT; i++) {
		sc->sc_cdata.mvgbe_tx_chain[i].mvgbe_mbuf = NULL;

		if (bus_dmamap_create(sc->sc_dmat,
		    MVGBE_JLEN, MVGBE_NTXSEG, MVGBE_JLEN, 0,
		    BUS_DMA_NOWAIT, &dmamap)) {
			aprint_error_dev(self, "Can't create TX dmamap\n");
			goto fail4;
		}

		entry = kmem_alloc(sizeof(*entry), KM_SLEEP);
		if (!entry) {
			aprint_error_dev(self, "Can't alloc txmap entry\n");
			bus_dmamap_destroy(sc->sc_dmat, dmamap);
			goto fail4;
		}
		entry->dmamap = dmamap;
		SIMPLEQ_INSERT_HEAD(&sc->sc_txmap_head, entry, link);
	}

	sc->sc_rdata = (struct mvgbe_ring_data *)kva;
	memset(sc->sc_rdata, 0, sizeof(struct mvgbe_ring_data));

#if 0
	/*
	 * We can support 802.1Q VLAN-sized frames and jumbo
	 * Ethernet frames.
	 */
	sc->sc_ethercom.ec_capabilities |=
	    ETHERCAP_VLAN_MTU | ETHERCAP_VLAN_HWTAGGING | ETHERCAP_JUMBO_MTU;
#else
	/* XXXX: We don't know the usage of VLAN. */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_JUMBO_MTU;
#endif

	/* Try to allocate memory for jumbo buffers. */
	if (mvgbe_alloc_jumbo_mem(sc)) {
		aprint_error_dev(self, "jumbo buffer allocation failed\n");
		goto fail4;
	}

	ifp = &sc->sc_ethercom.ec_if;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = mvgbe_start;
	ifp->if_ioctl = mvgbe_ioctl;
	ifp->if_init = mvgbe_init;
	ifp->if_stop = mvgbe_stop;
	ifp->if_watchdog = mvgbe_watchdog;
	/*
	 * We can do IPv4/TCPv4/UDPv4 checksums in hardware.
	 */
	sc->sc_ethercom.ec_if.if_capabilities |=
	    IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
	    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx;
	IFQ_SET_MAXLEN(&ifp->if_snd, max(MVGBE_TX_RING_CNT - 1, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);
	strcpy(ifp->if_xname, device_xname(sc->sc_dev));

	mvgbe_stop(ifp, 0);

	/*
	 * Do MII setup.
	 */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = mvgbec_miibus_readreg;
	sc->sc_mii.mii_writereg = mvgbec_miibus_writereg;
	sc->sc_mii.mii_statchg = mvgbec_miibus_statchg;

	sc->sc_ethercom.ec_mii = &sc->sc_mii;
	ifmedia_init(&sc->sc_mii.mii_media, 0,
	    mvgbe_ifmedia_upd, mvgbe_ifmedia_sts);
	mii_attach(self, &sc->sc_mii, 0xffffffff,
	    MII_PHY_ANY, MII_OFFSET_ANY, 0);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		aprint_error_dev(self, "no PHY found!\n");
		ifmedia_add(&sc->sc_mii.mii_media,
		    IFM_ETHER|IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);

	ether_ifattach(ifp, sc->sc_enaddr);

#if NRND > 0
	rnd_attach_source(&sc->sc_rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, 0);
#endif

	return;

fail4:
	while ((entry = SIMPLEQ_FIRST(&sc->sc_txmap_head)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_txmap_head, link);
		bus_dmamap_destroy(sc->sc_dmat, entry->dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_ring_map);
fail3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_ring_map);
fail2:
	bus_dmamem_unmap(sc->sc_dmat, kva, sizeof(struct mvgbe_ring_data));
fail1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
	return;
}


static int
mvgbe_intr(void *arg)
{
	struct mvgbe_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t ic, ice, datum = 0;
	int claimed = 0;

	for (;;) {
		ice = MVGBE_READ(sc, MVGBE_ICE);
		ic = MVGBE_READ(sc, MVGBE_IC);

		DPRINTFN(3, ("mvgbe_intr: ic=%#x, ice=%#x\n", ic, ice));
		if (ic == 0 && ice == 0)
			break;

		datum = datum ^ ic ^ ice;

		MVGBE_WRITE(sc, MVGBE_IC, ~ic);
		MVGBE_WRITE(sc, MVGBE_ICE, ~ice);

		claimed = 1;

		if (ice & MVGBE_ICE_LINKCHG) {
			if (MVGBE_READ(sc, MVGBE_PS) & MVGBE_PS_LINKUP) {
				/* Enable port RX and TX. */
				MVGBE_WRITE(sc, MVGBE_RQC, MVGBE_RQC_ENQ(0));
				MVGBE_WRITE(sc, MVGBE_TQC, MVGBE_TQC_ENQ);
			} else {
				MVGBE_WRITE(sc, MVGBE_RQC, MVGBE_RQC_DISQ(0));
				MVGBE_WRITE(sc, MVGBE_TQC, MVGBE_TQC_DISQ);
			}
		}

		if (ic & (MVGBE_IC_RXBUF | MVGBE_IC_RXERROR))
			mvgbe_rxeof(sc);

		if (ice & (MVGBE_ICE_TXBUF | MVGBE_ICE_TXERR))
			mvgbe_txeof(sc);
	}

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		mvgbe_start(ifp);

#if NRND > 0
	if (RND_ENABLED(&sc->sc_rnd_source))
		rnd_add_uint32(&sc->sc_rnd_source, datum);
#endif

	return claimed;
}

static void
mvgbe_start(struct ifnet *ifp)
{
	struct mvgbe_softc *sc = ifp->if_softc;
	struct mbuf *m_head = NULL;
	uint32_t idx = sc->sc_cdata.mvgbe_tx_prod;
	int pkts = 0;

	DPRINTFN(3, ("mvgbe_start (idx %d, tx_chain[idx] %p)\n", idx,
	    sc->sc_cdata.mvgbe_tx_chain[idx].mvgbe_mbuf));

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;
	/* If Link is DOWN, can't start TX */
	if (!(MVGBE_READ(sc, MVGBE_PS) & MVGBE_PS_LINKUP))
		return;

	while (sc->sc_cdata.mvgbe_tx_chain[idx].mvgbe_mbuf == NULL) {
		IFQ_POLL(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;

		/*
		 * Pack the data into the transmit ring. If we
		 * don't have room, set the OACTIVE flag and wait
		 * for the NIC to drain the ring.
		 */
		if (mvgbe_encap(sc, m_head, &idx)) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		/* now we are committed to transmit the packet */
		IFQ_DEQUEUE(&ifp->if_snd, m_head);
		pkts++;

		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		if (ifp->if_bpf)
			bpf_ops->bpf_mtap(ifp->if_bpf, m_head);
	}
	if (pkts == 0)
		return;

	/* Transmit at Queue 0 */
	if (idx != sc->sc_cdata.mvgbe_tx_prod) {
		sc->sc_cdata.mvgbe_tx_prod = idx;
		MVGBE_WRITE(sc, MVGBE_TQC, MVGBE_TQC_ENQ);

		/*
		 * Set a timeout in case the chip goes out to lunch.
		 */
		ifp->if_timer = 5;
	}
}

static int
mvgbe_ioctl(struct ifnet *ifp, u_long command, void *data)
{
	struct mvgbe_softc *sc = ifp->if_softc;
	struct ifreq *ifr = data;
	struct mii_data *mii;
	int s, error = 0;

	s = splnet();

	switch (command) {
	case SIOCSIFFLAGS:
		DPRINTFN(2, ("mvgbe_ioctl IFFLAGS\n"));
		if (ifp->if_flags & IFF_UP)
			mvgbe_init(ifp);
		else
			if (ifp->if_flags & IFF_RUNNING)
				mvgbe_stop(ifp, 0);
		sc->sc_if_flags = ifp->if_flags;
		error = 0;
		break;

	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		DPRINTFN(2, ("mvgbe_ioctl MEDIA\n"));
		mii = &sc->sc_mii;
		error = ifmedia_ioctl(ifp, ifr, &mii->mii_media, command);
		break;

	default:
		DPRINTFN(2, ("mvgbe_ioctl ETHER\n"));
		error = ether_ioctl(ifp, command, data);
		if (error == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING) {
				mvgbe_setmulti(sc);
				DPRINTFN(2,
				    ("mvgbe_ioctl setmulti called\n"));
			}
			error = 0;
		}
		break;
	}

	splx(s);

	return error;
}

static int
mvgbe_init(struct ifnet *ifp)
{
	struct mvgbe_softc *sc = ifp->if_softc;
	struct mvgbec_softc *csc = device_private(device_parent(sc->sc_dev));
	struct mii_data *mii = &sc->sc_mii;
	uint32_t reg;
	int i, s;

	DPRINTFN(2, ("mvgbe_init\n"));

	s = splnet();

	if (ifp->if_flags & IFF_RUNNING) {
		splx(s);
		return 0;
	}

	/* Cancel pending I/O and free all RX/TX buffers. */
	mvgbe_stop(ifp, 0);

	/* clear all ethernet port interrupts */
	MVGBE_WRITE(sc, MVGBE_IC, 0);
	MVGBE_WRITE(sc, MVGBE_ICE, 0);

	/* Init TX/RX descriptors */
	if (mvgbe_init_tx_ring(sc) == ENOBUFS) {
		aprint_error_ifnet(ifp,
		    "initialization failed: no memory for tx buffers\n");
		splx(s);
		return ENOBUFS;
	}
	if (mvgbe_init_rx_ring(sc) == ENOBUFS) {
		aprint_error_ifnet(ifp,
		    "initialization failed: no memory for rx buffers\n");
		splx(s);
		return ENOBUFS;
	}

	MVGBE_WRITE(sc, MVGBE_PSC,
	    MVGBE_PSC_ANFC |			/* Enable Auto-Neg Flow Ctrl */
	    MVGBE_PSC_RESERVED |		/* Must be set to 1 */
	    MVGBE_PSC_FLFAIL |			/* Do NOT Force Link Fail */
	    MVGBE_PSC_MRU(MVGBE_PSC_MRU_9700) |	/* Always 9700 OK */
	    MVGBE_PSC_SETFULLDX);		/* Set_FullDx */
	/* XXXX: mvgbe(4) always use RGMII. */
	MVGBE_WRITE(sc, MVGBE_PSC1,
	    MVGBE_READ(sc, MVGBE_PSC1) | MVGBE_PSC1_RGMIIEN);
	/* XXXX: Also always Weighted Round-Robin Priority Mode */
	MVGBE_WRITE(sc, MVGBE_TQFPC, MVGBE_TQFPC_EN(0));

	MVGBE_WRITE(sc, MVGBE_CRDP(0), MVGBE_RX_RING_ADDR(sc, 0));
	MVGBE_WRITE(sc, MVGBE_TCQDP, MVGBE_TX_RING_ADDR(sc, 0));

	if (csc->sc_fix_tqtb) {
		/*
		 * Queue 0 (offset 0x72700) must be programmed to 0x3fffffff.
		 * And offset 0x72704 must be programmed to 0x03ffffff.
		 * Queue 1 through 7 must be programmed to 0x0.
		 */
		MVGBE_WRITE(sc, MVGBE_TQTBCOUNT(0), 0x3fffffff);
		MVGBE_WRITE(sc, MVGBE_TQTBCONFIG(0), 0x03ffffff);
		for (i = 1; i < 8; i++) {
			MVGBE_WRITE(sc, MVGBE_TQTBCOUNT(i), 0x0);
			MVGBE_WRITE(sc, MVGBE_TQTBCONFIG(i), 0x0);
		}
	} else
		for (i = 1; i < 8; i++) {
			MVGBE_WRITE(sc, MVGBE_TQTBCOUNT(i), 0x3fffffff);
			MVGBE_WRITE(sc, MVGBE_TQTBCONFIG(i), 0xffff7fff);
			MVGBE_WRITE(sc, MVGBE_TQAC(i), 0xfc0000ff);
		}

	MVGBE_WRITE(sc, MVGBE_PXC, MVGBE_PXC_RXCS);
	MVGBE_WRITE(sc, MVGBE_PXCX, 0);
	MVGBE_WRITE(sc, MVGBE_SDC,
	    MVGBE_SDC_RXBSZ_16_64BITWORDS |
#if BYTE_ORDER == LITTLE_ENDIAN
	    MVGBE_SDC_BLMR |	/* Big/Litlle Endian Receive Mode: No swap */
	    MVGBE_SDC_BLMT |	/* Big/Litlle Endian Transmit Mode: No swap */
#endif
	    MVGBE_SDC_TXBSZ_16_64BITWORDS);

	mii_mediachg(mii);

	/* Enable port */
	reg = MVGBE_READ(sc, MVGBE_PSC);
	MVGBE_WRITE(sc, MVGBE_PSC, reg | MVGBE_PSC_PORTEN);

	/* If Link is UP, Start RX and TX traffic */
	if (MVGBE_READ(sc, MVGBE_PS) & MVGBE_PS_LINKUP) {
		/* Enable port RX/TX. */
		MVGBE_WRITE(sc, MVGBE_RQC, MVGBE_RQC_ENQ(0));
		MVGBE_WRITE(sc, MVGBE_TQC, MVGBE_TQC_ENQ);
	}

	/* Enable interrupt masks */
	MVGBE_WRITE(sc, MVGBE_PIM,
	    MVGBE_IC_RXBUF |
	    MVGBE_IC_EXTEND |
	    MVGBE_IC_RXBUFQ_MASK |
	    MVGBE_IC_RXERROR |
	    MVGBE_IC_RXERRQ_MASK);
	MVGBE_WRITE(sc, MVGBE_PEIM,
	    MVGBE_ICE_TXBUF |
	    MVGBE_ICE_TXERR |
	    MVGBE_ICE_LINKCHG);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);

	return 0;
}

/* ARGSUSED */
static void
mvgbe_stop(struct ifnet *ifp, int disable)
{
	struct mvgbe_softc *sc = ifp->if_softc;
	struct mvgbe_chain_data *cdata = &sc->sc_cdata;
	uint32_t reg;
	int i, cnt;

	DPRINTFN(2, ("mvgbe_stop\n"));

	/* Stop Rx port activity. Check port Rx activity. */
	reg = MVGBE_READ(sc, MVGBE_RQC);
	if (reg & MVGBE_RQC_ENQ_MASK)
		/* Issue stop command for active channels only */
		MVGBE_WRITE(sc, MVGBE_RQC, MVGBE_RQC_DISQ_DISABLE(reg));

	/* Stop Tx port activity. Check port Tx activity. */
	if (MVGBE_READ(sc, MVGBE_TQC) & MVGBE_TQC_ENQ)
		MVGBE_WRITE(sc, MVGBE_TQC, MVGBE_TQC_DISQ);

	/* Force link down */
	reg = MVGBE_READ(sc, MVGBE_PSC);
	MVGBE_WRITE(sc, MVGBE_PSC, reg & ~MVGBE_PSC_FLFAIL);

#define RX_DISABLE_TIMEOUT          0x1000000
#define TX_FIFO_EMPTY_TIMEOUT       0x1000000
	/* Wait for all Rx activity to terminate. */
	cnt = 0;
	do {
		if (cnt >= RX_DISABLE_TIMEOUT) {
			aprint_error_ifnet(ifp,
			    "timeout for RX stopped. rqc 0x%x\n", reg);
			break;
		}
		cnt++;

		/*
		 * Check Receive Queue Command register that all Rx queues
		 * are stopped
		 */
		reg = MVGBE_READ(sc, MVGBE_RQC);
	} while (reg & 0xff);

	/* Double check to verify that TX FIFO is empty */
	cnt = 0;
	while (1) {
		do {
			if (cnt >= TX_FIFO_EMPTY_TIMEOUT) {
				aprint_error_ifnet(ifp,
				    "timeout for TX FIFO empty. status 0x%x\n",
				    reg);
				break;
			}
			cnt++;

			reg = MVGBE_READ(sc, MVGBE_PS);
		} while
		    (!(reg & MVGBE_PS_TXFIFOEMP) || reg & MVGBE_PS_TXINPROG);

		if (cnt >= TX_FIFO_EMPTY_TIMEOUT)
			break;

		/* Double check */
		reg = MVGBE_READ(sc, MVGBE_PS);
		if (reg & MVGBE_PS_TXFIFOEMP && !(reg & MVGBE_PS_TXINPROG))
			break;
		else
			aprint_error_ifnet(ifp,
			    "TX FIFO empty double check failed."
			    " %d loops, status 0x%x\n", cnt, reg);
	}

	/* Reset the Enable bit in the Port Serial Control Register */
	reg = MVGBE_READ(sc, MVGBE_PSC);
	MVGBE_WRITE(sc, MVGBE_PSC, reg & ~MVGBE_PSC_PORTEN);

	/* Disable interrupts */
	MVGBE_WRITE(sc, MVGBE_PIM, 0);
	MVGBE_WRITE(sc, MVGBE_PEIM, 0);

	/* Free RX and TX mbufs still in the queues. */
	for (i = 0; i < MVGBE_RX_RING_CNT; i++) {
		if (cdata->mvgbe_rx_chain[i].mvgbe_mbuf != NULL) {
			m_freem(cdata->mvgbe_rx_chain[i].mvgbe_mbuf);
			cdata->mvgbe_rx_chain[i].mvgbe_mbuf = NULL;
		}
	}
	for (i = 0; i < MVGBE_TX_RING_CNT; i++) {
		if (cdata->mvgbe_tx_chain[i].mvgbe_mbuf != NULL) {
			m_freem(cdata->mvgbe_tx_chain[i].mvgbe_mbuf);
			cdata->mvgbe_tx_chain[i].mvgbe_mbuf = NULL;
		}
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static void
mvgbe_watchdog(struct ifnet *ifp)
{
	struct mvgbe_softc *sc = ifp->if_softc;

	/*
	 * Reclaim first as there is a possibility of losing Tx completion
	 * interrupts.
	 */
	mvgbe_txeof(sc);
	if (sc->sc_cdata.mvgbe_tx_cnt != 0) {
		aprint_error_ifnet(ifp, "watchdog timeout\n");

		ifp->if_oerrors++;

		mvgbe_init(ifp);
	}
}


/*
 * Set media options.
 */
static int
mvgbe_ifmedia_upd(struct ifnet *ifp)
{
	struct mvgbe_softc *sc = ifp->if_softc;

	mii_mediachg(&sc->sc_mii);
	return 0;
}

/*
 * Report current media status.
 */
static void
mvgbe_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct mvgbe_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
}


static int
mvgbe_init_rx_ring(struct mvgbe_softc *sc)
{
	struct mvgbe_chain_data *cd = &sc->sc_cdata;
	struct mvgbe_ring_data *rd = sc->sc_rdata;
	int i;

	bzero((char *)rd->mvgbe_rx_ring,
	    sizeof(struct mvgbe_rx_desc) * MVGBE_RX_RING_CNT);

	for (i = 0; i < MVGBE_RX_RING_CNT; i++) {
		cd->mvgbe_rx_chain[i].mvgbe_desc =
		    &rd->mvgbe_rx_ring[i];
		if (i == MVGBE_RX_RING_CNT - 1) {
			cd->mvgbe_rx_chain[i].mvgbe_next =
			    &cd->mvgbe_rx_chain[0];
			rd->mvgbe_rx_ring[i].nextdescptr =
			    MVGBE_RX_RING_ADDR(sc, 0);
		} else {
			cd->mvgbe_rx_chain[i].mvgbe_next =
			    &cd->mvgbe_rx_chain[i + 1];
			rd->mvgbe_rx_ring[i].nextdescptr =
			    MVGBE_RX_RING_ADDR(sc, i + 1);
		}
	}

	for (i = 0; i < MVGBE_RX_RING_CNT; i++) {
		if (mvgbe_newbuf(sc, i, NULL,
		    sc->sc_cdata.mvgbe_rx_jumbo_map) == ENOBUFS) {
			aprint_error_ifnet(&sc->sc_ethercom.ec_if,
			    "failed alloc of %dth mbuf\n", i);
			return ENOBUFS;
		}
	}
	sc->sc_cdata.mvgbe_rx_prod = 0;
	sc->sc_cdata.mvgbe_rx_cons = 0;

	return 0;
}

static int
mvgbe_init_tx_ring(struct mvgbe_softc *sc)
{
	struct mvgbe_chain_data *cd = &sc->sc_cdata;
	struct mvgbe_ring_data *rd = sc->sc_rdata;
	int i;

	bzero((char *)sc->sc_rdata->mvgbe_tx_ring,
	    sizeof(struct mvgbe_tx_desc) * MVGBE_TX_RING_CNT);

	for (i = 0; i < MVGBE_TX_RING_CNT; i++) {
		cd->mvgbe_tx_chain[i].mvgbe_desc =
		    &rd->mvgbe_tx_ring[i];
		if (i == MVGBE_TX_RING_CNT - 1) {
			cd->mvgbe_tx_chain[i].mvgbe_next =
			    &cd->mvgbe_tx_chain[0];
			rd->mvgbe_tx_ring[i].nextdescptr =
			    MVGBE_TX_RING_ADDR(sc, 0);
		} else {
			cd->mvgbe_tx_chain[i].mvgbe_next =
			    &cd->mvgbe_tx_chain[i + 1];
			rd->mvgbe_tx_ring[i].nextdescptr =
			    MVGBE_TX_RING_ADDR(sc, i + 1);
		}
		rd->mvgbe_tx_ring[i].cmdsts = MVGBE_BUFFER_OWNED_BY_HOST;
	}

	sc->sc_cdata.mvgbe_tx_prod = 0;
	sc->sc_cdata.mvgbe_tx_cons = 0;
	sc->sc_cdata.mvgbe_tx_cnt = 0;

	MVGBE_CDTXSYNC(sc, 0, MVGBE_TX_RING_CNT,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return 0;
}

static int
mvgbe_newbuf(struct mvgbe_softc *sc, int i, struct mbuf *m,
		bus_dmamap_t dmamap)
{
	struct mbuf *m_new = NULL;
	struct mvgbe_chain *c;
	struct mvgbe_rx_desc *r;
	int align;

	if (m == NULL) {
		void *buf = NULL;

		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			aprint_error_ifnet(&sc->sc_ethercom.ec_if,
			    "no memory for rx list -- packet dropped!\n");
			return ENOBUFS;
		}

		/* Allocate the jumbo buffer */
		buf = mvgbe_jalloc(sc);
		if (buf == NULL) {
			m_freem(m_new);
			DPRINTFN(1, ("%s jumbo allocation failed -- packet "
			    "dropped!\n", sc->sc_ethercom.ec_if.if_xname));
			return ENOBUFS;
		}

		/* Attach the buffer to the mbuf */
		m_new->m_len = m_new->m_pkthdr.len = MVGBE_JLEN;
		MEXTADD(m_new, buf, MVGBE_JLEN, 0, mvgbe_jfree, sc);
	} else {
		/*
		 * We're re-using a previously allocated mbuf;
		 * be sure to re-init pointers and lengths to
		 * default values.
		 */
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MVGBE_JLEN;
		m_new->m_data = m_new->m_ext.ext_buf;
	}
	align = (u_long)m_new->m_data & MVGBE_BUF_MASK;
	if (align != 0)
		m_adj(m_new,  MVGBE_BUF_ALIGN - align);

	c = &sc->sc_cdata.mvgbe_rx_chain[i];
	r = c->mvgbe_desc;
	c->mvgbe_mbuf = m_new;
	r->bufptr = dmamap->dm_segs[0].ds_addr +
	    (((vaddr_t)m_new->m_data - (vaddr_t)sc->sc_cdata.mvgbe_jumbo_buf));
	r->bufsize = MVGBE_JLEN & ~MVGBE_BUF_MASK;
	r->cmdsts = MVGBE_BUFFER_OWNED_BY_DMA | MVGBE_RX_ENABLE_INTERRUPT;

	MVGBE_CDRXSYNC(sc, i, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return 0;
}

/*
 * Memory management for jumbo frames.
 */

static int
mvgbe_alloc_jumbo_mem(struct mvgbe_softc *sc)
{
	char *ptr, *kva;
	bus_dma_segment_t seg;
	int i, rseg, state, error;
	struct mvgbe_jpool_entry *entry;

	state = error = 0;

	/* Grab a big chunk o' storage. */
	if (bus_dmamem_alloc(sc->sc_dmat, MVGBE_JMEM, PAGE_SIZE, 0,
	    &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		aprint_error_dev(sc->sc_dev, "can't alloc rx buffers\n");
		return ENOBUFS;
	}

	state = 1;
	if (bus_dmamem_map(sc->sc_dmat, &seg, rseg, MVGBE_JMEM,
	    (void **)&kva, BUS_DMA_NOWAIT)) {
		aprint_error_dev(sc->sc_dev,
		    "can't map dma buffers (%d bytes)\n", MVGBE_JMEM);
		error = ENOBUFS;
		goto out;
	}

	state = 2;
	if (bus_dmamap_create(sc->sc_dmat, MVGBE_JMEM, 1, MVGBE_JMEM, 0,
	    BUS_DMA_NOWAIT, &sc->sc_cdata.mvgbe_rx_jumbo_map)) {
		aprint_error_dev(sc->sc_dev, "can't create dma map\n");
		error = ENOBUFS;
		goto out;
	}

	state = 3;
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_cdata.mvgbe_rx_jumbo_map,
	    kva, MVGBE_JMEM, NULL, BUS_DMA_NOWAIT)) {
		aprint_error_dev(sc->sc_dev, "can't load dma map\n");
		error = ENOBUFS;
		goto out;
	}

	state = 4;
	sc->sc_cdata.mvgbe_jumbo_buf = (void *)kva;
	DPRINTFN(1,("mvgbe_jumbo_buf = 0x%p\n", sc->sc_cdata.mvgbe_jumbo_buf));

	LIST_INIT(&sc->sc_jfree_listhead);
	LIST_INIT(&sc->sc_jinuse_listhead);

	/*
	 * Now divide it up into 9K pieces and save the addresses
	 * in an array.
	 */
	ptr = sc->sc_cdata.mvgbe_jumbo_buf;
	for (i = 0; i < MVGBE_JSLOTS; i++) {
		sc->sc_cdata.mvgbe_jslots[i] = ptr;
		ptr += MVGBE_JLEN;
		entry = kmem_alloc(sizeof(struct mvgbe_jpool_entry), KM_SLEEP);
		if (entry == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "no memory for jumbo buffer queue!\n");
			error = ENOBUFS;
			goto out;
		}
		entry->slot = i;
		if (i)
			LIST_INSERT_HEAD(&sc->sc_jfree_listhead, entry,
			    jpool_entries);
		else
			LIST_INSERT_HEAD(&sc->sc_jinuse_listhead, entry,
			    jpool_entries);
	}
out:
	if (error != 0) {
		switch (state) {
		case 4:
			bus_dmamap_unload(sc->sc_dmat,
			    sc->sc_cdata.mvgbe_rx_jumbo_map);
		case 3:
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_cdata.mvgbe_rx_jumbo_map);
		case 2:
			bus_dmamem_unmap(sc->sc_dmat, kva, MVGBE_JMEM);
		case 1:
			bus_dmamem_free(sc->sc_dmat, &seg, rseg);
			break;
		default:
			break;
		}
	}

	return error;
}

/*
 * Allocate a jumbo buffer.
 */
static void *
mvgbe_jalloc(struct mvgbe_softc *sc)
{
	struct mvgbe_jpool_entry *entry;

	entry = LIST_FIRST(&sc->sc_jfree_listhead);

	if (entry == NULL)
		return NULL;

	LIST_REMOVE(entry, jpool_entries);
	LIST_INSERT_HEAD(&sc->sc_jinuse_listhead, entry, jpool_entries);
	return sc->sc_cdata.mvgbe_jslots[entry->slot];
}

/*
 * Release a jumbo buffer.
 */
static void
mvgbe_jfree(struct mbuf *m, void *buf, size_t size, void *arg)
{
	struct mvgbe_jpool_entry *entry;
	struct mvgbe_softc *sc;
	int i, s;

	/* Extract the softc struct pointer. */
	sc = (struct mvgbe_softc *)arg;

	if (sc == NULL)
		panic("%s: can't find softc pointer!", __func__);

	/* calculate the slot this buffer belongs to */

	i = ((vaddr_t)buf - (vaddr_t)sc->sc_cdata.mvgbe_jumbo_buf) / MVGBE_JLEN;

	if ((i < 0) || (i >= MVGBE_JSLOTS))
		panic("%s: asked to free buffer that we don't manage!",
		    __func__);

	s = splvm();
	entry = LIST_FIRST(&sc->sc_jinuse_listhead);
	if (entry == NULL)
		panic("%s: buffer not in use!", __func__);
	entry->slot = i;
	LIST_REMOVE(entry, jpool_entries);
	LIST_INSERT_HEAD(&sc->sc_jfree_listhead, entry, jpool_entries);

	if (__predict_true(m != NULL))
		pool_cache_put(mb_cache, m);
	splx(s);
}

static int
mvgbe_encap(struct mvgbe_softc *sc, struct mbuf *m_head,
	      uint32_t *txidx)
{
	struct mvgbe_tx_desc *f = NULL;
	struct mvgbe_txmap_entry *entry;
	bus_dma_segment_t *txseg;
	bus_dmamap_t txmap;
	uint32_t first, current, last, cmdsts = 0;
	int m_csumflags, i;

	DPRINTFN(3, ("mvgbe_encap\n"));

	entry = SIMPLEQ_FIRST(&sc->sc_txmap_head);
	if (entry == NULL) {
		DPRINTFN(2, ("mvgbe_encap: no txmap available\n"));
		return ENOBUFS;
	}
	txmap = entry->dmamap;

	first = current = last = *txidx;

	/*
	 * Preserve m_pkthdr.csum_flags here since m_head might be
	 * updated by m_defrag()
	 */
	m_csumflags = m_head->m_pkthdr.csum_flags;

	/*
	 * Start packing the mbufs in this chain into
	 * the fragment pointers. Stop when we run out
	 * of fragments or hit the end of the mbuf chain.
	 */
	if (bus_dmamap_load_mbuf(sc->sc_dmat, txmap, m_head, BUS_DMA_NOWAIT)) {
		DPRINTFN(1, ("mvgbe_encap: dmamap failed\n"));
		return ENOBUFS;
	}

	/* Sync the DMA map. */
	bus_dmamap_sync(sc->sc_dmat, txmap, 0, txmap->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	if (sc->sc_cdata.mvgbe_tx_cnt + txmap->dm_nsegs >=
	    MVGBE_TX_RING_CNT) {
		DPRINTFN(2, ("mvgbe_encap: too few descriptors free\n"));
		bus_dmamap_unload(sc->sc_dmat, txmap);
		return ENOBUFS;
	}

	txseg = txmap->dm_segs;

	DPRINTFN(2, ("mvgbe_encap: dm_nsegs=%d\n", txmap->dm_nsegs));

	for (i = 0; i < txmap->dm_nsegs; i++) {
		f = &sc->sc_rdata->mvgbe_tx_ring[current];
		f->bufptr = txseg[i].ds_addr;
		f->bytecnt = txseg[i].ds_len;
		f->cmdsts = MVGBE_BUFFER_OWNED_BY_DMA;
		last = current;
		current = (current + 1) % MVGBE_TX_RING_CNT;
	}

	if (m_csumflags & M_CSUM_IPv4)
		cmdsts |= MVGBE_TX_GENERATE_IP_CHKSUM;
	if (m_csumflags & M_CSUM_TCPv4)
		cmdsts |=
		    MVGBE_TX_GENERATE_L4_CHKSUM | MVGBE_TX_L4_TYPE_TCP;
	if (m_csumflags & M_CSUM_UDPv4)
		cmdsts |=
		    MVGBE_TX_GENERATE_L4_CHKSUM | MVGBE_TX_L4_TYPE_UDP;
	if (m_csumflags & (M_CSUM_IPv4 | M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
		const int iphdr_unitlen = sizeof(struct ip) / sizeof(uint32_t);

		cmdsts |= MVGBE_TX_IP_NO_FRAG |
		    MVGBE_TX_IP_HEADER_LEN(iphdr_unitlen);	/* unit is 4B */
	}
	if (txmap->dm_nsegs == 1)
		f->cmdsts = cmdsts		|
		    MVGBE_BUFFER_OWNED_BY_DMA	|
		    MVGBE_TX_GENERATE_CRC	|
		    MVGBE_TX_ENABLE_INTERRUPT	|
		    MVGBE_TX_ZERO_PADDING	|
		    MVGBE_TX_FIRST_DESC		|
		    MVGBE_TX_LAST_DESC;
	else {
		f = &sc->sc_rdata->mvgbe_tx_ring[first];
		f->cmdsts = cmdsts		|
		    MVGBE_BUFFER_OWNED_BY_DMA	|
		    MVGBE_TX_GENERATE_CRC	|
		    MVGBE_TX_FIRST_DESC;

		f = &sc->sc_rdata->mvgbe_tx_ring[last];
		f->cmdsts =
		    MVGBE_BUFFER_OWNED_BY_DMA	|
		    MVGBE_TX_ENABLE_INTERRUPT	|
		    MVGBE_TX_ZERO_PADDING	|
		    MVGBE_TX_LAST_DESC;
	}

	sc->sc_cdata.mvgbe_tx_chain[last].mvgbe_mbuf = m_head;
	SIMPLEQ_REMOVE_HEAD(&sc->sc_txmap_head, link);
	sc->sc_cdata.mvgbe_tx_map[last] = entry;

	/* Sync descriptors before handing to chip */
	MVGBE_CDTXSYNC(sc, *txidx, txmap->dm_nsegs,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	sc->sc_cdata.mvgbe_tx_cnt += i;
	*txidx = current;

	DPRINTFN(3, ("mvgbe_encap: completed successfully\n"));

	return 0;
}

static void
mvgbe_rxeof(struct mvgbe_softc *sc)
{
	struct mvgbe_chain_data *cdata = &sc->sc_cdata;
	struct mvgbe_rx_desc *cur_rx;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	bus_dmamap_t dmamap;
	uint32_t rxstat;
	int idx, cur, total_len;

	idx = sc->sc_cdata.mvgbe_rx_prod;

	DPRINTFN(3, ("mvgbe_rxeof %d\n", idx));

	for (;;) {
		cur = idx;

		/* Sync the descriptor */
		MVGBE_CDRXSYNC(sc, idx,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		cur_rx = &sc->sc_rdata->mvgbe_rx_ring[idx];

		if ((cur_rx->cmdsts & MVGBE_BUFFER_OWNED_MASK) ==
		    MVGBE_BUFFER_OWNED_BY_DMA) {
			/* Invalidate the descriptor -- it's not ready yet */
			MVGBE_CDRXSYNC(sc, idx, BUS_DMASYNC_PREREAD);
			sc->sc_cdata.mvgbe_rx_prod = idx;
			break;
		}
#ifdef DIAGNOSTIC
		if ((cur_rx->cmdsts &
		    (MVGBE_RX_LAST_DESC | MVGBE_RX_FIRST_DESC)) !=
		    (MVGBE_RX_LAST_DESC | MVGBE_RX_FIRST_DESC))
			panic(
			    "mvgbe_rxeof: buffer size is smaller than packet");
#endif

		dmamap = sc->sc_cdata.mvgbe_rx_jumbo_map;

		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);

		m = cdata->mvgbe_rx_chain[idx].mvgbe_mbuf;
		cdata->mvgbe_rx_chain[idx].mvgbe_mbuf = NULL;
		total_len = cur_rx->bytecnt;
		rxstat = cur_rx->cmdsts;

		cdata->mvgbe_rx_map[idx] = NULL;

		idx = (idx + 1) % MVGBE_RX_RING_CNT;

		if (rxstat & MVGBE_ERROR_SUMMARY) {
#if 0
			int err = rxstat & MVGBE_RX_ERROR_CODE_MASK;

			if (err == MVGBE_RX_CRC_ERROR)
				ifp->if_ierrors++;
			if (err == MVGBE_RX_OVERRUN_ERROR)
				ifp->if_ierrors++;
			if (err == MVGBE_RX_MAX_FRAME_LEN_ERROR)
				ifp->if_ierrors++;
			if (err == MVGBE_RX_RESOURCE_ERROR)
				ifp->if_ierrors++;
#else
			ifp->if_ierrors++;
#endif
			mvgbe_newbuf(sc, cur, m, dmamap);
			continue;
		}

		if (total_len > MVGBE_RX_CSUM_MIN_BYTE) {
			/* Check IP header checksum */
			if (rxstat & MVGBE_RX_IP_FRAME_TYPE) {
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
				if (!(rxstat & MVGBE_RX_IP_HEADER_OK))
					m->m_pkthdr.csum_flags |=
					    M_CSUM_IPv4_BAD;
			}
			/* Check TCP/UDP checksum */
			if (rxstat & MVGBE_RX_L4_TYPE_TCP)
				m->m_pkthdr.csum_flags |= M_CSUM_TCPv4;
			else if (rxstat & MVGBE_RX_L4_TYPE_UDP)
				m->m_pkthdr.csum_flags |= M_CSUM_UDPv4;
			if (!(rxstat & MVGBE_RX_L4_CHECKSUM))
				m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
		}

		/*
		 * Try to allocate a new jumbo buffer. If that
		 * fails, copy the packet to mbufs and put the
		 * jumbo buffer back in the ring so it can be
		 * re-used. If allocating mbufs fails, then we
		 * have to drop the packet.
		 */
		if (mvgbe_newbuf(sc, cur, NULL, dmamap) == ENOBUFS) {
			struct mbuf *m0;

			m0 = m_devget(mtod(m, char *), total_len, 0, ifp, NULL);
			mvgbe_newbuf(sc, cur, m, dmamap);
			if (m0 == NULL) {
				aprint_error_ifnet(ifp,
				    "no receive buffers available --"
				    " packet dropped!\n");
				ifp->if_ierrors++;
				continue;
			}
			m = m0;
		} else {
			m->m_pkthdr.rcvif = ifp;
			m->m_pkthdr.len = m->m_len = total_len;
		}

		/* Skip on first 2byte (HW header) */
		m_adj(m,  MVGBE_HWHEADER_SIZE);
		m->m_flags |= M_HASFCS;

		ifp->if_ipackets++;

		if (ifp->if_bpf)
			bpf_ops->bpf_mtap(ifp->if_bpf, m);

		/* pass it on. */
		(*ifp->if_input)(ifp, m);
	}
}

static void
mvgbe_txeof(struct mvgbe_softc *sc)
{
	struct mvgbe_chain_data *cdata = &sc->sc_cdata;
	struct mvgbe_tx_desc *cur_tx;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mvgbe_txmap_entry *entry;
	int idx;

	DPRINTFN(3, ("mvgbe_txeof\n"));

	/*
	 * Go through our tx ring and free mbufs for those
	 * frames that have been sent.
	 */
	idx = cdata->mvgbe_tx_cons;
	while (idx != cdata->mvgbe_tx_prod) {
		MVGBE_CDTXSYNC(sc, idx, 1,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		cur_tx = &sc->sc_rdata->mvgbe_tx_ring[idx];
#ifdef MVGBE_DEBUG
		if (mvgbe_debug >= 3)
			mvgbe_dump_txdesc(cur_tx, idx);
#endif
		if ((cur_tx->cmdsts & MVGBE_BUFFER_OWNED_MASK) ==
		    MVGBE_BUFFER_OWNED_BY_DMA) {
			MVGBE_CDTXSYNC(sc, idx, 1, BUS_DMASYNC_PREREAD);
			break;
		}
		if (cur_tx->cmdsts & MVGBE_TX_LAST_DESC)
			ifp->if_opackets++;
		if (cur_tx->cmdsts & MVGBE_ERROR_SUMMARY) {
			int err = cur_tx->cmdsts & MVGBE_TX_ERROR_CODE_MASK;

			if (err == MVGBE_TX_LATE_COLLISION_ERROR)
				ifp->if_collisions++;
			if (err == MVGBE_TX_UNDERRUN_ERROR)
				ifp->if_oerrors++;
			if (err == MVGBE_TX_EXCESSIVE_COLLISION_ERRO)
				ifp->if_collisions++;
		}
		if (cdata->mvgbe_tx_chain[idx].mvgbe_mbuf != NULL) {
			entry = cdata->mvgbe_tx_map[idx];

			m_freem(cdata->mvgbe_tx_chain[idx].mvgbe_mbuf);
			cdata->mvgbe_tx_chain[idx].mvgbe_mbuf = NULL;

			bus_dmamap_sync(sc->sc_dmat, entry->dmamap, 0,
			    entry->dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);

			bus_dmamap_unload(sc->sc_dmat, entry->dmamap);
			SIMPLEQ_INSERT_TAIL(&sc->sc_txmap_head, entry, link);
			cdata->mvgbe_tx_map[idx] = NULL;
		}
		cdata->mvgbe_tx_cnt--;
		idx = (idx + 1) % MVGBE_TX_RING_CNT;
	}
	if (cdata->mvgbe_tx_cnt == 0)
		ifp->if_timer = 0;

	if (cdata->mvgbe_tx_cnt < MVGBE_TX_RING_CNT - 2)
		ifp->if_flags &= ~IFF_OACTIVE;

	cdata->mvgbe_tx_cons = idx;
}

static void
mvgbe_setmulti(struct mvgbe_softc *sc)
{
	struct ifnet *ifp= &sc->sc_ethercom.ec_if;
	uint32_t pxc, dfut, upm = 0, filter = 0;
	uint8_t ln = sc->sc_enaddr[5] & 0xf;		/* last nibble */

	if (ifp->if_flags & IFF_PROMISC) {
		upm = MVGBE_PXC_UPM;
		filter =
		    MVGBE_DF(0, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS) |
		    MVGBE_DF(1, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS) |
		    MVGBE_DF(2, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS) |
		    MVGBE_DF(3, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS);
	} else if (ifp->if_flags & IFF_ALLMULTI) {
		filter =
		    MVGBE_DF(0, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS) |
		    MVGBE_DF(1, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS) |
		    MVGBE_DF(2, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS) |
		    MVGBE_DF(3, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS);
	}

	/* Set Unicast Promiscuous mode */
	pxc = MVGBE_READ(sc, MVGBE_PXC);
	pxc &= ~MVGBE_PXC_UPM;
	pxc |= upm;
	MVGBE_WRITE(sc, MVGBE_PXC, pxc);

	/* Set Destination Address Filter Multicast Tables */
	MVGBE_WRITE_FILTER(sc, MVGBE_DFSMT, filter, MVGBE_NDFSMT);
	MVGBE_WRITE_FILTER(sc, MVGBE_DFOMT, filter, MVGBE_NDFOMT);

	if (ifp->if_flags & IFF_PROMISC) {
		/* necessary ? */
		MVGBE_WRITE_FILTER(sc, MVGBE_DFUT, filter, MVGBE_NDFUT);
		return;
	}

	/* Set Destination Address Filter Unicast Table */
	dfut = MVGBE_READ_FILTER(sc, MVGBE_DFUT + (ln & 0x0c));
	dfut &= ~MVGBE_DF(ln & 0x03, MVGBE_DF_QUEUE_MASK);;
	dfut |= MVGBE_DF(ln & 0x03, MVGBE_DF_QUEUE(0) | MVGBE_DF_PASS);
	MVGBE_WRITE_FILTER(sc, MVGBE_DFUT + (ln & 0x0c), dfut, 1);
}

#ifdef MVGBE_DEBUG
static void
mvgbe_dump_txdesc(struct mvgbe_tx_desc *desc, int idx)
{
#define DESC_PRINT(X)					\
	if (X)						\
		printf("txdesc[%d]." #X "=%#x\n", idx, X);

#if BYTE_ORDER == BIG_ENDIAN
       DESC_PRINT(desc->bytecnt);
       DESC_PRINT(desc->l4ichk);
       DESC_PRINT(desc->cmdsts);
       DESC_PRINT(desc->nextdescptr);
       DESC_PRINT(desc->bufptr);
#else	/* LITTLE_ENDIAN */
       DESC_PRINT(desc->cmdsts);
       DESC_PRINT(desc->l4ichk);
       DESC_PRINT(desc->bytecnt);
       DESC_PRINT(desc->bufptr);
       DESC_PRINT(desc->nextdescptr);
#endif
#undef DESC_PRINT
       printf("txdesc[%d].desc->returninfo=%#lx\n", idx, desc->returninfo);
       printf("txdesc[%d].desc->alignbufptr=%p\n", idx, desc->alignbufptr);
}
#endif
