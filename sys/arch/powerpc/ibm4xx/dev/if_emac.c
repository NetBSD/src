/*	$NetBSD: if_emac.c,v 1.41.2.1 2014/08/20 00:03:19 tls Exp $	*/

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

/*
 * emac(4) supports following ibm4xx's EMACs.
 *   XXXX: ZMII and 'TCP Accelaration Hardware' not support yet...
 *
 *            tested
 *            ------
 * 405EP	-  10/100 x2
 * 405EX/EXr	o  10/100/1000 x2 (EXr x1), STA v2, 256bit hash-Table, RGMII
 * 405GP/GPr	o  10/100
 * 440EP	-  10/100 x2, ZMII
 * 440GP	-  10/100 x2, ZMII
 * 440GX	-  10/100/1000 x4, ZMII/RGMII(ch 2, 3), TAH(ch 2, 3)
 * 440SP	-  10/100/1000
 * 440SPe	-  10/100/1000, STA v2
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_emac.c,v 1.41.2.1 2014/08/20 00:03:19 tls Exp $");

#include "opt_emac.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>		/* for PAGE_SIZE */

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/mal405gp.h>
#include <powerpc/ibm4xx/dev/emacreg.h>
#include <powerpc/ibm4xx/dev/if_emacreg.h>
#include <powerpc/ibm4xx/dev/if_emacvar.h>
#include <powerpc/ibm4xx/dev/malvar.h>
#include <powerpc/ibm4xx/dev/opbreg.h>
#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/plbvar.h>
#if defined(EMAC_ZMII_PHY) || defined(EMAC_RGMII_PHY)
#include <powerpc/ibm4xx/dev/rmiivar.h>
#endif

#include <dev/mii/miivar.h>

#include "locators.h"


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
	device_t sc_dev;		/* generic device information */
	int sc_instance;		/* instance no. */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* ethernet common data */
	void *sc_sdhook;		/* shutdown hook */
	void *sc_powerhook;		/* power management hook */

	struct mii_data sc_mii;		/* MII/media information */
	struct callout sc_callout;	/* tick callout */

	uint32_t sc_mr1;		/* copy of Mode Register 1 */
	uint32_t sc_stacr_read;		/* Read opcode of STAOPC of STACR */
	uint32_t sc_stacr_write;	/* Write opcode of STAOPC of STACR */
	uint32_t sc_stacr_bits;		/* misc bits of STACR */
	bool sc_stacr_completed;	/* Operation completed of STACR */
	int sc_htsize;			/* Hash Table size */

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

	void (*sc_rmii_enable)(device_t, int);		/* reduced MII enable */
	void (*sc_rmii_disable)(device_t, int);		/* reduced MII disable*/
	void (*sc_rmii_speed)(device_t, int, int);	/* reduced MII speed */
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

#define	EMAC_SET_FILTER(aht, crc) \
do {									\
	(aht)[3 - (((crc) >> 26) >> 4)] |= 1 << (((crc) >> 26) & 0xf);	\
} while (/*CONSTCOND*/0)
#define	EMAC_SET_FILTER256(aht, crc) \
do {									\
	(aht)[7 - (((crc) >> 24) >> 5)] |= 1 << (((crc) >> 24) & 0x1f);	\
} while (/*CONSTCOND*/0)

static int	emac_match(device_t, cfdata_t, void *);
static void	emac_attach(device_t, device_t, void *);

static int	emac_intr(void *);
static void	emac_shutdown(void *);

static void	emac_start(struct ifnet *);
static int	emac_ioctl(struct ifnet *, u_long, void *);
static int	emac_init(struct ifnet *);
static void	emac_stop(struct ifnet *, int);
static void	emac_watchdog(struct ifnet *);

static int	emac_add_rxbuf(struct emac_softc *, int);
static void	emac_rxdrain(struct emac_softc *);
static int	emac_set_filter(struct emac_softc *);
static int	emac_txreap(struct emac_softc *);

