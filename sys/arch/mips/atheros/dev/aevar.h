/*	$NetBSD: aevar.h,v 1.5.6.2 2017/12/03 11:36:26 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#ifndef _MIPS_ATHEROS_DEV_AEVAR_H_
#define	_MIPS_ATHEROS_DEV_AEVAR_H_

#include <sys/queue.h>
#include <sys/callout.h>

#include <sys/rndsource.h>

/*
 * Misc. definitions for the Digital Semiconductor ``Tulip'' (21x4x)
 * Ethernet controller family driver.
 */

/*
 * Transmit descriptor list size.  This is arbitrary, but allocate
 * enough descriptors for 64 pending transmissions and 16 segments
 * per packet.  Since a descriptor holds 2 buffer addresses, that's
 * 8 descriptors per packet.  This MUST work out to a power of 2.
 */
#define	AE_NTXSEGS		16

#define	AE_TXQUEUELEN		64
#define	AE_NTXDESC		(AE_TXQUEUELEN * AE_NTXSEGS)
#define	AE_NTXDESC_MASK		(AE_NTXDESC - 1)
#define	AE_NEXTTX(x)		((x + 1) & AE_NTXDESC_MASK)

/*
 * Receive descriptor list size.  We have one Rx buffer per incoming
 * packet, so this logic is a little simpler.
 */
#define	AE_NRXDESC		64
#define	AE_NRXDESC_MASK		(AE_NRXDESC - 1)
#define	AE_NEXTRX(x)		((x + 1) & AE_NRXDESC_MASK)

/*
 * Control structures are DMA'd to the TULIP chip.  We allocate them in
 * a single clump that maps to a single DMA segment to make several things
 * easier.
 */
struct ae_control_data {
	/*
	 * The transmit descriptors.
	 */
	struct ae_desc acd_txdescs[AE_NTXDESC];

	/*
	 * The receive descriptors.
	 */
	struct ae_desc acd_rxdescs[AE_NRXDESC];
};

#define	AE_CDOFF(x)		offsetof(struct ae_control_data, x)
#define	AE_CDTXOFF(x)		AE_CDOFF(acd_txdescs[(x)])
#define	AE_CDRXOFF(x)		AE_CDOFF(acd_rxdescs[(x)])

/*
 * Software state for transmit jobs.
 */
struct ae_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	int txs_ndescs;			/* number of descriptors */
	SIMPLEQ_ENTRY(ae_txsoft) txs_q;
};

SIMPLEQ_HEAD(ae_txsq, ae_txsoft);

/*
 * Software state for receive jobs.
 */
struct ae_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

struct ae_softc;

/*
 * Some misc. statics, useful for debugging.
 */
struct ae_stats {
	u_long		ts_tx_uf;	/* transmit underflow errors */
	u_long		ts_tx_to;	/* transmit jabber timeouts */
	u_long		ts_tx_ec;	/* excessive collision count */
	u_long		ts_tx_lc;	/* late collision count */
};

#ifndef _STANDALONE
/*
 * Software state per device.
 */
struct ae_softc {
	device_t sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_size_t sc_size;		/* bus space size */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	void *sc_ih;			/* interrupt handle */	
	int sc_cirq;			/* interrupt request line (cpu) */
	int sc_mirq;			/* interrupt request line (misc) */
	struct ethercom sc_ethercom;	/* ethernet common data */
	void *sc_sdhook;		/* shutdown hook */
	void *sc_powerhook;		/* power management hook */

	struct ae_stats sc_stats;	/* debugging stats */

	int		sc_flags;	/* misc flags. */

	struct mii_data sc_mii;		/* MII/media information */

	int		sc_txthresh;	/* current transmit threshold */

	/* Media tick function. */
	void		(*sc_tick)(void *);
	struct callout sc_tick_callout;

	u_int32_t	sc_inten;	/* copy of CSR_INTEN */

	u_int32_t	sc_rxint_mask;	/* mask of Rx interrupts we want */
	u_int32_t	sc_txint_mask;	/* mask of Tx interrupts we want */

	bus_dma_segment_t sc_cdseg;	/* control data memory */
	int		sc_cdnseg;	/* number of segments */
	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define	sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	/*
	 * Software state for transmit and receive descriptors.
	 */
	struct ae_txsoft sc_txsoft[AE_TXQUEUELEN];
	struct ae_rxsoft sc_rxsoft[AE_NRXDESC];

