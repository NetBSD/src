/*	$NetBSD: if_emac.c,v 1.22.6.4 2008/01/21 09:38:21 yamt Exp $	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Jason Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_emac.c,v 1.22.6.4 2008/01/21 09:38:21 yamt Exp $");

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <uvm/uvm_extern.h>		/* for PAGE_SIZE */

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <powerpc/ibm4xx/dev/opbvar.h>

#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/mal405gp.h>
#include <powerpc/ibm4xx/dcr405gp.h>
#include <powerpc/ibm4xx/dev/emacreg.h>
#include <powerpc/ibm4xx/dev/if_emacreg.h>

#include <dev/mii/miivar.h>

/*
 * Transmit descriptor list size.  There are two Tx channels, each with
 * up to 256 hardware descriptors available.  We currently use one Tx
 * channel.  We tell the upper layers that they can queue a lot of
 * packets, and we go ahead and manage up to 64 of them at a time.  We
 * allow up to 16 DMA segments per packet.
 */
#define	EMAC_NTXSEGS		16
#define	EMAC_TXQUEUELEN		64
#define	EMAC_TXQUEUELEN_MASK	(EMAC_TXQUEUELEN - 1)
#define	EMAC_TXQUEUE_GC		(EMAC_TXQUEUELEN / 4)
#define	EMAC_NTXDESC		256
#define	EMAC_NTXDESC_MASK	(EMAC_NTXDESC - 1)
#define	EMAC_NEXTTX(x)		(((x) + 1) & EMAC_NTXDESC_MASK)
#define	EMAC_NEXTTXS(x)		(((x) + 1) & EMAC_TXQUEUELEN_MASK)

/*
 * Receive descriptor list size.  There is one Rx channel with up to 256
 * hardware descriptors available.  We allocate 64 receive descriptors,
 * each with a 2k buffer (MCLBYTES).
 */
#define	EMAC_NRXDESC		64
#define	EMAC_NRXDESC_MASK	(EMAC_NRXDESC - 1)
#define	EMAC_NEXTRX(x)		(((x) + 1) & EMAC_NRXDESC_MASK)
#define	EMAC_PREVRX(x)		(((x) - 1) & EMAC_NRXDESC_MASK)

/*
 * Transmit/receive descriptors that are DMA'd to the EMAC.
 */
struct emac_control_data {
	struct mal_descriptor ecd_txdesc[EMAC_NTXDESC];
	struct mal_descriptor ecd_rxdesc[EMAC_NRXDESC];
};

#define	EMAC_CDOFF(x)		offsetof(struct emac_control_data, x)
#define	EMAC_CDTXOFF(x)		EMAC_CDOFF(ecd_txdesc[(x)])
#define	EMAC_CDRXOFF(x)		EMAC_CDOFF(ecd_rxdesc[(x)])

/*
 * Software state for transmit jobs.
 */
struct emac_txsoft {
	struct mbuf *txs_mbuf;		/* head of mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	int txs_ndesc;			/* # of descriptors used */
};

/*
 * Software state for receive descriptors.
 */
struct emac_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

/*
 * Software state per device.
 */
struct emac_softc {
	struct device sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* ethernet common data */
	void *sc_sdhook;		/* shutdown hook */
	void *sc_powerhook;		/* power management hook */

	struct mii_data sc_mii;		/* MII/media information */
	struct callout sc_callout;	/* tick callout */

	u_int32_t sc_mr1;		/* copy of Mode Register 1 */

	bus_dmamap_t sc_cddmamap;	/* control data dma map */
#define	sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	/* Software state for transmit/receive descriptors. */
	struct emac_txsoft sc_txsoft[EMAC_TXQUEUELEN];
	struct emac_rxsoft sc_rxsoft[EMAC_NRXDESC];

	/* Control data structures. */
	struct emac_control_data *sc_control_data;
#define	sc_txdescs	sc_control_data->ecd_txdesc
#define	sc_rxdescs	sc_control_data->ecd_rxdesc

#ifdef EMAC_EVENT_COUNTERS
	struct evcnt sc_ev_rxintr;	/* Rx interrupts */
	struct evcnt sc_ev_txintr;	/* Tx interrupts */
	struct evcnt sc_ev_rxde;	/* Rx descriptor interrupts */
	struct evcnt sc_ev_txde;	/* Tx descriptor interrupts */
	struct evcnt sc_ev_wol;		/* Wake-On-Lan interrupts */
	struct evcnt sc_ev_serr;	/* MAL system error interrupts */
	struct evcnt sc_ev_intr;	/* General EMAC interrupts */

	struct evcnt sc_ev_txreap;	/* Calls to Tx descriptor reaper */
	struct evcnt sc_ev_txsstall;	/* Tx stalled due to no txs */
	struct evcnt sc_ev_txdstall;	/* Tx stalled due to no txd */
	struct evcnt sc_ev_txdrop;	/* Tx packets dropped (too many segs) */
	struct evcnt sc_ev_tu;		/* Tx underrun */
#endif /* EMAC_EVENT_COUNTERS */

	int sc_txfree;			/* number of free Tx descriptors */
	int sc_txnext;			/* next ready Tx descriptor */

	int sc_txsfree;			/* number of free Tx jobs */
	int sc_txsnext;			/* next ready Tx job */
	int sc_txsdirty;		/* dirty Tx jobs */

	int sc_rxptr;			/* next ready RX descriptor/descsoft */
};