static void	emac_soft_reset(struct emac_softc *);
static void	emac_smart_reset(struct emac_softc *);

static int	emac_mii_readreg(device_t, int, int);
static void	emac_mii_writereg(device_t, int, int, int);
static void	emac_mii_statchg(struct ifnet *);
static uint32_t	emac_mii_wait(struct emac_softc *);
static void	emac_mii_tick(void *);

int		emac_copy_small = 0;

CFATTACH_DECL_NEW(emac, sizeof(struct emac_softc),
    emac_match, emac_attach, NULL, NULL);


static int
emac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	/* match only on-chip ethernet devices */
	if (strcmp(oaa->opb_name, cf->cf_name) == 0)
		return 1;

	return 0;
}

static void
emac_attach(device_t parent, device_t self, void *aux)
{
	struct opb_attach_args *oaa = aux;
	struct emac_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mii_data *mii = &sc->sc_mii;
	const char * xname = device_xname(self);
	bus_dma_segment_t seg;
	int error, i, nseg, opb_freq, opbc, mii_phy = MII_PHY_ANY;
	const uint8_t *enaddr;
	prop_dictionary_t dict = device_properties(self);
	prop_data_t ea;

	bus_space_map(oaa->opb_bt, oaa->opb_addr, EMAC_NREG, 0, &sc->sc_sh);

	sc->sc_dev = self;
	sc->sc_instance = oaa->opb_instance;
	sc->sc_st = oaa->opb_bt;
	sc->sc_dmat = oaa->opb_dmat;

	callout_init(&sc->sc_callout, 0);

	aprint_naive("\n");
	aprint_normal(": Ethernet Media Access Controller\n");

	/* Fetch the Ethernet address. */
	ea = prop_dictionary_get(dict, "mac-address");
	if (ea == NULL) {
		aprint_error_dev(self, "unable to get mac-address property\n");
		return;
	}
	KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
	KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
	enaddr = prop_data_data_nocopy(ea);
	aprint_normal_dev(self, "Ethernet address %s\n", ether_sprintf(enaddr));

#if defined(EMAC_ZMII_PHY) || defined(EMAC_RGMII_PHY)
	/* Fetch the MII offset. */
	prop_dictionary_get_uint32(dict, "mii-phy", &mii_phy);

#ifdef EMAC_ZMII_PHY
	if (oaa->opb_flags & OPB_FLAGS_EMAC_RMII_ZMII)
		zmii_attach(parent, sc->sc_instance, &sc->sc_rmii_enable,
		    &sc->sc_rmii_disable, &sc->sc_rmii_speed);
#endif
#ifdef EMAC_RGMII_PHY
	if (oaa->opb_flags & OPB_FLAGS_EMAC_RMII_RGMII)
		rgmii_attach(parent, sc->sc_instance, &sc->sc_rmii_enable,
		    &sc->sc_rmii_disable, &sc->sc_rmii_speed);
#endif
#endif

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct emac_control_data), 0, 0, &seg, 1, &nseg, 0)) != 0) {
		aprint_error_dev(self,
		    "unable to allocate control data, error = %d\n", error);
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, nseg,
	    sizeof(struct emac_control_data), (void **)&sc->sc_control_data,
	    BUS_DMA_COHERENT)) != 0) {
		aprint_error_dev(self,
		    "unable to map control data, error = %d\n", error);
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct emac_control_data), 1,
	    sizeof(struct emac_control_data), 0, 0, &sc->sc_cddmamap)) != 0) {
		aprint_error_dev(self,
		    "unable to create control data DMA map, error = %d\n",
		    error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct emac_control_data), NULL,
	    0)) != 0) {
		aprint_error_dev(self,
		    "unable to load control data DMA map, error = %d\n", error);
		goto fail_3;
	}

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < EMAC_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    EMAC_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			aprint_error_dev(self,
			    "unable to create tx DMA map %d, error = %d\n",
			    i, error);
			goto fail_4;
		}
	}

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < EMAC_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			aprint_error_dev(self,
			    "unable to create rx DMA map %d, error = %d\n",
			    i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	/* Soft Reset the EMAC.  The chip to a known state. */
	emac_soft_reset(sc);

	opb_freq = opb_get_frequency();
	switch (opb_freq) {
	case  50000000: opbc =  STACR_OPBC_50MHZ; break;
	case  66666666: opbc =  STACR_OPBC_66MHZ; break;
	case  83333333: opbc =  STACR_OPBC_83MHZ; break;
	case 100000000: opbc = STACR_OPBC_100MHZ; break;

	default:
		if (opb_freq > 100000000) {
			opbc = STACR_OPBC_A100MHZ;
			break;
		}
		aprint_error_dev(self, "unsupport OPB frequency %dMHz\n",
		    opb_freq / 1000 / 1000);
		goto fail_5;
	}
	if (oaa->opb_flags & OPB_FLAGS_EMAC_GBE) {
		sc->sc_mr1 =
		    MR1_RFS_GBE(MR1__FS_16KB)	|
		    MR1_TFS_GBE(MR1__FS_16KB)	|
		    MR1_TR0_MULTIPLE		|
		    MR1_OBCI(opbc);
		sc->sc_ethercom.ec_capabilities |= ETHERCAP_JUMBO_MTU;

		if (oaa->opb_flags & OPB_FLAGS_EMAC_STACV2) {
			sc->sc_stacr_read = STACR_STAOPC_READ;
			sc->sc_stacr_write = STACR_STAOPC_WRITE;
			sc->sc_stacr_bits = STACR_OC;
			sc->sc_stacr_completed = false;
		} else {
			sc->sc_stacr_read = STACR_READ;
			sc->sc_stacr_write = STACR_WRITE;
			sc->sc_stacr_completed = true;
		}
	} else {
		/*
		 * Set up Mode Register 1 - set receive and transmit FIFOs to
		 * maximum size, allow transmit of multiple packets (only
		 * channel 0 is used).
		 *
		 * XXX: Allow pause packets??
		 */
		sc->sc_mr1 =
		    MR1_RFS(MR1__FS_4KB) |
		    MR1_TFS(MR1__FS_2KB) |
		    MR1_TR0_MULTIPLE;

		sc->sc_stacr_read = STACR_READ;
		sc->sc_stacr_write = STACR_WRITE;
		sc->sc_stacr_bits = STACR_OPBC(opbc);
		sc->sc_stacr_completed = true;
	}

	intr_establish(oaa->opb_irq, IST_LEVEL, IPL_NET, emac_intr, sc);
	mal_intr_establish(sc->sc_instance, sc);

	if (oaa->opb_flags & OPB_FLAGS_EMAC_HT256)
		sc->sc_htsize = 256;
	else
		sc->sc_htsize = 64;

	/* Clear all interrupts */
	EMAC_WRITE(sc, EMAC_ISR, ISR_ALL);

	/*
	 * Initialise the media structures.
	 */
	mii->mii_ifp = ifp;
	mii->mii_readreg = emac_mii_readreg;
	mii->mii_writereg = emac_mii_writereg;
	mii->mii_statchg = emac_mii_statchg;

	sc->sc_ethercom.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
	mii_attach(self, mii, 0xffffffff, mii_phy, MII_OFFSET_ANY,
	    MIIF_DOPAUSE);
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);

	ifp = &sc->sc_ethercom.ec_if;
	strcpy(ifp->if_xname, xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = emac_start;
	ifp->if_ioctl = emac_ioctl;
	ifp->if_init = emac_init;
	ifp->if_stop = emac_stop;
	ifp->if_watchdog = emac_watchdog;
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
	evcnt_attach_dynamic(&sc->sc_ev_txintr, EVCNT_TYPE_INTR,
	    NULL, xname, "txintr");
	evcnt_attach_dynamic(&sc->sc_ev_rxintr, EVCNT_TYPE_INTR,
	    NULL, xname, "rxintr");
	evcnt_attach_dynamic(&sc->sc_ev_txde, EVCNT_TYPE_INTR,
	    NULL, xname, "txde");
	evcnt_attach_dynamic(&sc->sc_ev_rxde, EVCNT_TYPE_INTR,
	    NULL, xname, "rxde");
	evcnt_attach_dynamic(&sc->sc_ev_intr, EVCNT_TYPE_INTR,
	    NULL, xname, "intr");

	evcnt_attach_dynamic(&sc->sc_ev_txreap, EVCNT_TYPE_MISC,
	    NULL, xname, "txreap");
	evcnt_attach_dynamic(&sc->sc_ev_txsstall, EVCNT_TYPE_MISC,
	    NULL, xname, "txsstall");
	evcnt_attach_dynamic(&sc->sc_ev_txdstall, EVCNT_TYPE_MISC,
	    NULL, xname, "txdstall");
	evcnt_attach_dynamic(&sc->sc_ev_txdrop, EVCNT_TYPE_MISC,
	    NULL, xname, "txdrop");
	evcnt_attach_dynamic(&sc->sc_ev_tu, EVCNT_TYPE_MISC,
	    NULL, xname, "tu");
#endif /* EMAC_EVENT_COUNTERS */

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	sc->sc_sdhook = shutdownhook_establish(emac_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		aprint_error_dev(self,
		    "WARNING: unable to establish shutdown hook\n");

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

	return 1;
}

static void
emac_shutdown(void *arg)
{
	struct emac_softc *sc = arg;

	emac_stop(&sc->sc_ethercom.ec_if, 0);
}


/*
 * ifnet interface functions
 */

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
				aprint_error_ifnet(ifp,
				    "Tx packet consumes too many "
				    "DMA segments, dropping...\n");
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
			struct mal_descriptor *txdesc =
			    &sc->sc_txdescs[nexttx];

			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the TX_READY bit just
			 * yet.  That could cause a race condition.
			 * We'll do it below.
			 */
			txdesc->md_data = dmamap->dm_segs[seg].ds_addr;
			txdesc->md_data_len = dmamap->dm_segs[seg].ds_len;
			txdesc->md_stat_ctrl =
			    (txdesc->md_stat_ctrl & MAL_TX_WRAP) |
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
		EMAC_WRITE(sc, EMAC_TMR0, TMR0_GNP0 | TMR0_TFAE_2);

		/* Advance the tx pointer. */
		sc->sc_txfree -= txs->txs_ndesc;
		sc->sc_txnext = nexttx;

		sc->sc_txsfree--;
		sc->sc_txsnext = EMAC_NEXTTXS(sc->sc_txsnext);

		/*
		 * Pass the packet to any BPF listeners.
		 */
		bpf_mtap(ifp, m0);
	}

	if (sc->sc_txfree == 0)
		/* No more slots left; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;

	if (sc->sc_txfree != ofree)
		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
}

static int
emac_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct emac_softc *sc = ifp->if_softc;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMTU:
	{
		struct ifreq *ifr = (struct ifreq *)data;
		int maxmtu;

		if (sc->sc_ethercom.ec_capabilities & ETHERCAP_JUMBO_MTU)
			maxmtu = EMAC_MAX_MTU;
		else
			maxmtu = ETHERMTU;

		if (ifr->ifr_mtu < ETHERMIN || ifr->ifr_mtu > maxmtu)
			error = EINVAL;
		else if ((error = ifioctl_common(ifp, cmd, data)) != ENETRESET)
			break;
		else if (ifp->if_flags & IFF_UP)
			error = emac_init(ifp);
		else
			error = 0;
		break;
	}

	default:
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
	}

	/* try to get more packets going */
	emac_start(ifp);

	splx(s);
	return error;
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
	emac_soft_reset(sc);

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
				aprint_error_ifnet(ifp,
				    "unable to allocate or map rx buffer %d,"
				    " error = %d\n",
				    i, error);
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
	 * Load the MAC address.
	 */
	EMAC_WRITE(sc, EMAC_IAHR, enaddr[0] << 8 | enaddr[1]);
	EMAC_WRITE(sc, EMAC_IALR,
	    enaddr[2] << 24 | enaddr[3] << 16 | enaddr[4] << 8 | enaddr[5]);

	/* Enable the transmit and receive channel on the MAL. */
	error = mal_start(sc->sc_instance,
	    EMAC_CDTXADDR(sc, 0), EMAC_CDRXADDR(sc, 0));
	if (error)
		goto out;

	sc->sc_mr1 &= ~MR1_JPSM;
	if (ifp->if_mtu > ETHERMTU)
		/* Enable Jumbo Packet Support Mode */
		sc->sc_mr1 |= MR1_JPSM;

	/* Set fifos, media modes. */
	EMAC_WRITE(sc, EMAC_MR1, sc->sc_mr1);

	/*
	 * Enable Individual and (possibly) Broadcast Address modes,
	 * runt packets, and strip padding.
	 */
	EMAC_WRITE(sc, EMAC_RMR, RMR_IAE | RMR_RRP | RMR_SP | RMR_TFAE_2 |
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
	 * Set interrupt status enable bits for EMAC.
	 */
	EMAC_WRITE(sc, EMAC_ISER,
	    ISR_TXPE |		/* TX Parity Error */
	    ISR_RXPE |		/* RX Parity Error */
	    ISR_TXUE |		/* TX Underrun Event */
	    ISR_RXOE |		/* RX Overrun Event */
	    ISR_OVR  |		/* Overrun Error */
	    ISR_PP   |		/* Pause Packet */
	    ISR_BP   |		/* Bad Packet */
	    ISR_RP   |		/* Runt Packet */
	    ISR_SE   |		/* Short Event */
	    ISR_ALE  |		/* Alignment Error */
	    ISR_BFCS |		/* Bad FCS */
	    ISR_PTLE |		/* Packet Too Long Error */
	    ISR_ORE  |		/* Out of Range Error */
	    ISR_IRE  |		/* In Range Error */
	    ISR_SE0  |		/* Signal Quality Error 0 (SQE) */
	    ISR_TE0  |		/* Transmit Error 0 */
	    ISR_MOS  |		/* MMA Operation Succeeded */
	    ISR_MOF);		/* MMA Operation Failed */

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
		aprint_error_ifnet(ifp, "interface not running\n");
	}
	return error;
}

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
	EMAC_WRITE(sc, EMAC_ISER, 0);

	/* Disable the receive and transmit channels. */
	mal_stop(sc->sc_instance);

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
		aprint_error_ifnet(ifp,
		    "device timeout (txfree %d txsfree %d txnext %d)\n",
		    sc->sc_txfree, sc->sc_txsfree, sc->sc_txnext);
		ifp->if_oerrors++;

		/* Reset the interface. */
		(void)emac_init(ifp);
	} else if (ifp->if_flags & IFF_DEBUG)
		aprint_error_ifnet(ifp, "recovered from device timeout\n");

	/* try to get more packets going */
	emac_start(ifp);
}

