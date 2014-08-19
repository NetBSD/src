/*	$NetBSD: ixp425_if_npe.c,v 1.23.2.1 2014/08/20 00:02:48 tls Exp $ */

/*-
 * Copyright (c) 2006 Sam Leffler.  All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: src/sys/arm/xscale/ixp425/if_npe.c,v 1.1 2006/11/19 23:55:23 sam Exp $");
#endif
__KERNEL_RCSID(0, "$NetBSD: ixp425_if_npe.c,v 1.23.2.1 2014/08/20 00:02:48 tls Exp $");

/*
 * Intel XScale NPE Ethernet driver.
 *
 * This driver handles the two ports present on the IXP425.
 * Packet processing is done by the Network Processing Engines
 * (NPE's) that work together with a MAC and PHY. The MAC
 * is also mapped to the XScale cpu; the PHY is accessed via
 * the MAC. NPE-XScale communication happens through h/w
 * queues managed by the Q Manager block.
 *
 * The code here replaces the ethAcc, ethMii, and ethDB classes
 * in the Intel Access Library (IAL) and the OS-specific driver.
 *
 * XXX add vlan support
 * XXX NPE-C port doesn't work yet
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/endian.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>

#include <sys/bus.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include <sys/rnd.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_qmgr.h>
#include <arm/xscale/ixp425_npevar.h>
#include <arm/xscale/ixp425_if_npereg.h>

#include <dev/mii/miivar.h>

#include "locators.h"

struct npebuf {
	struct npebuf	*ix_next;	/* chain to next buffer */
	void		*ix_m;		/* backpointer to mbuf */
	bus_dmamap_t	ix_map;		/* bus dma map for associated data */
	struct npehwbuf	*ix_hw;		/* associated h/w block */
	uint32_t	ix_neaddr;	/* phys address of ix_hw */
};

struct npedma {
	const char*	name;
	int		nbuf;		/* # npebuf's allocated */
	bus_dmamap_t	m_map;
	struct npehwbuf	*hwbuf;		/* NPE h/w buffers */
	bus_dmamap_t	buf_map;
	bus_addr_t	buf_phys;	/* phys addr of buffers */
	struct npebuf	*buf;		/* s/w buffers (1-1 w/ h/w) */
};

struct npe_softc {
	device_t	sc_dev;
	struct ethercom	sc_ethercom;
	uint8_t		sc_enaddr[ETHER_ADDR_LEN];
	struct mii_data	sc_mii;
	bus_space_tag_t	sc_iot;		
	bus_dma_tag_t	sc_dt;
	bus_space_handle_t sc_ioh;	/* MAC register window */
	bus_space_handle_t sc_miih;	/* MII register window */
	struct ixpnpe_softc *sc_npe;	/* NPE support */
	int		sc_unit;
	int		sc_phy;
	struct callout	sc_tick_ch;	/* Tick callout */
	struct npedma	txdma;
	struct npebuf	*tx_free;	/* list of free tx buffers */
	struct npedma	rxdma;
	int		rx_qid;		/* rx qid */
	int		rx_freeqid;	/* rx free buffers qid */
	int		tx_qid;		/* tx qid */
	int		tx_doneqid;	/* tx completed qid */
	struct npestats	*sc_stats;
	bus_dmamap_t	sc_stats_map;
	bus_addr_t	sc_stats_phys;	/* phys addr of sc_stats */
	int		sc_if_flags;	/* keep last if_flags */
	krndsource_t rnd_source; /* random source */
};

/*
 * Per-unit static configuration for IXP425.  The tx and
 * rx free Q id's are fixed by the NPE microcode.  The
 * rx Q id's are programmed to be separate to simplify
 * multi-port processing.  It may be better to handle
 * all traffic through one Q (as done by the Intel drivers).
 *
 * Note that the PHY's are accessible only from MAC A
 * on the IXP425.  This and other platform-specific
 * assumptions probably need to be handled through hints.
 */
static const struct {
	const char	*desc;		/* device description */
	int		npeid;		/* NPE assignment */
	int		macport;	/* Port number of the MAC */
	uint32_t	imageid;	/* NPE firmware image id */
	uint32_t	regbase;
	int		regsize;
	uint32_t	miibase;
	int		miisize;
	uint8_t		rx_qid;
	uint8_t		rx_freeqid;
	uint8_t		tx_qid;
	uint8_t		tx_doneqid;
} npeconfig[NPE_PORTS_MAX] = {
	{ .desc		= "IXP NPE-B",
	  .npeid	= NPE_B,
	  .macport	= 0x10,
	  .imageid	= IXP425_NPE_B_IMAGEID,
	  .regbase	= IXP425_MAC_A_HWBASE,
	  .regsize	= IXP425_MAC_A_SIZE,
	  .miibase	= IXP425_MAC_A_HWBASE,
	  .miisize	= IXP425_MAC_A_SIZE,
	  .rx_qid	= 4,
	  .rx_freeqid	= 27,
	  .tx_qid	= 24,
	  .tx_doneqid	= 31
	},
	{ .desc		= "IXP NPE-C",
	  .npeid	= NPE_C,
	  .macport	= 0x20,
	  .imageid	= IXP425_NPE_C_IMAGEID,
	  .regbase	= IXP425_MAC_B_HWBASE,
	  .regsize	= IXP425_MAC_B_SIZE,
	  .miibase	= IXP425_MAC_A_HWBASE,
	  .miisize	= IXP425_MAC_A_SIZE,
	  .rx_qid	= 12,
	  .rx_freeqid	= 28,
	  .tx_qid	= 25,
	  .tx_doneqid	= 31
	},
};
static struct npe_softc *npes[NPE_MAX];	/* NB: indexed by npeid */

static __inline uint32_t
RD4(struct npe_softc *sc, bus_size_t off)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, off);
}

static __inline void
WR4(struct npe_softc *sc, bus_size_t off, uint32_t val)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, off, val);
}

static int	npe_activate(struct npe_softc *);
#if 0
static void	npe_deactivate(struct npe_softc *);
#endif
static void	npe_ifmedia_status(struct ifnet *ifp, struct ifmediareq *ifmr);
static void	npe_setmac(struct npe_softc *sc, const u_char *eaddr);
static void	npe_getmac(struct npe_softc *sc);
static void	npe_txdone(int qid, void *arg);
static int	npe_rxbuf_init(struct npe_softc *, struct npebuf *,
			struct mbuf *);
static void	npe_rxdone(int qid, void *arg);
static void	npeinit_macreg(struct npe_softc *);
static int	npeinit(struct ifnet *);
static void	npeinit_resetcb(void *);
static void	npeinit_locked(void *);
static void	npestart(struct ifnet *);
static void	npestop(struct ifnet *, int);
static void	npewatchdog(struct ifnet *);
static int	npeioctl(struct ifnet * ifp, u_long, void *);

static int	npe_setrxqosentry(struct npe_softc *, int classix,
			int trafclass, int qid);
static int	npe_updatestats(struct npe_softc *);
#if 0
static int	npe_getstats(struct npe_softc *);
static uint32_t	npe_getimageid(struct npe_softc *);
static int	npe_setloopback(struct npe_softc *, int ena);
#endif