#ifdef EMAC_EVENT_COUNTERS
#define	EMAC_EVCNT_INCR(ev)	(ev)->ev_count++
#else
#define	EMAC_EVCNT_INCR(ev)	/* nothing */
#endif

#define	EMAC_CDTXADDR(sc, x)	((sc)->sc_cddma + EMAC_CDTXOFF((x)))
#define	EMAC_CDRXADDR(sc, x)	((sc)->sc_cddma + EMAC_CDRXOFF((x)))

#define	EMAC_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > EMAC_NTXDESC) {				\
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,	\
		    EMAC_CDTXOFF(__x), sizeof(struct mal_descriptor) *	\
		    (EMAC_NTXDESC - __x), (ops));			\
		__n -= (EMAC_NTXDESC - __x);				\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    EMAC_CDTXOFF(__x), sizeof(struct mal_descriptor) * __n, (ops)); \
} while (/*CONSTCOND*/0)

#define	EMAC_CDRXSYNC(sc, x, ops)					\
do {									\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    EMAC_CDRXOFF((x)), sizeof(struct mal_descriptor), (ops));	\
} while (/*CONSTCOND*/0)

#define	EMAC_INIT_RXDESC(sc, x)						\
do {									\
	struct emac_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	struct mal_descriptor *__rxd = &(sc)->sc_rxdescs[(x)];		\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
									\
	/*								\
	 * Note: We scoot the packet forward 2 bytes in the buffer	\
	 * so that the payload after the Ethernet header is aligned	\
	 * to a 4-byte boundary.					\
	 */								\
	__m->m_data = __m->m_ext.ext_buf + 2;				\
									\
	__rxd->md_data = __rxs->rxs_dmamap->dm_segs[0].ds_addr + 2;	\
	__rxd->md_data_len = __m->m_ext.ext_size - 2;			\
	__rxd->md_stat_ctrl = MAL_RX_EMPTY | MAL_RX_INTERRUPT |		\
	    /* Set wrap on last descriptor. */				\
	    (((x) == EMAC_NRXDESC - 1) ? MAL_RX_WRAP : 0);		\
	EMAC_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
} while (/*CONSTCOND*/0)

#define	EMAC_WRITE(sc, reg, val) \
	bus_space_write_stream_4((sc)->sc_st, (sc)->sc_sh, (reg), (val))
#define	EMAC_READ(sc, reg) \
	bus_space_read_stream_4((sc)->sc_st, (sc)->sc_sh, (reg))

#define	EMAC_SET_FILTER(aht, category) \
do {									\
	(aht)[3 - ((category) >> 4)] |= 1 << ((category) & 0xf);	\
} while (/*CONSTCOND*/0)

static int	emac_match(struct device *, struct cfdata *, void *);
static void	emac_attach(struct device *, struct device *, void *);

static int	emac_add_rxbuf(struct emac_softc *, int);
static int	emac_init(struct ifnet *);
static int	emac_ioctl(struct ifnet *, u_long, void *);
static void	emac_reset(struct emac_softc *);
static void	emac_rxdrain(struct emac_softc *);
static int	emac_txreap(struct emac_softc *);
static void	emac_shutdown(void *);
static void	emac_start(struct ifnet *);
static void	emac_stop(struct ifnet *, int);
static void	emac_watchdog(struct ifnet *);
static int	emac_set_filter(struct emac_softc *);

static int	emac_wol_intr(void *);
static int	emac_serr_intr(void *);
static int	emac_txeob_intr(void *);
static int	emac_rxeob_intr(void *);
static int	emac_txde_intr(void *);
static int	emac_rxde_intr(void *);
static int	emac_intr(void *);

static int	emac_mii_readreg(struct device *, int, int);
static void	emac_mii_statchg(struct device *);
static void	emac_mii_tick(void *);
static uint32_t	emac_mii_wait(struct emac_softc *);
static void	emac_mii_writereg(struct device *, int, int, int);

int		emac_copy_small = 0;

CFATTACH_DECL(emac, sizeof(struct emac_softc),
    emac_match, emac_attach, NULL, NULL);