static int
emac_add_rxbuf(struct emac_softc *sc, int idx)
{
	struct emac_rxsoft *rxs = &sc->sc_rxsoft[idx];
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return ENOBUFS;
	}

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, rxs->rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "can't load rx DMA map %d, error = %d\n", idx, error);
		panic("emac_add_rxbuf");		/* XXX */
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	EMAC_INIT_RXDESC(sc, idx);

	return 0;
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

static int
emac_set_filter(struct emac_softc *sc)
{
	struct ether_multistep step;
	struct ether_multi *enm;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t rmr, crc, mask, tmp, reg, gaht[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int regs, cnt = 0, i;

	if (sc->sc_htsize == 256) {
		reg = EMAC_GAHT256(0);
		regs = 8;
	} else {
		reg = EMAC_GAHT64(0);
		regs = 4;
	}
	mask = (1ULL << (sc->sc_htsize / regs)) - 1;

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
			gaht[0] = gaht[1] = gaht[2] = gaht[3] =
			    gaht[4] = gaht[5] = gaht[6] = gaht[7] = mask;
			break;
		}

		crc = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);

		if (sc->sc_htsize == 256)
			EMAC_SET_FILTER256(gaht, crc);
		else
			EMAC_SET_FILTER(gaht, crc);

		ETHER_NEXT_MULTI(step, enm);
		cnt++;
	}

	for (i = 1, tmp = gaht[0]; i < regs; i++)
		tmp &= gaht[i];
	if (tmp == mask) {
		/* All categories are true. */
		ifp->if_flags |= IFF_ALLMULTI;
		rmr |= RMR_PMME;
	} else if (cnt != 0) {
		/* Some categories are true. */
		for (i = 0; i < regs; i++)
			EMAC_WRITE(sc, reg + (i << 2), gaht[i]);
		rmr |= RMR_MAE;
	}
	EMAC_WRITE(sc, EMAC_RMR, rmr);

	return 0;
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
	uint32_t txstat;

	EMAC_EVCNT_INCR(&sc->sc_ev_txreap);
	handled = 0;

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

		if (txstat &
		    (EMAC_TXS_EC | EMAC_TXS_MC | EMAC_TXS_SC | EMAC_TXS_LC)) {
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
				aprint_error_ifnet(ifp, "excessive deferral\n");
			if (txstat & EMAC_TXS_EC)
				aprint_error_ifnet(ifp,
				    "excessive collisions\n");
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

	return handled;
}