static int	npe_miibus_readreg(device_t, int, int);
static void	npe_miibus_writereg(device_t, int, int, int);
static void	npe_miibus_statchg(struct ifnet *);

static int	npe_debug;
#define DPRINTF(sc, fmt, ...) do {			\
	if (npe_debug) printf(fmt, __VA_ARGS__);	\
} while (0)
#define DPRINTFn(n, sc, fmt, ...) do {			\
	if (npe_debug >= n) printf(fmt, __VA_ARGS__);	\
} while (0)

#define	NPE_TXBUF	128
#define	NPE_RXBUF	64

#ifndef ETHER_ALIGN
#define	ETHER_ALIGN	2	/* XXX: Ditch this */
#endif

#define MAC2UINT64(addr)	(((uint64_t)addr[0] << 40)	\
				    + ((uint64_t)addr[1] << 32)	\
				    + ((uint64_t)addr[2] << 24)	\
				    + ((uint64_t)addr[3] << 16)	\
				    + ((uint64_t)addr[4] << 8)	\
				    + (uint64_t)addr[5])

/* NB: all tx done processing goes through one queue */
static int tx_doneqid = -1;

void (*npe_getmac_md)(int, uint8_t *);

static int npe_match(device_t, cfdata_t, void *);
static void npe_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(npe, sizeof(struct npe_softc),
    npe_match, npe_attach, NULL, NULL);

static int
npe_match(device_t parent, cfdata_t cf, void *arg)
{
	struct ixpnpe_attach_args *na = arg;

	return (na->na_unit == NPE_B || na->na_unit == NPE_C);
}

static void
npe_attach(device_t parent, device_t self, void *arg)
{
	struct npe_softc *sc = device_private(self);
	struct ixpnpe_softc *isc = device_private(parent);
	struct ixpnpe_attach_args *na = arg;
	struct ifnet *ifp;

	aprint_naive("\n");
	aprint_normal(": Ethernet co-processor\n");

	sc->sc_dev = self;
	sc->sc_iot = na->na_iot;
	sc->sc_dt = na->na_dt;
	sc->sc_npe = na->na_npe;
	sc->sc_unit = (na->na_unit == NPE_B) ? 0 : 1;
	sc->sc_phy = na->na_phy;

	memset(&sc->sc_ethercom, 0, sizeof(sc->sc_ethercom));
	memset(&sc->sc_mii, 0, sizeof(sc->sc_mii));

	callout_init(&sc->sc_tick_ch, 0);

	if (npe_activate(sc)) {
		aprint_error_dev(sc->sc_dev,
		    "Failed to activate NPE (missing microcode?)\n");
		return;
	}

	npe_getmac(sc);
	npeinit_macreg(sc);

	aprint_normal_dev(sc->sc_dev, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	ifp = &sc->sc_ethercom.ec_if;
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = npe_miibus_readreg;
	sc->sc_mii.mii_writereg = npe_miibus_writereg;
	sc->sc_mii.mii_statchg = npe_miibus_statchg;
	sc->sc_ethercom.ec_mii = &sc->sc_mii;

	ifmedia_init(&sc->sc_mii.mii_media, IFM_IMASK, ether_mediachange,
	    npe_ifmedia_status);

	mii_attach(sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
		    MII_OFFSET_ANY, MIIF_DOPAUSE);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

	ifp->if_softc = sc;
	strcpy(ifp->if_xname, device_xname(sc->sc_dev));
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = npestart;
	ifp->if_ioctl = npeioctl;
	ifp->if_watchdog = npewatchdog;
	ifp->if_init = npeinit;
	ifp->if_stop = npestop;
	IFQ_SET_READY(&ifp->if_snd);

	/* VLAN capable */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	/* callback function to reset MAC */
	isc->macresetcbfunc = npeinit_resetcb;
	isc->macresetcbarg = sc;
}

/*
 * Compute and install the multicast filter.
 */
static void
npe_setmcast(struct npe_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t mask[ETHER_ADDR_LEN], addr[ETHER_ADDR_LEN];
	uint32_t reg;
	uint32_t msg[2];
	int i;

	/* Always use filter. Is here a correct position? */
	reg = RD4(sc, NPE_MAC_RX_CNTRL1);
	WR4(sc, NPE_MAC_RX_CNTRL1, reg | NPE_RX_CNTRL1_ADDR_FLTR_EN);

	if (ifp->if_flags & IFF_PROMISC) {
		memset(mask, 0, ETHER_ADDR_LEN);
		memset(addr, 0, ETHER_ADDR_LEN);
	} else if (ifp->if_flags & IFF_ALLMULTI) {
		static const uint8_t allmulti[ETHER_ADDR_LEN] =
		    { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
 all_multi:
		memcpy(mask, allmulti, ETHER_ADDR_LEN);
		memcpy(addr, allmulti, ETHER_ADDR_LEN);
	} else {
		uint8_t clr[ETHER_ADDR_LEN], set[ETHER_ADDR_LEN];
		struct ether_multistep step;
		struct ether_multi *enm;

		memset(clr, 0, ETHER_ADDR_LEN);
		memset(set, 0xff, ETHER_ADDR_LEN);

		ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
				ifp->if_flags |= IFF_ALLMULTI;
				goto all_multi;
			}

			for (i = 0; i < ETHER_ADDR_LEN; i++) {
				clr[i] |= enm->enm_addrlo[i];
				set[i] &= enm->enm_addrlo[i];
			}

			ETHER_NEXT_MULTI(step, enm);
		}

		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			mask[i] = set[i] | ~clr[i];
			addr[i] = set[i];
		}
	}

	/*
	 * Write the mask and address registers.
	 */
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		WR4(sc, NPE_MAC_ADDR_MASK(i), mask[i]);
		WR4(sc, NPE_MAC_ADDR(i), addr[i]);
	}

	msg[0] = NPE_ADDRESSFILTERCONFIG << NPE_MAC_MSGID_SHL
	    | (npeconfig[sc->sc_unit].macport << NPE_MAC_PORTID_SHL);
	msg[1] = ((ifp->if_flags & IFF_PROMISC) ? 1 : 0) << 24
	    | ((RD4(sc, NPE_MAC_UNI_ADDR_6) & 0xff) << 16)
	    | (addr[5] << 8) | mask[5];
	ixpnpe_sendandrecvmsg(sc->sc_npe, msg, msg);
}