static int
emac_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	/* match only on-chip ethernet devices */
	if (strcmp(oaa->opb_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

static void
emac_attach(struct device *parent, struct device *self, void *aux)
{
	struct opb_attach_args *oaa = aux;
	struct emac_softc *sc = (struct emac_softc *)self;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mii_data *mii = &sc->sc_mii;
	bus_dma_segment_t seg;
	int error, i, nseg;
	const uint8_t *enaddr;
	prop_data_t ea;

	bus_space_map(oaa->opb_bt, oaa->opb_addr, EMAC_NREG, 0, &sc->sc_sh);
	sc->sc_st = oaa->opb_bt;
	sc->sc_dmat = oaa->opb_dmat;

	printf(": 405GP EMAC\n");

	/*
	 * Set up Mode Register 1 - set receive and transmit FIFOs to maximum
	 * size, allow transmit of multiple packets (only channel 0 is used).
	 *
	 * XXX: Allow pause packets??
	 */
	sc->sc_mr1 = MR1_RFS_4KB | MR1_TFS_2KB | MR1_TR0_MULTIPLE;

	intr_establish(oaa->opb_irq    , IST_LEVEL, IPL_NET, emac_wol_intr, sc);
	intr_establish(oaa->opb_irq + 1, IST_LEVEL, IPL_NET, emac_serr_intr, sc);
	intr_establish(oaa->opb_irq + 2, IST_LEVEL, IPL_NET, emac_txeob_intr, sc);
	intr_establish(oaa->opb_irq + 3, IST_LEVEL, IPL_NET, emac_rxeob_intr, sc);
	intr_establish(oaa->opb_irq + 4, IST_LEVEL, IPL_NET, emac_txde_intr, sc);
	intr_establish(oaa->opb_irq + 5, IST_LEVEL, IPL_NET, emac_rxde_intr, sc);
	intr_establish(oaa->opb_irq + 6, IST_LEVEL, IPL_NET, emac_intr, sc);
	printf("%s: interrupting at irqs %d .. %d\n", sc->sc_dev.dv_xname,
	    oaa->opb_irq, oaa->opb_irq + 6);

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct emac_control_data), 0, 0, &seg, 1, &nseg, 0)) != 0) {
		printf("%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, nseg,
	    sizeof(struct emac_control_data), (void **)&sc->sc_control_data,
	    BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct emac_control_data), 1,
	    sizeof(struct emac_control_data), 0, 0, &sc->sc_cddmamap)) != 0) {
		printf("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct emac_control_data), NULL,
	    0)) != 0) {
		printf("%s: unable to load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_3;
	}

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < EMAC_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    EMAC_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			printf("%s: unable to create tx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_4;
		}
	}

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < EMAC_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			printf("%s: unable to create rx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	/*
	 * Reset the chip to a known state.
	 */
	emac_reset(sc);

	/* Fetch the Ethernet address. */
	ea = prop_dictionary_get(device_properties(&sc->sc_dev), "mac-addr");
	if (ea == NULL) {
		printf("%s: unable to get mac-addr property\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
	KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
	enaddr = prop_data_data_nocopy(ea);

	printf("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr));

	/*
	 * Initialise the media structures.
	 */
	mii->mii_ifp = ifp;
	mii->mii_readreg = emac_mii_readreg;
	mii->mii_writereg = emac_mii_writereg;
	mii->mii_statchg = emac_mii_statchg;

	sc->sc_ethercom.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
	mii_attach(&sc->sc_dev, mii, 0xffffffff,
	    MII_PHY_ANY, MII_OFFSET_ANY, 0);
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);

	ifp = &sc->sc_ethercom.ec_if;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = emac_ioctl;
	ifp->if_start = emac_start;
	ifp->if_watchdog = emac_watchdog;
	ifp->if_init = emac_init;
	ifp->if_stop = emac_stop;
	IFQ_SET_READY(&ifp->if_snd);

	/*
	 * We can support 802.1Q VLAN-sized frames.
	 */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

#ifdef EMAC_EVENT_COUNTERS
	/*
	 * Attach the event counters.
	 */
	evcnt_attach_dynamic(&sc->sc_ev_rxintr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "rxintr");
	evcnt_attach_dynamic(&sc->sc_ev_txintr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "txintr");
	evcnt_attach_dynamic(&sc->sc_ev_rxde, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "rxde");
	evcnt_attach_dynamic(&sc->sc_ev_txde, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "txde");
	evcnt_attach_dynamic(&sc->sc_ev_wol, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "wol");
	evcnt_attach_dynamic(&sc->sc_ev_serr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "serr");
	evcnt_attach_dynamic(&sc->sc_ev_intr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "intr");

	evcnt_attach_dynamic(&sc->sc_ev_txreap, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txreap");
	evcnt_attach_dynamic(&sc->sc_ev_txsstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txsstall");
	evcnt_attach_dynamic(&sc->sc_ev_txdstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txdstall");
	evcnt_attach_dynamic(&sc->sc_ev_txdrop, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txdrop");
	evcnt_attach_dynamic(&sc->sc_ev_tu, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "tu");
#endif /* EMAC_EVENT_COUNTERS */

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	sc->sc_sdhook = shutdownhook_establish(emac_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);

	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
fail_5:
	for (i = 0; i < EMAC_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
fail_4:
	for (i = 0; i < EMAC_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_control_data,
	    sizeof(struct emac_control_data));
fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, nseg);
fail_0:
	return;
}

/*
 * Device shutdown routine.
 */
static void
emac_shutdown(void *arg)
{
	struct emac_softc *sc = arg;

	emac_stop(&sc->sc_ethercom.ec_if, 0);
}

/* ifnet interface function */
static void
emac_start(struct ifnet *ifp)
{
	struct emac_softc *sc = ifp->if_softc;
	struct mbuf *m0;
	struct emac_txsoft *txs;
	bus_dmamap_t dmamap;
	int error, firsttx, nexttx, lasttx, ofree, seg;

	lasttx = 0;	/* XXX gcc */

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous number of free descriptors.
	 */
	ofree = sc->sc_txfree;

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	for (;;) {
		/* Grab a packet off the queue. */
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		/*
		 * Get a work queue entry.  Reclaim used Tx descriptors if
		 * we are running low.
		 */
		if (sc->sc_txsfree < EMAC_TXQUEUE_GC) {
			emac_txreap(sc);
			if (sc->sc_txsfree == 0) {
				EMAC_EVCNT_INCR(&sc->sc_ev_txsstall);
				break;
			}
		}

		txs = &sc->sc_txsoft[sc->sc_txsnext];
		dmamap = txs->txs_dmamap;

		/*
		 * Load the DMA map.  If this fails, the packet either
		 * didn't fit in the alloted number of segments, or we
		 * were short on resources.  In this case, we'll copy
		 * and try again.
		 */
		error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
		if (error) {
			if (error == EFBIG) {
				EMAC_EVCNT_INCR(&sc->sc_ev_txdrop);
				printf("%s: Tx packet consumes too many "
				    "DMA segments, dropping...\n",
				    sc->sc_dev.dv_xname);
				    IFQ_DEQUEUE(&ifp->if_snd, m0);
				    m_freem(m0);
				    continue;
			}
			/* Short on resources, just stop for now. */
			break;
		}

		/*
		 * Ensure we have enough descriptors free to describe
		 * the packet.
		 */
		if (dmamap->dm_nsegs > sc->sc_txfree) {
			/*
			 * Not enough free descriptors to transmit this
			 * packet.  We haven't committed anything yet,
			 * so just unload the DMA map, put the packet
			 * back on the queue, and punt.  Notify the upper
			 * layer that there are not more slots left.
			 *
			 */
			ifp->if_flags |= IFF_OACTIVE;
			bus_dmamap_unload(sc->sc_dmat, dmamap);
			EMAC_EVCNT_INCR(&sc->sc_ev_txdstall);
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		/* Sync the DMA map. */
		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Store a pointer to the packet so that we can free it
		 * later.
		 */
		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_ndesc = dmamap->dm_nsegs;

		/*
		 * Initialize the transmit descriptor.
		 */
		firsttx = sc->sc_txnext;
		for (nexttx = sc->sc_txnext, seg = 0;
		     seg < dmamap->dm_nsegs;
		     seg++, nexttx = EMAC_NEXTTX(nexttx)) {
			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the TX_READY bit just
			 * yet.  That could cause a race condition.
			 * We'll do it below.
			 */
			sc->sc_txdescs[nexttx].md_data =
			    dmamap->dm_segs[seg].ds_addr;
			sc->sc_txdescs[nexttx].md_data_len =
			    dmamap->dm_segs[seg].ds_len;
			sc->sc_txdescs[nexttx].md_stat_ctrl =
			    (sc->sc_txdescs[nexttx].md_stat_ctrl & MAL_TX_WRAP) |
			    (nexttx == firsttx ? 0 : MAL_TX_READY) |
			    EMAC_TXC_GFCS | EMAC_TXC_GPAD;
			lasttx = nexttx;
		}

		/* Set the LAST bit on the last segment. */
		sc->sc_txdescs[lasttx].md_stat_ctrl |= MAL_TX_LAST;

		/*
		 * Set up last segment descriptor to send an interrupt after
		 * that descriptor is transmitted, and bypass existing Tx
		 * descriptor reaping method (for now...).
		 */
		sc->sc_txdescs[lasttx].md_stat_ctrl |= MAL_TX_INTERRUPT;


		txs->txs_lastdesc = lasttx;

		/* Sync the descriptors we're using. */
		EMAC_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/*
		 * The entire packet chain is set up.  Give the
		 * first descriptor to the chip now.
		 */
		sc->sc_txdescs[firsttx].md_stat_ctrl |= MAL_TX_READY;
		EMAC_CDTXSYNC(sc, firsttx, 1,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
		/*
		 * Tell the EMAC that a new packet is available.
		 */
		EMAC_WRITE(sc, EMAC_TMR0, TMR0_GNP0);

		/* Advance the tx pointer. */
		sc->sc_txfree -= txs->txs_ndesc;
		sc->sc_txnext = nexttx;

		sc->sc_txsfree--;
		sc->sc_txsnext = EMAC_NEXTTXS(sc->sc_txsnext);

#if NBPFILTER > 0
		/*
		 * Pass the packet to any BPF listeners.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif /* NBPFILTER > 0 */
	}

	if (sc->sc_txfree == 0) {
		/* No more slots left; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}

	if (sc->sc_txfree != ofree) {
		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

static int
emac_init(struct ifnet *ifp)
{
	struct emac_softc *sc = ifp->if_softc;
	struct emac_rxsoft *rxs;
	const uint8_t *enaddr = CLLADDR(ifp->if_sadl);
	int error, i;

	error = 0;

	/* Cancel any pending I/O. */
	emac_stop(ifp, 0);

	/* Reset the chip to a known state. */
	emac_reset(sc);

	/* 
	 * Initialise the transmit descriptor ring.
	 */
	memset(sc->sc_txdescs, 0, sizeof(sc->sc_txdescs));
	/* set wrap on last descriptor */
	sc->sc_txdescs[EMAC_NTXDESC - 1].md_stat_ctrl |= MAL_TX_WRAP;
	EMAC_CDTXSYNC(sc, 0, EMAC_NTXDESC,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	sc->sc_txfree = EMAC_NTXDESC;
	sc->sc_txnext = 0;

	/*
	 * Initialise the transmit job descriptors.
	 */
	for (i = 0; i < EMAC_TXQUEUELEN; i++)
		sc->sc_txsoft[i].txs_mbuf = NULL;
	sc->sc_txsfree = EMAC_TXQUEUELEN;
	sc->sc_txsnext = 0;
	sc->sc_txsdirty = 0;

	/*
	 * Initialise the receiver descriptor and receive job
	 * descriptor rings.
	 */
	for (i = 0; i < EMAC_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf == NULL) {
			if ((error = emac_add_rxbuf(sc, i)) != 0) {
				printf("%s: unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    sc->sc_dev.dv_xname, i, error);
				/*
				 * XXX Should attempt to run with fewer receive
				 * XXX buffers instead of just failing.
				 */
				emac_rxdrain(sc);
				goto out;
			}
		} else
			EMAC_INIT_RXDESC(sc, i);
	}
	sc->sc_rxptr = 0;

	/*
	 * Set the current media.
	 */
	if ((error = ether_mediachange(ifp)) != 0)
		goto out;

	/*
	 * Give the transmit and receive rings to the MAL.
	 */
	mtdcr(DCR_MAL0_TXCTP0R, EMAC_CDTXADDR(sc, 0));
	mtdcr(DCR_MAL0_RXCTP0R, EMAC_CDRXADDR(sc, 0));

	/*
	 * Load the MAC address.
	 */
	EMAC_WRITE(sc, EMAC_IAHR, enaddr[0] << 8 | enaddr[1]);
	EMAC_WRITE(sc, EMAC_IALR,
	    enaddr[2] << 24 | enaddr[3] << 16 | enaddr[4] << 8 | enaddr[5]);

	/*
	 * Set the receive channel buffer size (in units of 16 bytes).
	 */
#if MCLBYTES > (4096 - 16)	/* XXX! */
# error	MCLBYTES > max rx channel buffer size
#endif
	mtdcr(DCR_MAL0_RCBS0, MCLBYTES / 16);

	/* Set fifos, media modes. */
	EMAC_WRITE(sc, EMAC_MR1, sc->sc_mr1);

	/*
	 * Enable Individual and (possibly) Broadcast Address modes,
	 * runt packets, and strip padding.
	 */
	EMAC_WRITE(sc, EMAC_RMR, RMR_IAE | RMR_RRP | RMR_SP |
	    (ifp->if_flags & IFF_PROMISC ? RMR_PME : 0) |
	    (ifp->if_flags & IFF_BROADCAST ? RMR_BAE : 0));

	/*
	 * Set multicast filter.
	 */
	emac_set_filter(sc);

	/*
	 * Set low- and urgent-priority request thresholds.
	 */
	EMAC_WRITE(sc, EMAC_TMR1,
	    ((7 << TMR1_TLR_SHIFT) & TMR1_TLR_MASK) | /* 16 word burst */
	    ((15 << TMR1_TUR_SHIFT) & TMR1_TUR_MASK));
	/*
	 * Set Transmit Request Threshold Register.
	 */
	EMAC_WRITE(sc, EMAC_TRTR, TRTR_256);

	/*
	 * Set high and low receive watermarks.
	 */
	EMAC_WRITE(sc, EMAC_RWMR,
	    30 << RWMR_RLWM_SHIFT | 64 << RWMR_RLWM_SHIFT);

	/*
	 * Set frame gap.
	 */
	EMAC_WRITE(sc, EMAC_IPGVR, 8);

	/*
	 * Set interrupt status enable bits for EMAC and MAL.
	 */
	EMAC_WRITE(sc, EMAC_ISER,
	    ISR_BP | ISR_SE | ISR_ALE | ISR_BFCS | ISR_PTLE | ISR_ORE | ISR_IRE);
	mtdcr(DCR_MAL0_IER, MAL0_IER_DE | MAL0_IER_NWE | MAL0_IER_TO |
	    MAL0_IER_OPB | MAL0_IER_PLB);

	/*
	 * Enable the transmit and receive channel on the MAL.
	 */
	mtdcr(DCR_MAL0_RXCASR, MAL0_RXCASR_CHAN0);
	mtdcr(DCR_MAL0_TXCASR, MAL0_TXCASR_CHAN0);

	/*
	 * Enable the transmit and receive channel on the EMAC.
	 */
	EMAC_WRITE(sc, EMAC_MR0, MR0_TXE | MR0_RXE);

	/*
	 * Start the one second MII clock.
	 */
	callout_reset(&sc->sc_callout, hz, emac_mii_tick, sc);

	/*
	 * ... all done!
	 */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

 out:
	if (error) {
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		ifp->if_timer = 0;
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
	}
	return (error);
}

static int
emac_add_rxbuf(struct emac_softc *sc, int idx)
{
	struct emac_rxsoft *rxs = &sc->sc_rxsoft[idx];
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)  
		return (ENOBUFS);

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return (ENOBUFS);
	}

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, rxs->rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load rx DMA map %d, error = %d\n",
		    sc->sc_dev.dv_xname, idx, error);
		panic("emac_add_rxbuf");		/* XXX */
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	EMAC_INIT_RXDESC(sc, idx);

	return (0);
}

/* ifnet interface function */
static void
emac_watchdog(struct ifnet *ifp)
{
	struct emac_softc *sc = ifp->if_softc;

	/*
	 * Since we're not interrupting every packet, sweep
	 * up before we report an error.
	 */
	emac_txreap(sc);

	if (sc->sc_txfree != EMAC_NTXDESC) {
		printf("%s: device timeout (txfree %d txsfree %d txnext %d)\n",
		    sc->sc_dev.dv_xname, sc->sc_txfree, sc->sc_txsfree,
		    sc->sc_txnext);
		ifp->if_oerrors++;

		/* Reset the interface. */
		(void)emac_init(ifp);
	} else if (ifp->if_flags & IFF_DEBUG)
		printf("%s: recovered from device timeout\n",
		    sc->sc_dev.dv_xname);

	/* try to get more packets going */
	emac_start(ifp);
}

static void
emac_rxdrain(struct emac_softc *sc)
{
	struct emac_rxsoft *rxs;
	int i;

	for (i = 0; i < EMAC_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

/* ifnet interface function */
static void
emac_stop(struct ifnet *ifp, int disable)
{
	struct emac_softc *sc = ifp->if_softc;
	struct emac_txsoft *txs;
	int i;

	/* Stop the one second clock. */
	callout_stop(&sc->sc_callout);

	/* Down the MII */
	mii_down(&sc->sc_mii);

	/* Disable interrupts. */
#if 0	/* Can't disable MAL interrupts without a reset... */
	EMAC_WRITE(sc, EMAC_ISER, 0);
#endif
	mtdcr(DCR_MAL0_IER, 0);

	/* Disable the receive and transmit channels. */
	mtdcr(DCR_MAL0_RXCARR, MAL0_RXCARR_CHAN0);
	mtdcr(DCR_MAL0_TXCARR, MAL0_TXCARR_CHAN0 | MAL0_TXCARR_CHAN1);

	/* Disable the transmit enable and receive MACs. */
	EMAC_WRITE(sc, EMAC_MR0,
	    EMAC_READ(sc, EMAC_MR0) & ~(MR0_TXE | MR0_RXE));

	/* Release any queued transmit buffers. */
	for (i = 0; i < EMAC_TXQUEUELEN; i++) {
		txs = &sc->sc_txsoft[i];
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
	}

	if (disable)
		emac_rxdrain(sc);

	/*
	 * Mark the interface down and cancel the watchdog timer.
	 */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

/* ifnet interface function */
static int
emac_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct emac_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();

	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET) { 
		/*
		 * Multicast list has changed; set the hardware filter
		 * accordingly.
		 */
		if (ifp->if_flags & IFF_RUNNING)
			error = emac_set_filter(sc);
		else
			error = 0;
	}

	/* try to get more packets going */
	emac_start(ifp);

	splx(s);
	return (error);
}

static void
emac_reset(struct emac_softc *sc)
{

	/* reset the MAL */
	mtdcr(DCR_MAL0_CFG, MAL0_CFG_SR);

	EMAC_WRITE(sc, EMAC_MR0, MR0_SRST);
	delay(5);

	/* XXX: check if MR0_SRST is clear until a timeout instead? */
	EMAC_WRITE(sc, EMAC_MR0, EMAC_READ(sc, EMAC_MR0) & ~MR0_SRST);

	/* XXX clear interrupts in EMAC_ISR just to be sure?? */

	/* set the MAL config register */
	mtdcr(DCR_MAL0_CFG, MAL0_CFG_PLBB | MAL0_CFG_OPBBL | MAL0_CFG_LEA |
	    MAL0_CFG_SD | MAL0_CFG_PLBLT);
}

static int
emac_set_filter(struct emac_softc *sc)
{
	struct ether_multistep step;
	struct ether_multi *enm;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t rmr, crc, gaht[4] = {0, 0, 0, 0};
	int category, cnt = 0;

	rmr = EMAC_READ(sc, EMAC_RMR);
	rmr &= ~(RMR_PMME | RMR_MAE);
	ifp->if_flags &= ~IFF_ALLMULTI;

	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo,
		    enm->enm_addrhi, ETHER_ADDR_LEN) != 0) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			gaht[0] = gaht[1] = gaht[2] = gaht[3] = 0xffff;
			break;
		}

		crc = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);

		/* Just want the 6 most significant bits. */
		category = crc >> 26;
		EMAC_SET_FILTER(gaht, category);

		ETHER_NEXT_MULTI(step, enm);
		cnt++;
	}

	if ((gaht[0] & gaht[1] & gaht[2] & gaht[3]) == 0xffff) {
		/* All categories are true. */
		ifp->if_flags |= IFF_ALLMULTI;
		rmr |= RMR_PMME;
	} else if (cnt != 0) {
		/* Some categories are true. */
		EMAC_WRITE(sc, EMAC_GAHT1, gaht[0]); 
		EMAC_WRITE(sc, EMAC_GAHT2, gaht[1]);
		EMAC_WRITE(sc, EMAC_GAHT3, gaht[2]);
		EMAC_WRITE(sc, EMAC_GAHT4, gaht[3]);

		rmr |= RMR_MAE;
	}
	EMAC_WRITE(sc, EMAC_RMR, rmr);

	return 0;
}

/*
 * EMAC General interrupt handler
 */
static int
emac_intr(void *arg)
{
	struct emac_softc *sc = arg;
	uint32_t status;

	EMAC_EVCNT_INCR(&sc->sc_ev_intr);
	status = EMAC_READ(sc, EMAC_ISR);

	/* Clear the interrupt status bits. */
	EMAC_WRITE(sc, EMAC_ISR, status);

	return (0);
}

/*
 * EMAC Wake-On-LAN interrupt handler
 */
static int
emac_wol_intr(void *arg)
{
	struct emac_softc *sc = arg;

	EMAC_EVCNT_INCR(&sc->sc_ev_wol);
	printf("%s: emac_wol_intr\n", sc->sc_dev.dv_xname);
	return (0);
}

/*
 * MAL System ERRor interrupt handler
 */
static int
emac_serr_intr(void *arg)
{
#ifdef EMAC_EVENT_COUNTERS
	struct emac_softc *sc = arg;
#endif
	u_int32_t esr;

	EMAC_EVCNT_INCR(&sc->sc_ev_serr);
	esr = mfdcr(DCR_MAL0_ESR);

	/* Clear the interrupt status bits. */
	mtdcr(DCR_MAL0_ESR, esr);
	return (0);
}

/*
 * MAL Transmit End-Of-Buffer interrupt handler.
 * NOTE: This shouldn't be called!
 */
static int
emac_txeob_intr(void *arg)
{
	struct emac_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int handled;

	EMAC_EVCNT_INCR(&sc->sc_ev_txintr);
	handled = emac_txreap(arg);

	/* try to get more packets going */
	emac_start(ifp);

	return (handled);
	
}

/*
 * Reap completed Tx descriptors.
 */
static int
emac_txreap(struct emac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct emac_txsoft *txs;
	int handled, i;
	u_int32_t txstat;

	EMAC_EVCNT_INCR(&sc->sc_ev_txreap);
	handled = 0;

	/* Clear the interrupt */
	mtdcr(DCR_MAL0_TXEOBISR, mfdcr(DCR_MAL0_TXEOBISR));

	ifp->if_flags &= ~IFF_OACTIVE;

	/*
	 * Go through our Tx list and free mbufs for those
	 * frames that have been transmitted.
	 */
	for (i = sc->sc_txsdirty; sc->sc_txsfree != EMAC_TXQUEUELEN;
	    i = EMAC_NEXTTXS(i), sc->sc_txsfree++) {
		txs = &sc->sc_txsoft[i];

		EMAC_CDTXSYNC(sc, txs->txs_lastdesc,
		    txs->txs_dmamap->dm_nsegs,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		txstat = sc->sc_txdescs[txs->txs_lastdesc].md_stat_ctrl;
		if (txstat & MAL_TX_READY)
			break;

		handled = 1;

		/*
		 * Check for errors and collisions.
		 */
		if (txstat & (EMAC_TXS_UR | EMAC_TXS_ED))
			ifp->if_oerrors++;

#ifdef EMAC_EVENT_COUNTERS
		if (txstat & EMAC_TXS_UR)
			EMAC_EVCNT_INCR(&sc->sc_ev_tu);
#endif /* EMAC_EVENT_COUNTERS */

		if (txstat & (EMAC_TXS_EC | EMAC_TXS_MC | EMAC_TXS_SC | EMAC_TXS_LC)) {
			if (txstat & EMAC_TXS_EC)
				ifp->if_collisions += 16;
			else if (txstat & EMAC_TXS_MC)
				ifp->if_collisions += 2;	/* XXX? */
			else if (txstat & EMAC_TXS_SC)
				ifp->if_collisions++;
			if (txstat & EMAC_TXS_LC)
				ifp->if_collisions++;
		} else
			ifp->if_opackets++;

		if (ifp->if_flags & IFF_DEBUG) {
			if (txstat & EMAC_TXS_ED)
				printf("%s: excessive deferral\n",
				    sc->sc_dev.dv_xname);
			if (txstat & EMAC_TXS_EC)
				printf("%s: excessive collisions\n",
				    sc->sc_dev.dv_xname);
		}

		sc->sc_txfree += txs->txs_ndesc;
		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
	}

	/* Update the dirty transmit buffer pointer. */
	sc->sc_txsdirty = i;

	/*
	 * If there are no more pending transmissions, cancel the watchdog
	 * timer.
	 */
	if (sc->sc_txsfree == EMAC_TXQUEUELEN)
		ifp->if_timer = 0;

	return (handled);
}

/*
 * MAL Receive End-Of-Buffer interrupt handler
 */
static int
emac_rxeob_intr(void *arg)
{
	struct emac_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct emac_rxsoft *rxs;
	struct mbuf *m;
	u_int32_t rxstat;
	int i, len;

	EMAC_EVCNT_INCR(&sc->sc_ev_rxintr);

	/* Clear the interrupt */
	mtdcr(DCR_MAL0_RXEOBISR, mfdcr(DCR_MAL0_RXEOBISR));

	for (i = sc->sc_rxptr;; i = EMAC_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		EMAC_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		rxstat = sc->sc_rxdescs[i].md_stat_ctrl;

		if (rxstat & MAL_RX_EMPTY)
			/*
			 * We have processed all of the receive buffers.
			 */
			break;

		/*
		 * If an error occurred, update stats, clear the status
		 * word, and leave the packet buffer in place.  It will
		 * simply be reused the next time the ring comes around.
		 */
		if (rxstat & (EMAC_RXS_OE | EMAC_RXS_BP | EMAC_RXS_SE |
		    EMAC_RXS_AE | EMAC_RXS_BFCS | EMAC_RXS_PTL | EMAC_RXS_ORE |
		    EMAC_RXS_IRE)) {
#define	PRINTERR(bit, str)						\
			if (rxstat & (bit))				\
				printf("%s: receive error: %s\n",	\
				    sc->sc_dev.dv_xname, str)
			ifp->if_ierrors++;
			PRINTERR(EMAC_RXS_OE, "overrun error");
			PRINTERR(EMAC_RXS_BP, "bad packet");
			PRINTERR(EMAC_RXS_RP, "runt packet");
			PRINTERR(EMAC_RXS_SE, "short event");
			PRINTERR(EMAC_RXS_AE, "alignment error");
			PRINTERR(EMAC_RXS_BFCS, "bad FCS");
			PRINTERR(EMAC_RXS_PTL, "packet too long");
			PRINTERR(EMAC_RXS_ORE, "out of range error");
			PRINTERR(EMAC_RXS_IRE, "in range error");
#undef PRINTERR
			EMAC_INIT_RXDESC(sc, i);
			continue;
		}

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		/*
		 * No errors; receive the packet.  Note, the 405GP emac
		 * includes the CRC with every packet.
		 */
		len = sc->sc_rxdescs[i].md_data_len - ETHER_CRC_LEN;

		/*
		 * If the packet is small enough to fit in a
		 * single header mbuf, allocate one and copy
		 * the data into it.  This greatly reduces
		 * memory consumption when we receive lots
		 * of small packets.
		 *
		 * Otherwise, we add a new buffer to the receive
		 * chain.  If this fails, we drop the packet and
		 * recycle the old buffer.
		 */
		if (emac_copy_small != 0 && len <= MHLEN) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL)
				goto dropit;
			memcpy(mtod(m, void *),
			    mtod(rxs->rxs_mbuf, void *), len);
			EMAC_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize,
			    BUS_DMASYNC_PREREAD);
		} else {
			m = rxs->rxs_mbuf;
			if (emac_add_rxbuf(sc, i) != 0) {
 dropit:
				ifp->if_ierrors++;
				EMAC_INIT_RXDESC(sc, i);
				bus_dmamap_sync(sc->sc_dmat,
				    rxs->rxs_dmamap, 0,
				    rxs->rxs_dmamap->dm_mapsize,
				    BUS_DMASYNC_PREREAD);
				continue;
			}
		}

		ifp->if_ipackets++;
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

#if NBPFILTER > 0
		/*
		 * Pass this up to any BPF listeners, but only
		 * pass if up the stack if it's for us.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif /* NBPFILTER > 0 */

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* Update the receive pointer. */
	sc->sc_rxptr = i;

	return (0);
}

/*
 * MAL Transmit Descriptor Error interrupt handler
 */
static int
emac_txde_intr(void *arg)
{
	struct emac_softc *sc = arg;

	EMAC_EVCNT_INCR(&sc->sc_ev_txde);
	printf("%s: emac_txde_intr\n", sc->sc_dev.dv_xname);
	return (0);
}

/*
 * MAL Receive Descriptor Error interrupt handler
 */
static int
emac_rxde_intr(void *arg)
{
	int i;
	struct emac_softc *sc = arg;

	EMAC_EVCNT_INCR(&sc->sc_ev_rxde);
	printf("%s: emac_rxde_intr\n", sc->sc_dev.dv_xname);
	/*
	 * XXX!
	 * This is a bit drastic; we just drop all descriptors that aren't
	 * "clean".  We should probably send any that are up the stack.
	 */
	for (i = 0; i < EMAC_NRXDESC; i++) {
		EMAC_CDRXSYNC(sc, i, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		if (sc->sc_rxdescs[i].md_data_len != MCLBYTES) {
			EMAC_INIT_RXDESC(sc, i);
		}

	}

	/* Reenable the receive channel */
	mtdcr(DCR_MAL0_RXCASR, MAL0_RXCASR_CHAN0);

	/* Clear the interrupt */
	mtdcr(DCR_MAL0_RXDEIR, mfdcr(DCR_MAL0_RXDEIR));

	return (0);
}

static uint32_t
emac_mii_wait(struct emac_softc *sc)
{
	int i;
	uint32_t reg;

	/* wait for PHY data transfer to complete */
	i = 0;
	while ((reg = EMAC_READ(sc, EMAC_STACR) & STACR_OC) == 0) {
		delay(7);
		if (i++ > 5) {
			printf("%s: MII timed out\n", sc->sc_dev.dv_xname);
			return (0);
		}
	}
	return (reg);
}

static int
emac_mii_readreg(struct device *self, int phy, int reg)
{
	struct emac_softc *sc = (struct emac_softc *)self;
	uint32_t sta_reg;

	/* wait for PHY data transfer to complete */
	if (emac_mii_wait(sc) == 0)
		return (0);

	sta_reg = reg << STACR_PRASHIFT;
	sta_reg |= STACR_READ;
	sta_reg |= phy << STACR_PCDASHIFT;

	sta_reg &= ~STACR_OPBC_MASK;
	sta_reg |= STACR_OPBC_50MHZ;


	EMAC_WRITE(sc, EMAC_STACR, sta_reg);

	if ((sta_reg = emac_mii_wait(sc)) == 0)
		return (0);
	sta_reg = EMAC_READ(sc, EMAC_STACR);
	if ((sta_reg & STACR_PHYE) != 0)
		return (0);
	return (sta_reg >> STACR_PHYDSHIFT);
}

static void
emac_mii_writereg(struct device *self, int phy, int reg, int val)
{
	struct emac_softc *sc = (struct emac_softc *)self;
	uint32_t sta_reg;

	/* wait for PHY data transfer to complete */
	if (emac_mii_wait(sc) == 0)
		return;

	sta_reg = reg << STACR_PRASHIFT;
	sta_reg |= STACR_WRITE;
	sta_reg |= phy << STACR_PCDASHIFT;

	sta_reg &= ~STACR_OPBC_MASK;
	sta_reg |= STACR_OPBC_50MHZ;

	sta_reg |= val << STACR_PHYDSHIFT;

	EMAC_WRITE(sc, EMAC_STACR, sta_reg);

	if ((sta_reg = emac_mii_wait(sc)) == 0)
		return;
	if ((sta_reg & STACR_PHYE) != 0)
		/* error */
		return;
}

static void
emac_mii_statchg(struct device *self)
{
	struct emac_softc *sc = (void *)self;

	if (sc->sc_mii.mii_media_active & IFM_FDX)
		sc->sc_mr1 |= MR1_FDE;
	else
		sc->sc_mr1 &= ~(MR1_FDE | MR1_EIFC);

	/* XXX 802.1x flow-control? */

	/*
	 * MR1 can only be written immediately after a reset...
	 */
	emac_reset(sc);
}

static void
emac_mii_tick(void *arg)
{
	struct emac_softc *sc = arg;
	int s;

	if (!device_is_active(&sc->sc_dev))
		return;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_callout, hz, emac_mii_tick, sc);
}