/*
 * Reset functions
 */

static void
emac_soft_reset(struct emac_softc *sc)
{
	uint32_t sdr;
	int t = 0;

	/*
	 * The PHY must provide a TX Clk in order perform a soft reset the
	 * EMAC.  If none is present, select the internal clock,
	 * SDR0_MFR[E0CS,E1CS].  After the soft reset, select the external
	 * clock.
	 */

	sdr = mfsdr(DCR_SDR0_MFR);
	sdr |= SDR0_MFR_ECS(sc->sc_instance);
	mtsdr(DCR_SDR0_MFR, sdr);

	EMAC_WRITE(sc, EMAC_MR0, MR0_SRST);

	sdr = mfsdr(DCR_SDR0_MFR);
	sdr &= ~SDR0_MFR_ECS(sc->sc_instance);
	mtsdr(DCR_SDR0_MFR, sdr);

	delay(5);

	/* wait finish */
	while (EMAC_READ(sc, EMAC_MR0) & MR0_SRST) {
		if (++t == 1000000 /* 1sec XXXXX */) {
			aprint_error_dev(sc->sc_dev, "Soft Reset failed\n");
			return;
		}
		delay(1);
	}
}

static void
emac_smart_reset(struct emac_softc *sc)
{
	uint32_t mr0;
	int t = 0;

	mr0 = EMAC_READ(sc, EMAC_MR0);
	if (mr0 & (MR0_TXE | MR0_RXE)) {
		mr0 &= ~(MR0_TXE | MR0_RXE);
		EMAC_WRITE(sc, EMAC_MR0, mr0);

		/* wait idel state */
		while ((EMAC_READ(sc, EMAC_MR0) & (MR0_TXI | MR0_RXI)) !=
		    (MR0_TXI | MR0_RXI)) {
			if (++t == 1000000 /* 1sec XXXXX */) {
				aprint_error_dev(sc->sc_dev,
				    "Smart Reset failed\n");
				return;
			}
			delay(1);
		}
	}
}