static int
npe_dma_setup(struct npe_softc *sc, struct npedma *dma,
	const char *name, int nbuf, int maxseg)
{
	bus_dma_segment_t seg;
	int rseg, error, i;
	void *hwbuf;
	size_t size;

	memset(dma, 0, sizeof(*dma));

	dma->name = name;
	dma->nbuf = nbuf;

	size = nbuf * sizeof(struct npehwbuf);

	/* XXX COHERENT for now */
	error = bus_dmamem_alloc(sc->sc_dt, size, sizeof(uint32_t), 0, &seg,
	    1, &rseg, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s %s buffers, error %u\n",
		    "allocate memory", dma->name, "h/w", error);
	}

	error = bus_dmamem_map(sc->sc_dt, &seg, 1, size, &hwbuf,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT | BUS_DMA_NOCACHE);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s %s buffers, error %u\n",
		    "map memory", dma->name, "h/w", error);
 free_dmamem:
		bus_dmamem_free(sc->sc_dt, &seg, rseg);
		return error;
	}
	dma->hwbuf = (void *)hwbuf;

	error = bus_dmamap_create(sc->sc_dt, size, 1, size, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &dma->buf_map);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s %s buffers, error %u\n",
		    "create map", dma->name, "h/w", error);
 unmap_dmamem:
		dma->hwbuf = NULL;
		bus_dmamem_unmap(sc->sc_dt, hwbuf, size);
		goto free_dmamem;
	}

	error = bus_dmamap_load(sc->sc_dt, dma->buf_map, hwbuf, size, NULL,
	    BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s %s buffers, error %u\n",
		    "load map", dma->name, "h/w", error);
 destroy_dmamap:
		bus_dmamap_destroy(sc->sc_dt, dma->buf_map);
		goto unmap_dmamem;
	}

	/* XXX M_TEMP */
	dma->buf = malloc(nbuf * sizeof(struct npebuf), M_TEMP, M_NOWAIT | M_ZERO);
	if (dma->buf == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s %s buffers, error %u\n",
		    "allocate memory", dma->name, "h/w", error);
		bus_dmamap_unload(sc->sc_dt, dma->buf_map);
		error = ENOMEM;
		goto destroy_dmamap;
	}

	dma->buf_phys = dma->buf_map->dm_segs[0].ds_addr;
	for (i = 0; i < dma->nbuf; i++) {
		struct npebuf *npe = &dma->buf[i];
		struct npehwbuf *hw = &dma->hwbuf[i];

		/* calculate offset to shared area */
		npe->ix_neaddr = dma->buf_phys +
			((uintptr_t)hw - (uintptr_t)dma->hwbuf);
		KASSERT((npe->ix_neaddr & 0x1f) == 0);
		error = bus_dmamap_create(sc->sc_dt, MCLBYTES, maxseg,
		    MCLBYTES, 0, 0, &npe->ix_map);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to %s for %s buffer %u, error %u\n",
			    "create dmamap", dma->name, i, error);
			/* XXXSCW: Free up maps... */
			return error;
		}
		npe->ix_hw = hw;
	}
	bus_dmamap_sync(sc->sc_dt, dma->buf_map, 0, dma->buf_map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);
	return 0;
}

#if 0
static void
npe_dma_destroy(struct npe_softc *sc, struct npedma *dma)
{
	int i;

/* XXXSCW: Clean this up */

	if (dma->hwbuf != NULL) {
		for (i = 0; i < dma->nbuf; i++) {
			struct npebuf *npe = &dma->buf[i];
			bus_dmamap_destroy(sc->sc_dt, npe->ix_map);
		}
		bus_dmamap_unload(sc->sc_dt, dma->buf_map);
		bus_dmamem_free(sc->sc_dt, (void *)dma->hwbuf, dma->buf_map);
		bus_dmamap_destroy(sc->sc_dt, dma->buf_map);
	}
	if (dma->buf != NULL)
		free(dma->buf, M_TEMP);
	memset(dma, 0, sizeof(*dma));
}
#endif

static int
npe_activate(struct npe_softc *sc)
{
	bus_dma_segment_t seg;
	int unit = sc->sc_unit;
	int error, i, rseg;
	void *statbuf;

	/* load NPE firmware and start it running */
	error = ixpnpe_init(sc->sc_npe, "npe_fw", npeconfig[unit].imageid);
	if (error != 0)
		return error;

	if (bus_space_map(sc->sc_iot, npeconfig[unit].regbase,
	    npeconfig[unit].regsize, 0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "Cannot map registers 0x%x:0x%x\n",
		    npeconfig[unit].regbase, npeconfig[unit].regsize);
		return ENOMEM;
	}

	if (npeconfig[unit].miibase != npeconfig[unit].regbase) {
		/*
		 * The PHY's are only accessible from one MAC (it appears)
		 * so for other MAC's setup an additional mapping for
		 * frobbing the PHY registers.
		 */
		if (bus_space_map(sc->sc_iot, npeconfig[unit].miibase,
		    npeconfig[unit].miisize, 0, &sc->sc_miih)) {
			aprint_error_dev(sc->sc_dev,
			    "Cannot map MII registers 0x%x:0x%x\n",
			    npeconfig[unit].miibase, npeconfig[unit].miisize);
			return ENOMEM;
		}
	} else
		sc->sc_miih = sc->sc_ioh;
	error = npe_dma_setup(sc, &sc->txdma, "tx", NPE_TXBUF, NPE_MAXSEG);
	if (error != 0)
		return error;
	error = npe_dma_setup(sc, &sc->rxdma, "rx", NPE_RXBUF, 1);
	if (error != 0)
		return error;

	/* setup statistics block */
	error = bus_dmamem_alloc(sc->sc_dt, sizeof(struct npestats),
	    sizeof(uint32_t), 0, &seg, 1, &rseg, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s, error %u\n",
		    "allocate memory", "stats block", error);
		return error;
	}

	error = bus_dmamem_map(sc->sc_dt, &seg, 1, sizeof(struct npestats),
	    &statbuf, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s, error %u\n",
		    "map memory", "stats block", error);
		return error;
	}
	sc->sc_stats = (void *)statbuf;

	error = bus_dmamap_create(sc->sc_dt, sizeof(struct npestats), 1,
	    sizeof(struct npestats), 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &sc->sc_stats_map);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s, error %u\n",
		    "create map", "stats block", error);
		return error;
	}

	if (bus_dmamap_load(sc->sc_dt, sc->sc_stats_map, sc->sc_stats,
	    sizeof(struct npestats), NULL, BUS_DMA_NOWAIT) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to %s for %s, error %u\n",
		    "load map", "stats block", error);
		return error;
	}
	sc->sc_stats_phys = sc->sc_stats_map->dm_segs[0].ds_addr;

	/* XXX disable half-bridge LEARNING+FILTERING feature */

	/*
	 * Setup h/w rx/tx queues.  There are four q's:
	 *   rx		inbound q of rx'd frames
	 *   rx_free	pool of ixpbuf's for receiving frames
	 *   tx		outbound q of frames to send
	 *   tx_done	q of tx frames that have been processed
	 *
	 * The NPE handles the actual tx/rx process and the q manager
	 * handles the queues.  The driver just writes entries to the
	 * q manager mailbox's and gets callbacks when there are rx'd
	 * frames to process or tx'd frames to reap.  These callbacks
	 * are controlled by the q configurations; e.g. we get a
	 * callback when tx_done has 2 or more frames to process and
	 * when the rx q has at least one frame.  These setings can
	 * changed at the time the q is configured.
	 */
	sc->rx_qid = npeconfig[unit].rx_qid;
	ixpqmgr_qconfig(sc->rx_qid, NPE_RXBUF, 0,  1,
		IX_QMGR_Q_SOURCE_ID_NOT_E, npe_rxdone, sc);
	sc->rx_freeqid = npeconfig[unit].rx_freeqid;
	ixpqmgr_qconfig(sc->rx_freeqid,	NPE_RXBUF, 0, NPE_RXBUF/2, 0, NULL, sc);
	/* tell the NPE to direct all traffic to rx_qid */