	/*
	 * Control data structures.
	 */
	struct ae_control_data *sc_control_data;
#define	sc_txdescs	sc_control_data->acd_txdescs
#define	sc_rxdescs	sc_control_data->acd_rxdescs
#define	sc_setup_desc	sc_control_data->acd_setup_desc

	int	sc_txfree;		/* number of free Tx descriptors */
	int	sc_txnext;		/* next ready Tx descriptor */

	struct ae_txsq sc_txfreeq;	/* free Tx descsofts */
	struct ae_txsq sc_txdirtyq;	/* dirty Tx descsofts */

	short	sc_if_flags;

	int	sc_rxptr;		/* next ready RX descriptor/descsoft */

	krndsource_t sc_rnd_source; /* random source */
};
#endif

/* sc_flags */
#define	AE_ATTACHED		0x00000800	/* attach has succeeded */
#define	AE_ENABLED		0x00001000	/* chip is enabled */

#define	AE_IS_ENABLED(sc)	((sc)->sc_flags & AE_ENABLED)

/*
 * This macro returns the current media entry.
 */
#define	AE_CURRENT_MEDIA(sc) ((sc)->sc_mii.mii_media.ifm_cur)

/*
 * This macro determines if a change to media-related OPMODE bits requires
 * a chip reset.
 */
#define	TULIP_MEDIA_NEEDSRESET(sc, newbits)				\
	(((sc)->sc_opmode & OPMODE_MEDIA_BITS) !=			\
	 ((newbits) & OPMODE_MEDIA_BITS))

#define	AE_CDTXADDR(sc, x)	((sc)->sc_cddma + AE_CDTXOFF((x)))
#define	AE_CDRXADDR(sc, x)	((sc)->sc_cddma + AE_CDRXOFF((x)))

#define	AE_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > AE_NTXDESC) {					\
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,	\
		    AE_CDTXOFF(__x), sizeof(struct ae_desc) *		\
		    (AE_NTXDESC - __x), (ops));				\
		__n -= (AE_NTXDESC - __x);				\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    AE_CDTXOFF(__x), sizeof(struct ae_desc) * __n, (ops));	\
} while (0)

#define	AE_CDRXSYNC(sc, x, ops)						\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    AE_CDRXOFF((x)), sizeof(struct ae_desc), (ops))

/*
 * Note we rely on MCLBYTES being a power of two.  Because the `length'
 * field is only 11 bits, we must subtract 1 from the length to avoid
 * having it truncated to 0!
 */
#define	AE_INIT_RXDESC(sc, x)						\
do {									\
	struct ae_rxsoft *__rxs = &sc->sc_rxsoft[(x)];			\
	struct ae_desc *__rxd = &sc->sc_rxdescs[(x)];			\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
									\
	__m->m_data = __m->m_ext.ext_buf;				\
	__rxd->ad_bufaddr1 =						\
	    (__rxs->rxs_dmamap->dm_segs[0].ds_addr);			\
	__rxd->ad_bufaddr2 =						\
	    AE_CDRXADDR((sc), AE_NEXTRX((x)));				\
	__rxd->ad_ctl =							\
	    ((((__m->m_ext.ext_size - 1) & ~0x3U)			\
	    << ADCTL_SIZE1_SHIFT) | 					\
	    ((x) == (AE_NRXDESC - 1) ? ADCTL_ER : 0));			\
	__rxd->ad_status = ADSTAT_OWN|ADSTAT_Rx_FS|ADSTAT_Rx_LS;	\
	AE_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
} while (0)

/* CSR access */

#define	AE_READ(sc, reg)						\
	bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (reg))

#define	AE_WRITE(sc, reg, val)						\
	bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (reg), (val))

#define	AE_SET(sc, reg, mask)						\
	AE_WRITE((sc), (reg), AE_READ((sc), (reg)) | (mask))

#define	AE_CLR(sc, reg, mask)						\
	AE_WRITE((sc), (reg), AE_READ((sc), (reg)) & ~(mask))

#define	AE_ISSET(sc, reg, mask)						\
	(AE_READ((sc), (reg)) & (mask))

#define	AE_BARRIER(sc)							\
	bus_space_barrier((sc)->sc_st, (sc)->sc_sh, 0, (sc)->sc_size,	\
	    BUS_SPACE_BARRIER_WRITE)

#endif /* _MIPS_ATHEROS_DEV_AEVAR_H_ */