/*
 * MII related functions
 */

static int
emac_mii_readreg(device_t self, int phy, int reg)
{
	struct emac_softc *sc = device_private(self);
	uint32_t sta_reg;

	if (sc->sc_rmii_enable)
		sc->sc_rmii_enable(device_parent(self), sc->sc_instance);

	/* wait for PHY data transfer to complete */
	if (emac_mii_wait(sc))
		goto fail;

	sta_reg =
	    sc->sc_stacr_read		|
	    (reg << STACR_PRA_SHIFT)	|
	    (phy << STACR_PCDA_SHIFT)	|
	    sc->sc_stacr_bits;
	EMAC_WRITE(sc, EMAC_STACR, sta_reg);

	if (emac_mii_wait(sc))
		goto fail;
	sta_reg = EMAC_READ(sc, EMAC_STACR);

	if (sc->sc_rmii_disable)
		sc->sc_rmii_disable(device_parent(self), sc->sc_instance);

	if (sta_reg & STACR_PHYE)
		return 0;
	return sta_reg >> STACR_PHYD_SHIFT;

fail:
	if (sc->sc_rmii_disable)
		sc->sc_rmii_disable(device_parent(self), sc->sc_instance);
	return 0;
}

static void
emac_mii_writereg(device_t self, int phy, int reg, int val)
{
	struct emac_softc *sc = device_private(self);
	uint32_t sta_reg;

	if (sc->sc_rmii_enable)
		sc->sc_rmii_enable(device_parent(self), sc->sc_instance);

	/* wait for PHY data transfer to complete */
	if (emac_mii_wait(sc))
		goto out;

	sta_reg =
	    (val << STACR_PHYD_SHIFT)	|
	    sc->sc_stacr_write		|
	    (reg << STACR_PRA_SHIFT)	|
	    (phy << STACR_PCDA_SHIFT)	|
	    sc->sc_stacr_bits;
	EMAC_WRITE(sc, EMAC_STACR, sta_reg);

	if (emac_mii_wait(sc))
		goto out;
	if (EMAC_READ(sc, EMAC_STACR) & STACR_PHYE)
		aprint_error_dev(sc->sc_dev, "MII PHY Error\n");

out:
	if (sc->sc_rmii_disable)
		sc->sc_rmii_disable(device_parent(self), sc->sc_instance);
}