#if 0
	for (i = 0; i < 8; i++)
#else
printf("%s: remember to fix rx q setup\n", device_xname(sc->sc_dev));
	for (i = 0; i < 4; i++)
#endif
		npe_setrxqosentry(sc, i, 0, sc->rx_qid);

	sc->tx_qid = npeconfig[unit].tx_qid;
	sc->tx_doneqid = npeconfig[unit].tx_doneqid;
	ixpqmgr_qconfig(sc->tx_qid, NPE_TXBUF, 0, NPE_TXBUF, 0, NULL, sc);
	if (tx_doneqid == -1) {
		ixpqmgr_qconfig(sc->tx_doneqid,	NPE_TXBUF, 0,  2,
			IX_QMGR_Q_SOURCE_ID_NOT_E, npe_txdone, sc);
		tx_doneqid = sc->tx_doneqid;
	}

	KASSERT(npes[npeconfig[unit].npeid] == NULL);
	npes[npeconfig[unit].npeid] = sc;

	return 0;
}

#if 0
static void
npe_deactivate(struct npe_softc *sc);
{
	int unit = sc->sc_unit;

	npes[npeconfig[unit].npeid] = NULL;

	/* XXX disable q's */
	if (sc->sc_npe != NULL)
		ixpnpe_stop(sc->sc_npe);
	if (sc->sc_stats != NULL) {
		bus_dmamap_unload(sc->sc_stats_tag, sc->sc_stats_map);
		bus_dmamem_free(sc->sc_stats_tag, sc->sc_stats,
			sc->sc_stats_map);
		bus_dmamap_destroy(sc->sc_stats_tag, sc->sc_stats_map);
	}
	if (sc->sc_stats_tag != NULL)
		bus_dma_tag_destroy(sc->sc_stats_tag);
	npe_dma_destroy(sc, &sc->txdma);
	npe_dma_destroy(sc, &sc->rxdma);
	bus_generic_detach(sc->sc_dev);
	if (sc->sc_mii)
		device_delete_child(sc->sc_dev, sc->sc_mii);
#if 0
	/* XXX sc_ioh and sc_miih */
	if (sc->mem_res)
		bus_release_resource(dev, SYS_RES_IOPORT,
		    rman_get_rid(sc->mem_res), sc->mem_res);
	sc->mem_res = 0;
#endif
}
#endif

/*
 * Notify the world which media we're using.
 */
static void
npe_ifmedia_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct npe_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);

	ifmr->ifm_active = sc->sc_mii.mii_media_active;
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
}

static void
npe_addstats(struct npe_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct npestats *ns = sc->sc_stats;

	ifp->if_oerrors +=
		  be32toh(ns->dot3StatsInternalMacTransmitErrors)
		+ be32toh(ns->dot3StatsCarrierSenseErrors)
		+ be32toh(ns->TxVLANIdFilterDiscards)
		;
	ifp->if_ierrors += be32toh(ns->dot3StatsFCSErrors)
		+ be32toh(ns->dot3StatsInternalMacReceiveErrors)
		+ be32toh(ns->RxOverrunDiscards)
		+ be32toh(ns->RxUnderflowEntryDiscards)
		;
	ifp->if_collisions +=
		  be32toh(ns->dot3StatsSingleCollisionFrames)
		+ be32toh(ns->dot3StatsMultipleCollisionFrames)
		;
}

static void
npe_tick(void *xsc)
{
#define	ACK	(NPE_RESETSTATS << NPE_MAC_MSGID_SHL)
	struct npe_softc *sc = xsc;
	uint32_t msg[2];

	/*
	 * NB: to avoid sleeping with the softc lock held we
	 * split the NPE msg processing into two parts.  The
	 * request for statistics is sent w/o waiting for a
	 * reply and then on the next tick we retrieve the
	 * results.  This works because npe_tick is the only
	 * code that talks via the mailbox's (except at setup).
	 * This likely can be handled better.
	 */
	if (ixpnpe_recvmsg(sc->sc_npe, msg) == 0 && msg[0] == ACK) {
		bus_dmamap_sync(sc->sc_dt, sc->sc_stats_map, 0,
		    sizeof(struct npestats), BUS_DMASYNC_POSTREAD);
		npe_addstats(sc);
	}
	npe_updatestats(sc);
	mii_tick(&sc->sc_mii);

	/* schedule next poll */
	callout_reset(&sc->sc_tick_ch, hz, npe_tick, sc);
#undef ACK
}

static void
npe_setmac(struct npe_softc *sc, const u_char *eaddr)
{

	WR4(sc, NPE_MAC_UNI_ADDR_1, eaddr[0]);
	WR4(sc, NPE_MAC_UNI_ADDR_2, eaddr[1]);
	WR4(sc, NPE_MAC_UNI_ADDR_3, eaddr[2]);
	WR4(sc, NPE_MAC_UNI_ADDR_4, eaddr[3]);
	WR4(sc, NPE_MAC_UNI_ADDR_5, eaddr[4]);
	WR4(sc, NPE_MAC_UNI_ADDR_6, eaddr[5]);
}

static void
npe_getmac(struct npe_softc *sc)
{
	uint8_t *eaddr = sc->sc_enaddr;

	if (npe_getmac_md != NULL) {
		(*npe_getmac_md)(device_unit(sc->sc_dev), eaddr);
	} else {
		/*
		 * Some system's unicast address appears to be loaded from
		 * EEPROM on reset
		 */
		eaddr[0] = RD4(sc, NPE_MAC_UNI_ADDR_1) & 0xff;
		eaddr[1] = RD4(sc, NPE_MAC_UNI_ADDR_2) & 0xff;
		eaddr[2] = RD4(sc, NPE_MAC_UNI_ADDR_3) & 0xff;
		eaddr[3] = RD4(sc, NPE_MAC_UNI_ADDR_4) & 0xff;
		eaddr[4] = RD4(sc, NPE_MAC_UNI_ADDR_5) & 0xff;
		eaddr[5] = RD4(sc, NPE_MAC_UNI_ADDR_6) & 0xff;
	}
}

struct txdone {
	struct npebuf *head;
	struct npebuf **tail;
	int count;
};

static __inline void
npe_txdone_finish(struct npe_softc *sc, const struct txdone *td)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	*td->tail = sc->tx_free;
	sc->tx_free = td->head;
	/*
	 * We're no longer busy, so clear the busy flag and call the
	 * start routine to xmit more packets.
	 */
	ifp->if_opackets += td->count;
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;
	npestart(ifp);
}

/*
 * Q manager callback on tx done queue.  Reap mbufs
 * and return tx buffers to the free list.  Finally
 * restart output.  Note the microcode has only one
 * txdone q wired into it so we must use the NPE ID
 * returned with each npehwbuf to decide where to
 * send buffers.
 */
static void
npe_txdone(int qid, void *arg)
{
#define	P2V(a, dma) \
	&(dma)->buf[((a) - (dma)->buf_phys) / sizeof(struct npehwbuf)]
	struct npe_softc *sc;
	struct npebuf *npe;
	struct txdone *td, q[NPE_MAX];
	uint32_t entry;

	/* XXX no NPE-A support */
	q[NPE_B].tail = &q[NPE_B].head; q[NPE_B].count = 0;
	q[NPE_C].tail = &q[NPE_C].head; q[NPE_C].count = 0;
	/* XXX max # at a time? */
	while (ixpqmgr_qread(qid, &entry) == 0) {
		sc = npes[NPE_QM_Q_NPE(entry)];
		DPRINTF(sc, "%s: entry 0x%x NPE %u port %u\n",
		    __func__, entry, NPE_QM_Q_NPE(entry), NPE_QM_Q_PORT(entry));
		rnd_add_uint32(&sc->rnd_source, entry);

		npe = P2V(NPE_QM_Q_ADDR(entry), &sc->txdma);
		m_freem(npe->ix_m);
		npe->ix_m = NULL;

		td = &q[NPE_QM_Q_NPE(entry)];
		*td->tail = npe;
		td->tail = &npe->ix_next;
		td->count++;
	}

	if (q[NPE_B].count)
		npe_txdone_finish(npes[NPE_B], &q[NPE_B]);
	if (q[NPE_C].count)
		npe_txdone_finish(npes[NPE_C], &q[NPE_C]);
#undef P2V
}

static __inline struct mbuf *
npe_getcl(void)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m != NULL) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			m = NULL;
		}
	}
	return (m);
}

static int
npe_rxbuf_init(struct npe_softc *sc, struct npebuf *npe, struct mbuf *m)
{
	struct npehwbuf *hw;
	int error;

	if (m == NULL) {
		m = npe_getcl();
		if (m == NULL)
			return ENOBUFS;
	}
	KASSERT(m->m_ext.ext_size >= (NPE_FRAME_SIZE_DEFAULT + ETHER_ALIGN));
	m->m_pkthdr.len = m->m_len = NPE_FRAME_SIZE_DEFAULT;
	/* backload payload and align ip hdr */
	m->m_data = m->m_ext.ext_buf + (m->m_ext.ext_size
	    - (NPE_FRAME_SIZE_DEFAULT + ETHER_ALIGN));
	error = bus_dmamap_load_mbuf(sc->sc_dt, npe->ix_map, m,
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error != 0) {
		m_freem(m);
		return error;
	}
	hw = npe->ix_hw;
	hw->ix_ne[0].data = htobe32(npe->ix_map->dm_segs[0].ds_addr);
	/* NB: NPE requires length be a multiple of 64 */
	/* NB: buffer length is shifted in word */
	hw->ix_ne[0].len = htobe32(npe->ix_map->dm_segs[0].ds_len << 16);
	hw->ix_ne[0].next = 0;
	npe->ix_m = m;
	/* Flush the memory in the mbuf */
	bus_dmamap_sync(sc->sc_dt, npe->ix_map, 0, npe->ix_map->dm_mapsize,
	    BUS_DMASYNC_PREREAD);
	return 0;
}

/*
 * RX q processing for a specific NPE.  Claim entries
 * from the hardware queue and pass the frames up the
 * stack. Pass the rx buffers to the free list.
 */
static void
npe_rxdone(int qid, void *arg)
{
#define	P2V(a, dma) \
	&(dma)->buf[((a) - (dma)->buf_phys) / sizeof(struct npehwbuf)]
	struct npe_softc *sc = arg;
	struct npedma *dma = &sc->rxdma;
	uint32_t entry;

	while (ixpqmgr_qread(qid, &entry) == 0) {
		struct npebuf *npe = P2V(NPE_QM_Q_ADDR(entry), dma);
		struct mbuf *m;

		DPRINTF(sc, "%s: entry 0x%x neaddr 0x%x ne_len 0x%x\n",
		    __func__, entry, npe->ix_neaddr, npe->ix_hw->ix_ne[0].len);
		rnd_add_uint32(&sc->rnd_source, entry);
		/*
		 * Allocate a new mbuf to replenish the rx buffer.
		 * If doing so fails we drop the rx'd frame so we
		 * can reuse the previous mbuf.  When we're able to
		 * allocate a new mbuf dispatch the mbuf w/ rx'd
		 * data up the stack and replace it with the newly
		 * allocated one.
		 */
		m = npe_getcl();
		if (m != NULL) {
			struct mbuf *mrx = npe->ix_m;
			struct npehwbuf *hw = npe->ix_hw;
			struct ifnet *ifp = &sc->sc_ethercom.ec_if;

			/* Flush mbuf memory for rx'd data */
			bus_dmamap_sync(sc->sc_dt, npe->ix_map, 0,
			    npe->ix_map->dm_mapsize, BUS_DMASYNC_POSTREAD);

			/* XXX flush hw buffer; works now 'cuz coherent */
			/* set m_len etc. per rx frame size */
			mrx->m_len = be32toh(hw->ix_ne[0].len) & 0xffff;
			mrx->m_pkthdr.len = mrx->m_len;
			mrx->m_pkthdr.rcvif = ifp;
			/* Don't add M_HASFCS. See below */

#if 1
			if (mrx->m_pkthdr.len < sizeof(struct ether_header)) {
				log(LOG_INFO, "%s: too short frame (len=%d)\n",
				    device_xname(sc->sc_dev), mrx->m_pkthdr.len);
				/* Back out "newly allocated" mbuf. */
				m_freem(m);
				ifp->if_ierrors++;
				goto fail;
			}
			if ((ifp->if_flags & IFF_PROMISC) == 0) {
				struct ether_header *eh;

				/*
				 * Workaround for "Non-Intel XScale Technology
				 * Eratta" No. 29. AA:BB:CC:DD:EE:xF's packet
				 * matches the filter (both unicast and
				 * multicast).
				 */
				eh = mtod(mrx, struct ether_header *);
				if (ETHER_IS_MULTICAST(eh->ether_dhost) == 0) {
					/* unicast */

					if (sc->sc_enaddr[5] != eh->ether_dhost[5]) {
						/* discard it */
#if 0
						printf("discard it\n");
#endif
						/*
						 * Back out "newly allocated"
						 * mbuf.
						 */
						m_freem(m);
						goto fail;
					}
				} else if (memcmp(eh->ether_dhost,
					etherbroadcastaddr, 6) == 0) {
					/* Always accept broadcast packet*/
				} else {
					struct ethercom *ec = &sc->sc_ethercom;
					struct ether_multi *enm;
					struct ether_multistep step;
					int match = 0;

					/* multicast */

					ETHER_FIRST_MULTI(step, ec, enm);
					while (enm != NULL) {
						uint64_t lowint, highint, dest;

						lowint = MAC2UINT64(enm->enm_addrlo);
						highint = MAC2UINT64(enm->enm_addrhi);
						dest = MAC2UINT64(eh->ether_dhost);
#if 0
						printf("%llx\n", lowint);
						printf("%llx\n", dest);
						printf("%llx\n", highint);
#endif
						if ((lowint <= dest) && (dest <= highint)) {
							match = 1;
							break;
						}
						ETHER_NEXT_MULTI(step, enm);
					}
					if (match == 0) {
						/* discard it */
#if 0
						printf("discard it(M)\n");
#endif
						/*
						 * Back out "newly allocated"
						 * mbuf.
						 */
						m_freem(m);
						goto fail;
					}
				}
			}
			if (mrx->m_pkthdr.len > NPE_FRAME_SIZE_DEFAULT) {
				log(LOG_INFO, "%s: oversized frame (len=%d)\n",
				    device_xname(sc->sc_dev), mrx->m_pkthdr.len);
				/* Back out "newly allocated" mbuf. */
				m_freem(m);
				ifp->if_ierrors++;
				goto fail;
			}
#endif

			/*
			 * Trim FCS!
			 * NPE always adds the FCS by this driver's setting,
			 * so we always trim it here and not add M_HASFCS.
			 */
			m_adj(mrx, -ETHER_CRC_LEN);

			ifp->if_ipackets++;
			/*
			 * Tap off here if there is a bpf listener.
			 */
			bpf_mtap(ifp, mrx);
			ifp->if_input(ifp, mrx);
		} else {
fail:
			/* discard frame and re-use mbuf */
			m = npe->ix_m;
		}
		if (npe_rxbuf_init(sc, npe, m) == 0) {
			/* return npe buf to rx free list */
			ixpqmgr_qwrite(sc->rx_freeqid, npe->ix_neaddr);
		} else {
			/* XXX should not happen */
		}
	}
#undef P2V
}