static void
emac_mii_statchg(struct ifnet *ifp)
{
	struct emac_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;

	/*
	 * MR1 can only be written immediately after a reset...
	 */
	emac_smart_reset(sc);

	sc->sc_mr1 &= ~(MR1_FDE | MR1_ILE | MR1_EIFC | MR1_MF_MASK | MR1_IST);
	if (mii->mii_media_active & IFM_FDX)
		sc->sc_mr1 |= (MR1_FDE | MR1_EIFC | MR1_IST);
	if (mii->mii_media_active & IFM_FLOW)
		sc->sc_mr1 |= MR1_EIFC;
	if (mii->mii_media_active & IFM_LOOP)
		sc->sc_mr1 |= MR1_ILE;
	switch (IFM_SUBTYPE(mii->mii_media_active)) {
	case IFM_1000_T:
		sc->sc_mr1 |= (MR1_MF_1000MBS | MR1_IST);
		break;

	case IFM_100_TX:
		sc->sc_mr1 |= (MR1_MF_100MBS | MR1_IST);
		break;

	case IFM_10_T:
		sc->sc_mr1 |= MR1_MF_10MBS;
		break;

	case IFM_NONE:
		break;

	default:
		aprint_error_dev(sc->sc_dev, "unknown sub-type %d\n",
		    IFM_SUBTYPE(mii->mii_media_active));
		break;
	}
	if (sc->sc_rmii_speed)
		sc->sc_rmii_speed(device_parent(sc->sc_dev), sc->sc_instance,
		    IFM_SUBTYPE(mii->mii_media_active));

	EMAC_WRITE(sc, EMAC_MR1, sc->sc_mr1);

	/* Enable TX and RX if already RUNNING */
	if (ifp->if_flags & IFF_RUNNING)
		EMAC_WRITE(sc, EMAC_MR0, MR0_TXE | MR0_RXE);
}