static void
npe_startxmit(struct npe_softc *sc)
{
	struct npedma *dma = &sc->txdma;
	int i;

	sc->tx_free = NULL;
	for (i = 0; i < dma->nbuf; i++) {
		struct npebuf *npe = &dma->buf[i];
		if (npe->ix_m != NULL) {
			/* NB: should not happen */
			printf("%s: %s: free mbuf at entry %u\n",
			    device_xname(sc->sc_dev), __func__, i);
			m_freem(npe->ix_m);
		}
		npe->ix_m = NULL;
		npe->ix_next = sc->tx_free;
		sc->tx_free = npe;
	}
}

static void
npe_startrecv(struct npe_softc *sc)
{
	struct npedma *dma = &sc->rxdma;
	struct npebuf *npe;
	int i;

	for (i = 0; i < dma->nbuf; i++) {
		npe = &dma->buf[i];
		npe_rxbuf_init(sc, npe, npe->ix_m);
		/* set npe buf on rx free list */
		ixpqmgr_qwrite(sc->rx_freeqid, npe->ix_neaddr);
	}
}

static void
npeinit_macreg(struct npe_softc *sc)
{

	/*
	 * Reset MAC core.
	 */
	WR4(sc, NPE_MAC_CORE_CNTRL, NPE_CORE_RESET);
	DELAY(NPE_MAC_RESET_DELAY);
	/* configure MAC to generate MDC clock */
	WR4(sc, NPE_MAC_CORE_CNTRL, NPE_CORE_MDC_EN);

	/* disable transmitter and reciver in the MAC */
 	WR4(sc, NPE_MAC_RX_CNTRL1,
	    RD4(sc, NPE_MAC_RX_CNTRL1) &~ NPE_RX_CNTRL1_RX_EN);
 	WR4(sc, NPE_MAC_TX_CNTRL1,
	    RD4(sc, NPE_MAC_TX_CNTRL1) &~ NPE_TX_CNTRL1_TX_EN);

	/*
	 * Set the MAC core registers.
	 */
	WR4(sc, NPE_MAC_INT_CLK_THRESH, 0x1);	/* clock ratio: for ipx4xx */
	WR4(sc, NPE_MAC_TX_CNTRL2,	0xf);	/* max retries */
	WR4(sc, NPE_MAC_RANDOM_SEED,	0x8);	/* LFSR back-off seed */
	/* thresholds determined by NPE firmware FS */
	WR4(sc, NPE_MAC_THRESH_P_EMPTY,	0x12);
	WR4(sc, NPE_MAC_THRESH_P_FULL,	0x30);
	WR4(sc, NPE_MAC_BUF_SIZE_TX, NPE_MAC_BUF_SIZE_TX_DEFAULT);
						/* tx fifo threshold (bytes) */
	WR4(sc, NPE_MAC_TX_DEFER,	0x15);	/* for single deferral */
	WR4(sc, NPE_MAC_RX_DEFER,	0x16);	/* deferral on inter-frame gap*/
	WR4(sc, NPE_MAC_TX_TWO_DEFER_1,	0x8);	/* for 2-part deferral */
	WR4(sc, NPE_MAC_TX_TWO_DEFER_2,	0x7);	/* for 2-part deferral */
	WR4(sc, NPE_MAC_SLOT_TIME, NPE_MAC_SLOT_TIME_MII_DEFAULT);
						/* assumes MII mode */
	WR4(sc, NPE_MAC_TX_CNTRL1,
		  NPE_TX_CNTRL1_RETRY		/* retry failed xmits */
		| NPE_TX_CNTRL1_FCS_EN		/* append FCS */
		| NPE_TX_CNTRL1_2DEFER		/* 2-part deferal */
		| NPE_TX_CNTRL1_PAD_EN);	/* pad runt frames */
	/* XXX pad strip? */
	WR4(sc, NPE_MAC_RX_CNTRL1,
		  NPE_RX_CNTRL1_CRC_EN		/* include CRC/FCS */
		| NPE_RX_CNTRL1_PAUSE_EN);	/* ena pause frame handling */
	WR4(sc, NPE_MAC_RX_CNTRL2, 0);
}

static void
npeinit_resetcb(void *xsc)
{
	struct npe_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t msg[2];

	ifp->if_oerrors++;
	npeinit_locked(sc);

	msg[0] = NPE_NOTIFYMACRECOVERYDONE << NPE_MAC_MSGID_SHL
	    | (npeconfig[sc->sc_unit].macport << NPE_MAC_PORTID_SHL);
	msg[1] = 0;
	ixpnpe_sendandrecvmsg(sc->sc_npe, msg, msg);
}

/*
 * Reset and initialize the chip
 */
static void
npeinit_locked(void *xsc)
{
	struct npe_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	/* Cancel any pending I/O. */
	npestop(ifp, 0);

	/* Reset the chip to a known state. */
	npeinit_macreg(sc);
	npe_setmac(sc, CLLADDR(ifp->if_sadl));
	ether_mediachange(ifp);
	npe_setmcast(sc);

	npe_startxmit(sc);
	npe_startrecv(sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;		/* just in case */

	/* enable transmitter and reciver in the MAC */
 	WR4(sc, NPE_MAC_RX_CNTRL1,
	    RD4(sc, NPE_MAC_RX_CNTRL1) | NPE_RX_CNTRL1_RX_EN);
 	WR4(sc, NPE_MAC_TX_CNTRL1,
	    RD4(sc, NPE_MAC_TX_CNTRL1) | NPE_TX_CNTRL1_TX_EN);

	callout_reset(&sc->sc_tick_ch, hz, npe_tick, sc);
}

static int
npeinit(struct ifnet *ifp)
{
	struct npe_softc *sc = ifp->if_softc;
	int s;

	s = splnet();
	npeinit_locked(sc);
	splx(s);

	return (0);
}

/*
 * Defragment an mbuf chain, returning at most maxfrags separate
 * mbufs+clusters.  If this is not possible NULL is returned and
 * the original mbuf chain is left in it's present (potentially
 * modified) state.  We use two techniques: collapsing consecutive
 * mbufs and replacing consecutive mbufs by a cluster.
 */
static __inline struct mbuf *
npe_defrag(struct mbuf *m0)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (NULL);
	M_COPY_PKTHDR(m, m0);

	if ((m->m_len = m0->m_pkthdr.len) > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return (NULL);
		}
	}

	m_copydata(m0, 0, m0->m_pkthdr.len, mtod(m, void *));
	m_freem(m0);

	return (m);
}

/*
 * Dequeue packets and place on the h/w transmit queue.
 */
static void
npestart(struct ifnet *ifp)
{
	struct npe_softc *sc = ifp->if_softc;
	struct npebuf *npe;
	struct npehwbuf *hw;
	struct mbuf *m, *n;
	bus_dma_segment_t *segs;
	int nseg, len, error, i;
	uint32_t next;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	while (sc->tx_free != NULL) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;
		npe = sc->tx_free;
		error = bus_dmamap_load_mbuf(sc->sc_dt, npe->ix_map, m,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
		if (error == EFBIG) {
			n = npe_defrag(m);
			if (n == NULL) {
				printf("%s: %s: too many fragments\n",
				    device_xname(sc->sc_dev), __func__);
				m_freem(m);
				return;	/* XXX? */
			}
			m = n;
			error = bus_dmamap_load_mbuf(sc->sc_dt, npe->ix_map,
			    m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
		}
		if (error != 0) {
			printf("%s: %s: error %u\n",
			    device_xname(sc->sc_dev), __func__, error);
			m_freem(m);
			return;	/* XXX? */
		}
		sc->tx_free = npe->ix_next;

		/*
		 * Tap off here if there is a bpf listener.
		 */
		bpf_mtap(ifp, m);

		bus_dmamap_sync(sc->sc_dt, npe->ix_map, 0,
		    npe->ix_map->dm_mapsize, BUS_DMASYNC_PREWRITE);

		npe->ix_m = m;
		hw = npe->ix_hw;
		len = m->m_pkthdr.len;
		nseg = npe->ix_map->dm_nsegs;
		segs = npe->ix_map->dm_segs;
		next = npe->ix_neaddr + sizeof(hw->ix_ne[0]);
		for (i = 0; i < nseg; i++) {
			hw->ix_ne[i].data = htobe32(segs[i].ds_addr);
			hw->ix_ne[i].len = htobe32((segs[i].ds_len<<16) | len);
			hw->ix_ne[i].next = htobe32(next);

			len = 0;		/* zero for segments > 1 */
			next += sizeof(hw->ix_ne[0]);
		}
		hw->ix_ne[i-1].next = 0;	/* zero last in chain */
		/* XXX flush descriptor instead of using uncached memory */

		DPRINTF(sc, "%s: qwrite(%u, 0x%x) ne_data %x ne_len 0x%x\n",
		    __func__, sc->tx_qid, npe->ix_neaddr,
		    hw->ix_ne[0].data, hw->ix_ne[0].len);
		/* stick it on the tx q */
		/* XXX add vlan priority */
		ixpqmgr_qwrite(sc->tx_qid, npe->ix_neaddr);

		ifp->if_timer = 5;
	}
	if (sc->tx_free == NULL)
		ifp->if_flags |= IFF_OACTIVE;
}

static void
npe_stopxmit(struct npe_softc *sc)
{
	struct npedma *dma = &sc->txdma;
	int i;

	/* XXX qmgr */
	for (i = 0; i < dma->nbuf; i++) {
		struct npebuf *npe = &dma->buf[i];

		if (npe->ix_m != NULL) {
			bus_dmamap_unload(sc->sc_dt, npe->ix_map);
			m_freem(npe->ix_m);
			npe->ix_m = NULL;
		}
	}
}

static void
npe_stoprecv(struct npe_softc *sc)
{
	struct npedma *dma = &sc->rxdma;
	int i;

	/* XXX qmgr */
	for (i = 0; i < dma->nbuf; i++) {
		struct npebuf *npe = &dma->buf[i];

		if (npe->ix_m != NULL) {
			bus_dmamap_unload(sc->sc_dt, npe->ix_map);
			m_freem(npe->ix_m);
			npe->ix_m = NULL;
		}
	}
}

/*
 * Turn off interrupts, and stop the nic.
 */
void
npestop(struct ifnet *ifp, int disable)
{
	struct npe_softc *sc = ifp->if_softc;

	/*  disable transmitter and reciver in the MAC  */
 	WR4(sc, NPE_MAC_RX_CNTRL1,
	    RD4(sc, NPE_MAC_RX_CNTRL1) &~ NPE_RX_CNTRL1_RX_EN);
 	WR4(sc, NPE_MAC_TX_CNTRL1,
	    RD4(sc, NPE_MAC_TX_CNTRL1) &~ NPE_TX_CNTRL1_TX_EN);

	callout_stop(&sc->sc_tick_ch);

	npe_stopxmit(sc);
	npe_stoprecv(sc);
	/* XXX go into loopback & drain q's? */
	/* XXX but beware of disabling tx above */

	/*
	 * The MAC core rx/tx disable may leave the MAC hardware in an
	 * unpredictable state. A hw reset is executed before resetting 
	 * all the MAC parameters to a known value.
	 */
	WR4(sc, NPE_MAC_CORE_CNTRL, NPE_CORE_RESET);
	DELAY(NPE_MAC_RESET_DELAY);
	WR4(sc, NPE_MAC_INT_CLK_THRESH, NPE_MAC_INT_CLK_THRESH_DEFAULT);
	WR4(sc, NPE_MAC_CORE_CNTRL, NPE_CORE_MDC_EN);

	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

void
npewatchdog(struct ifnet *ifp)
{
	struct npe_softc *sc = ifp->if_softc;
	int s;

	aprint_error_dev(sc->sc_dev, "device timeout\n");
	s = splnet();
	ifp->if_oerrors++;
	npeinit_locked(sc);
	splx(s);
}

static int
npeioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct npe_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *) data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
#if 0 /* not yet */
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
			sc->sc_flowflags = ifr->ifr_media & IFM_ETH_FMASK;
		}
#endif
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			/*
			 * If interface is marked down and it is running,
			 * then stop and disable it.
			 */
			(*ifp->if_stop)(ifp, 1);
		} else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			error = (*ifp->if_init)(ifp);
		} else if ((ifp->if_flags & IFF_UP) != 0) {
			int diff;

			/* Up (AND RUNNING). */

			diff = (ifp->if_flags ^ sc->sc_if_flags)
			    & (IFF_PROMISC|IFF_ALLMULTI);
			if ((diff & (IFF_PROMISC|IFF_ALLMULTI)) != 0) {
				/*
				 * If the difference bettween last flag and
				 * new flag only IFF_PROMISC or IFF_ALLMULTI,
				 * set multicast filter only (don't reset to
				 * prevent link down).
				 */
				npe_setmcast(sc);
			} else {
				/*
				 * Reset the interface to pick up changes in
				 * any other flags that affect the hardware
				 * state.
				 */
				error = (*ifp->if_init)(ifp);
			}
		}
		sc->sc_if_flags = ifp->if_flags;
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			npe_setmcast(sc);
			error = 0;
		}
	}

	npestart(ifp);

	splx(s);
	return error;
}