static uint32_t
emac_mii_wait(struct emac_softc *sc)
{
	int i;
	uint32_t oc;

	/* wait for PHY data transfer to complete */
	i = 0;
	oc = EMAC_READ(sc, EMAC_STACR) & STACR_OC;
	while ((oc == STACR_OC) != sc->sc_stacr_completed) {
		delay(7);
		if (i++ > 5) {
			aprint_error_dev(sc->sc_dev, "MII timed out\n");
			return -1;
		}
		oc = EMAC_READ(sc, EMAC_STACR) & STACR_OC;
	}
	return 0;
}

static void
emac_mii_tick(void *arg)
{
	struct emac_softc *sc = arg;
	int s;

	if (!device_is_active(sc->sc_dev))
		return;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_callout, hz, emac_mii_tick, sc);
}

int
emac_txeob_intr(void *arg)
{
	struct emac_softc *sc = arg;
	int handled = 0;

	EMAC_EVCNT_INCR(&sc->sc_ev_txintr);
	handled |= emac_txreap(sc);

	/* try to get more packets going */
	emac_start(&sc->sc_ethercom.ec_if);

	return handled;
}

int
emac_rxeob_intr(void *arg)
{
	struct emac_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct emac_rxsoft *rxs;
	struct mbuf *m;
	uint32_t rxstat;
	int i, len;

	EMAC_EVCNT_INCR(&sc->sc_ev_rxintr);

	for (i = sc->sc_rxptr; ; i = EMAC_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		EMAC_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		rxstat = sc->sc_rxdescs[i].md_stat_ctrl;

		if (rxstat & MAL_RX_EMPTY) {
			/*
			 * We have processed all of the receive buffers.
			 */
			/* Flush current empty descriptor */
			EMAC_CDRXSYNC(sc, i,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
			break;
		}

		/*
		 * If an error occurred, update stats, clear the status
		 * word, and leave the packet buffer in place.  It will
		 * simply be reused the next time the ring comes around.
		 */
		if (rxstat & (EMAC_RXS_OE | EMAC_RXS_BP | EMAC_RXS_SE |
		    EMAC_RXS_AE | EMAC_RXS_BFCS | EMAC_RXS_PTL | EMAC_RXS_ORE |
		    EMAC_RXS_IRE)) {
#define	PRINTERR(bit, str)					\
			if (rxstat & (bit))			\
				aprint_error_ifnet(ifp,		\
				    "receive error: %s\n", str)
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

		/*
		 * Pass this up to any BPF listeners, but only
		 * pass if up the stack if it's for us.
		 */
		bpf_mtap(ifp, m);

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* Update the receive pointer. */
	sc->sc_rxptr = i;

	return 1;
}

int
emac_txde_intr(void *arg)
{
	struct emac_softc *sc = arg;

	EMAC_EVCNT_INCR(&sc->sc_ev_txde);
	aprint_error_dev(sc->sc_dev, "emac_txde_intr\n");
	return 1;
}

int
emac_rxde_intr(void *arg)
{
	struct emac_softc *sc = arg;
	int i;

	EMAC_EVCNT_INCR(&sc->sc_ev_rxde);
	aprint_error_dev(sc->sc_dev, "emac_rxde_intr\n");
	/*
	 * XXX!
	 * This is a bit drastic; we just drop all descriptors that aren't
	 * "clean".  We should probably send any that are up the stack.
	 */
	for (i = 0; i < EMAC_NRXDESC; i++) {
		EMAC_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		if (sc->sc_rxdescs[i].md_data_len != MCLBYTES)
			EMAC_INIT_RXDESC(sc, i);
	}

	return 1;
}