/*
 * Setup a traffic class -> rx queue mapping.
 */
static int
npe_setrxqosentry(struct npe_softc *sc, int classix, int trafclass, int qid)
{
	int npeid = npeconfig[sc->sc_unit].npeid;
	uint32_t msg[2];

	msg[0] = (NPE_SETRXQOSENTRY << NPE_MAC_MSGID_SHL) | (npeid << 20)
	    | classix;
	msg[1] = (trafclass << 24) | (1 << 23) | (qid << 16) | (qid << 4);
	return ixpnpe_sendandrecvmsg(sc->sc_npe, msg, msg);
}

/*
 * Update and reset the statistics in the NPE.
 */
static int
npe_updatestats(struct npe_softc *sc)
{
	uint32_t msg[2];

	msg[0] = NPE_RESETSTATS << NPE_MAC_MSGID_SHL;
	msg[1] = sc->sc_stats_phys;	/* physical address of stat block */
	return ixpnpe_sendmsg(sc->sc_npe, msg);		/* NB: no recv */
}

#if 0
/*
 * Get the current statistics block.
 */
static int
npe_getstats(struct npe_softc *sc)
{
	uint32_t msg[2];

	msg[0] = NPE_GETSTATS << NPE_MAC_MSGID_SHL;
	msg[1] = sc->sc_stats_phys;	/* physical address of stat block */
	return ixpnpe_sendandrecvmsg(sc->sc_npe, msg, msg);
}

/*
 * Query the image id of the loaded firmware.
 */
static uint32_t
npe_getimageid(struct npe_softc *sc)
{
	uint32_t msg[2];

	msg[0] = NPE_GETSTATUS << NPE_MAC_MSGID_SHL;
	msg[1] = 0;
	return ixpnpe_sendandrecvmsg(sc->sc_npe, msg, msg) == 0 ? msg[1] : 0;
}

/*
 * Enable/disable loopback.
 */
static int
npe_setloopback(struct npe_softc *sc, int ena)
{
	uint32_t msg[2];

	msg[0] = (NPE_SETLOOPBACK << NPE_MAC_MSGID_SHL) | (ena != 0);
	msg[1] = 0;
	return ixpnpe_sendandrecvmsg(sc->sc_npe, msg, msg);
}
#endif

/*
 * MII bus support routines.
 *
 * NB: ixp425 has one PHY per NPE
 */
static uint32_t
npe_mii_mdio_read(struct npe_softc *sc, int reg)
{
#define	MII_RD4(sc, reg)	bus_space_read_4(sc->sc_iot, sc->sc_miih, reg)
	uint32_t v;

	/* NB: registers are known to be sequential */
	v =  (MII_RD4(sc, reg+0) & 0xff) << 0;
	v |= (MII_RD4(sc, reg+4) & 0xff) << 8;
	v |= (MII_RD4(sc, reg+8) & 0xff) << 16;
	v |= (MII_RD4(sc, reg+12) & 0xff) << 24;
	return v;
#undef MII_RD4
}

static void
npe_mii_mdio_write(struct npe_softc *sc, int reg, uint32_t cmd)
{
#define	MII_WR4(sc, reg, v) \
	bus_space_write_4(sc->sc_iot, sc->sc_miih, reg, v)

	/* NB: registers are known to be sequential */
	MII_WR4(sc, reg+0, cmd & 0xff);
	MII_WR4(sc, reg+4, (cmd >> 8) & 0xff);
	MII_WR4(sc, reg+8, (cmd >> 16) & 0xff);
	MII_WR4(sc, reg+12, (cmd >> 24) & 0xff);
#undef MII_WR4
}

static int
npe_mii_mdio_wait(struct npe_softc *sc)
{
#define	MAXTRIES	100	/* XXX */
	uint32_t v;
	int i;

	for (i = 0; i < MAXTRIES; i++) {
		v = npe_mii_mdio_read(sc, NPE_MAC_MDIO_CMD);
		if ((v & NPE_MII_GO) == 0)
			return 1;
	}
	return 0;		/* NB: timeout */
#undef MAXTRIES
}

static int
npe_miibus_readreg(device_t self, int phy, int reg)
{
	struct npe_softc *sc = device_private(self);
	uint32_t v;

	if (sc->sc_phy > IXPNPECF_PHY_DEFAULT && phy != sc->sc_phy)
		return 0xffff;
	v = (phy << NPE_MII_ADDR_SHL) | (reg << NPE_MII_REG_SHL)
	  | NPE_MII_GO;
	npe_mii_mdio_write(sc, NPE_MAC_MDIO_CMD, v);
	if (npe_mii_mdio_wait(sc))
		v = npe_mii_mdio_read(sc, NPE_MAC_MDIO_STS);
	else
		v = 0xffff | NPE_MII_READ_FAIL;
	return (v & NPE_MII_READ_FAIL) ? 0xffff : (v & 0xffff);
#undef MAXTRIES
}

static void
npe_miibus_writereg(device_t self, int phy, int reg, int data)
{
	struct npe_softc *sc = device_private(self);
	uint32_t v;

	if (sc->sc_phy > IXPNPECF_PHY_DEFAULT && phy != sc->sc_phy)
		return;
	v = (phy << NPE_MII_ADDR_SHL) | (reg << NPE_MII_REG_SHL)
	  | data | NPE_MII_WRITE
	  | NPE_MII_GO;
	npe_mii_mdio_write(sc, NPE_MAC_MDIO_CMD, v);
	/* XXX complain about timeout */
	(void) npe_mii_mdio_wait(sc);
}

static void
npe_miibus_statchg(struct ifnet *ifp)
{
	struct npe_softc *sc = ifp->if_softc;
	uint32_t tx1, rx1;
	uint32_t randoff;

	/* sync MAC duplex state */
	tx1 = RD4(sc, NPE_MAC_TX_CNTRL1);
	rx1 = RD4(sc, NPE_MAC_RX_CNTRL1);
	if (sc->sc_mii.mii_media_active & IFM_FDX) {
		WR4(sc, NPE_MAC_SLOT_TIME, NPE_MAC_SLOT_TIME_MII_DEFAULT);
		tx1 &= ~NPE_TX_CNTRL1_DUPLEX;
		rx1 |= NPE_RX_CNTRL1_PAUSE_EN;
	} else {
		struct timeval now;
		getmicrotime(&now);
		randoff = (RD4(sc, NPE_MAC_UNI_ADDR_6) ^ now.tv_usec)
		    & 0x7f;
		WR4(sc, NPE_MAC_SLOT_TIME, NPE_MAC_SLOT_TIME_MII_DEFAULT
		    + randoff);
		tx1 |= NPE_TX_CNTRL1_DUPLEX;
		rx1 &= ~NPE_RX_CNTRL1_PAUSE_EN;
	}
	WR4(sc, NPE_MAC_RX_CNTRL1, rx1);
	WR4(sc, NPE_MAC_TX_CNTRL1, tx1);
}
