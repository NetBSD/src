/*	$NetBSD: tulipvar.h,v 1.22 1999/12/11 00:33:02 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DEV_IC_TULIPVAR_H_
#define	_DEV_IC_TULIPVAR_H_

#include <sys/queue.h>

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
#define	TULIP_NTXSEGS		16

#define	TULIP_TXQUEUELEN	64
#define	TULIP_NTXDESC		(TULIP_TXQUEUELEN * TULIP_NTXSEGS)
#define	TULIP_NTXDESC_MASK	(TULIP_NTXDESC - 1)
#define	TULIP_NEXTTX(x)		((x + 1) & TULIP_NTXDESC_MASK)

/*
 * Receive descriptor list size.  We have one Rx buffer per incoming
 * packet, so this logic is a little simpler.
 */
#define	TULIP_NRXDESC		64
#define	TULIP_NRXDESC_MASK	(TULIP_NRXDESC - 1)
#define	TULIP_NEXTRX(x)		((x + 1) & TULIP_NRXDESC_MASK)

/*
 * Control structures are DMA'd to the TULIP chip.  We allocate them in
 * a single clump that maps to a single DMA segment to make several things
 * easier.
 */
struct tulip_control_data {
	/*
	 * The transmit descriptors.
	 */
	struct tulip_desc tcd_txdescs[TULIP_NTXDESC];

	/*
	 * The receive descriptors.
	 */
	struct tulip_desc tcd_rxdescs[TULIP_NRXDESC];

	/*
	 * The setup descriptor.
	 */
	struct tulip_desc tcd_setup_desc;

	/*
	 * The setup packet.
	 */
	u_int32_t tcd_setup_packet[TULIP_SETUP_PACKET_LEN / sizeof(u_int32_t)];
};

#define	TULIP_CDOFF(x)		offsetof(struct tulip_control_data, x)
#define	TULIP_CDTXOFF(x)	TULIP_CDOFF(tcd_txdescs[(x)])
#define	TULIP_CDRXOFF(x)	TULIP_CDOFF(tcd_rxdescs[(x)])
#define	TULIP_CDSDOFF		TULIP_CDOFF(tcd_setup_desc)
#define	TULIP_CDSPOFF		TULIP_CDOFF(tcd_setup_packet)

/*
 * Software state for transmit jobs.
 */
struct tulip_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	SIMPLEQ_ENTRY(tulip_txsoft) txs_q;
};

SIMPLEQ_HEAD(tulip_txsq, tulip_txsoft);

/*
 * Software state for receive jobs.
 */
struct tulip_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

/*
 * Type of Tulip chip we're dealing with.
 */
typedef enum {
	TULIP_CHIP_INVALID   = 0,	/* invalid chip type */
	TULIP_CHIP_DE425     = 1,	/* DE-425 EISA */
	TULIP_CHIP_21040     = 2,	/* DECchip 21040 */
	TULIP_CHIP_21041     = 3,	/* DECchip 21041 */
	TULIP_CHIP_21140     = 4,	/* DECchip 21140 */
	TULIP_CHIP_21140A    = 5,	/* DECchip 21140A */
	TULIP_CHIP_21142     = 6,	/* DECchip 21142 */
	TULIP_CHIP_21143     = 7,	/* DECchip 21143 */
	TULIP_CHIP_82C168    = 8,	/* Lite-On 82C168 PNIC */
	TULIP_CHIP_82C169    = 9,	/* Lite-On 82C169 PNIC */
	TULIP_CHIP_82C115    = 10,	/* Lite-On 82C115 PNIC II */
	TULIP_CHIP_MX98713   = 11,	/* Macronix 98713 PMAC */
	TULIP_CHIP_MX98713A  = 12,	/* Macronix 98713A PMAC */
	TULIP_CHIP_MX98715   = 13,	/* Macronix 98715 PMAC */
	TULIP_CHIP_MX98715A  = 14,	/* Macronix 98715A PMAC */
	TULIP_CHIP_MX98725   = 15,	/* Macronix 98725 PMAC */
	TULIP_CHIP_WB89C840F = 16,	/* Winbond 89C840F */
	TULIP_CHIP_DM9102    = 17,	/* Davicom DM9102 */
	TULIP_CHIP_AL981     = 18,	/* ADMtek AL981 */
	TULIP_CHIP_AX88140   = 19,	/* ASIX AX88140 */
	TULIP_CHIP_AX88141   = 20,	/* ASIX AX88141 */
} tulip_chip_t;

#define	TULIP_CHIP_NAMES						\
{									\
	NULL,								\
	"DE-425",							\
	"DECchip 21040",						\
	"DECchip 21041",						\
	"DECchip 21140",						\
	"DECchip 21140A",						\
	"DECchip 21142",						\
	"DECchip 21143",						\
	"Lite-On 82C168",						\
	"Lite-On 82C169",						\
	"Lite-On 82C115",						\
	"Macronix MX98713",						\
	"Macronix MX98713A",						\
	"Macronix MX98715",						\
	"Macronix MX98715A",						\
	"Macronix MX98725",						\
	"Winbond 89C840F",						\
	"Davicom DM9102",						\
	"ADMtek AL981",							\
	"ASIX AX88140",							\
	"ASIX AX88141",							\
}

struct tulip_softc;

/*
 * Media init, change, status function pointers.
 */
struct tulip_mediasw {
	void	(*tmsw_init) __P((struct tulip_softc *));
	void	(*tmsw_get) __P((struct tulip_softc *, struct ifmediareq *));
	int	(*tmsw_set) __P((struct tulip_softc *));
};

/*
 * Table which describes the transmit threshold mode.
 */
struct tulip_txthresh_tab {
	u_int32_t txth_opmode;		/* OPMODE bits */
	const char *txth_name;		/* name of mode */
};

/*
 * Settings for Tulip SIA media.
 */
struct tulip_sia_media {
	u_int32_t	tsm_siaconn;	/* CSR13 value */
	u_int32_t	tsm_siatxrx;	/* CSR14 value */
	u_int32_t	tsm_siagen;	/* CSR15 value */
};

/*
 * Description of 2x14x media.
 */
struct tulip_21x4x_media {
	int		tm_type;	/* type of media; see tulipreg.h */
	const char	*tm_name;	/* name of media */

	void		(*tm_get) __P((struct tulip_softc *,
			    struct ifmediareq *));
	int		(*tm_set) __P((struct tulip_softc *));

	int		tm_phyno;	/* PHY # on MII */

	int		tm_gp_length;	/* MII select sequence length */
	int		tm_gp_offset;	/* MII select sequence offset */

	int		tm_reset_length;/* MII reset sequence length */
	int		tm_reset_offset;/* MII reset sequence offset */

	u_int32_t	tm_opmode;	/* OPMODE bits for this media */
	u_int32_t	tm_gpctl;	/* GPIO control bits for this media */
	u_int32_t	tm_gpdata;	/* GPIO bits for this media */
	u_int32_t	tm_actmask;	/* `active' bits for this data */
	u_int32_t	tm_actdata;	/* active high/low info */

	struct tulip_sia_media tm_sia;	/* SIA settings */
#define	tm_siaconn	tm_sia.tsm_siaconn
#define	tm_siatxrx	tm_sia.tsm_siatxrx
#define	tm_siagen	tm_sia.tsm_siagen
};

/*
 * Table for converting Tulip SROM media info into ifmedia data.
 */
struct tulip_srom_to_ifmedia {
	u_int8_t	tsti_srom;	/* SROM media type */
	int		tsti_subtype;	/* ifmedia subtype */
	int		tsti_options;	/* ifmedia options */
	const char	*tsti_name;	/* media name */

	u_int32_t	tsti_opmode;	/* OPMODE bits for this media */

	/*
	 * Settings for 21040, 21041, and 21142/21143 SIA, in the event
	 * the SROM doesn't have them.
	 */
	struct tulip_sia_media tsti_21040;
	struct tulip_sia_media tsti_21041;
	struct tulip_sia_media tsti_21142;
};

/*
 * Some misc. statics, useful for debugging.
 */
struct tulip_stats {
	u_long		ts_tx_uf;	/* transmit underflow errors */
	u_long		ts_tx_to;	/* transmit jabber timeouts */
	u_long		ts_tx_ec;	/* excessve collision count */
	u_long		ts_tx_lc;	/* late collision count */
};

/*
 * Software state per device.
 */
struct tulip_softc {
	struct device sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* ethernet common data */
	void *sc_sdhook;		/* shutdown hook */

	struct tulip_stats sc_stats;	/* debugging stats */

	/*
	 * Contents of the SROM.
	 */
	u_int8_t sc_srom[TULIP_MAX_ROM_SIZE];
	int sc_srom_addrbits;

	/*
	 * Media access functions for this chip.
	 */
	const struct tulip_mediasw *sc_mediasw;
	mii_bitbang_ops_t sc_bitbang_ops;

	/*
	 * For chips with built-in NWay blocks, these are state
	 * variables required for autonegotiation.
	 */
	int		sc_nway_ticks;	/* tick counter */
	struct ifmedia_entry *sc_nway_active; /* the active media */

	tulip_chip_t	sc_chip;	/* chip type */
	int		sc_rev;		/* chip revision */
	int		sc_flags;	/* misc flags. */
	char		sc_name[16];	/* board name */
	u_int32_t	sc_cacheline;	/* cache line size */
	int		sc_devno;	/* PCI device # */

	struct mii_data sc_mii;		/* MII/media information */

	const struct tulip_txthresh_tab *sc_txth;
	int		sc_txthresh;	/* current transmit threshold */

	u_int8_t	sc_gp_dir;	/* GPIO pin direction bits (21140) */
	int		sc_media_seen;	/* ISV media block types seen */
	int		sc_tlp_minst;	/* Tulip internal media instance */

	/* Reset function. */
	void		(*sc_reset) __P((struct tulip_softc *));

	/* Pre-init function. */
	void		(*sc_preinit) __P((struct tulip_softc *));

	/* Filter setup function. */
	void		(*sc_filter_setup) __P((struct tulip_softc *));

	/* Media status update function. */
	void		(*sc_statchg) __P((struct device *));

	/* Media tick function. */
	void		(*sc_tick) __P((void *));

	/*
	 * The Winbond 89C840F places registers 4 bytes apart, instead
	 * of 8.
	 */
	int		sc_regshift;

	u_int32_t	sc_busmode;	/* copy of CSR_BUSMODE */
	u_int32_t	sc_opmode;	/* copy of CSR_OPMODE */
	u_int32_t	sc_inten;	/* copy of CSR_INTEN */

	u_int32_t	sc_rxint_mask;	/* mask of Rx interrupts we want */
	u_int32_t	sc_txint_mask;	/* mask of Tx interrupts we want */

	u_int32_t	sc_filtmode;	/* filter mode we're using */

	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define	sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	/*
	 * Software state for transmit and receive descriptors.
	 */
	struct tulip_txsoft sc_txsoft[TULIP_TXQUEUELEN];
	struct tulip_rxsoft sc_rxsoft[TULIP_NRXDESC];

	/*
	 * Control data structures.
	 */
	struct tulip_control_data *sc_control_data;
#define	sc_txdescs	sc_control_data->tcd_txdescs
#define	sc_rxdescs	sc_control_data->tcd_rxdescs
#define	sc_setup_desc	sc_control_data->tcd_setup_desc

	int	sc_txfree;		/* number of free Tx descriptors */
	int	sc_txnext;		/* next ready Tx descriptor */

	struct tulip_txsq sc_txfreeq;	/* free Tx descsofts */
	struct tulip_txsq sc_txdirtyq;	/* dirty Tx descsofts */

	int	sc_rxptr;		/* next ready RX descriptor/descsoft */
};

/* sc_flags */
#define	TULIPF_WANT_SETUP	0x00000001	/* want filter setup */
#define	TULIPF_DOING_SETUP	0x00000002	/* doing multicast setup */
#define	TULIPF_HAS_MII		0x00000004	/* has media on MII */
#define	TULIPF_IC_FS		0x00000008	/* IC bit on first tx seg */
#define	TULIPF_MRL		0x00000010	/* memory read line okay */
#define	TULIPF_MRM		0x00000020	/* memory read multi okay */
#define	TULIPF_MWI		0x00000040	/* memory write inval okay */
#define	TULIPF_LINK_UP		0x00000100	/* link is up (non-MII) */
#define	TULIPF_LINK_VALID	0x00000200	/* link state valid */
#define	TULIPF_DOINGAUTO	0x00000400	/* doing autoneg (non-MII) */

/*
 * This macro returns the current media entry for *non-MII* media.
 */
#define	TULIP_CURRENT_MEDIA(sc)						\
	(IFM_SUBTYPE((sc)->sc_mii.mii_media.ifm_cur->ifm_media) != IFM_AUTO ? \
	 (sc)->sc_mii.mii_media.ifm_cur : (sc)->sc_nway_active)

/*
 * This macro determines if a change to media-related OPMODE bits requires
 * a chip reset.
 */
#define	TULIP_MEDIA_NEEDSRESET(sc, newbits)				\
	(((sc)->sc_opmode & OPMODE_MEDIA_BITS) !=			\
	 ((newbits) & OPMODE_MEDIA_BITS))

#define	TULIP_CDTXADDR(sc, x)	((sc)->sc_cddma + TULIP_CDTXOFF((x)))
#define	TULIP_CDRXADDR(sc, x)	((sc)->sc_cddma + TULIP_CDRXOFF((x)))

#define	TULIP_CDSDADDR(sc)	((sc)->sc_cddma + TULIP_CDSDOFF)
#define	TULIP_CDSPADDR(sc)	((sc)->sc_cddma + TULIP_CDSPOFF)

#define	TULIP_CDSP(sc)		((sc)->sc_control_data->tcd_setup_packet)

#define	TULIP_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > TULIP_NTXDESC) {				\
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,	\
		    TULIP_CDTXOFF(__x), sizeof(struct tulip_desc) *	\
		    (TULIP_NTXDESC - __x), (ops));			\
		__n -= (TULIP_NTXDESC - __x);				\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    TULIP_CDTXOFF(__x), sizeof(struct tulip_desc) * __n, (ops)); \
} while (0)

#define	TULIP_CDRXSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    TULIP_CDRXOFF((x)), sizeof(struct tulip_desc), (ops))

#define	TULIP_CDSDSYNC(sc, ops)						\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    TULIP_CDSDOFF, sizeof(struct tulip_desc), (ops))

#define	TULIP_CDSPSYNC(sc, ops)						\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    TULIP_CDSPOFF, TULIP_SETUP_PACKET_LEN, (ops))

/*
 * Note we rely on MCLBYTES being a power of two.  Because the `length'
 * field is only 11 bits, we must subtract 1 from the length to avoid
 * having it truncated to 0!
 */
#define	TULIP_INIT_RXDESC(sc, x)					\
do {									\
	struct tulip_rxsoft *__rxs = &sc->sc_rxsoft[(x)];		\
	struct tulip_desc *__rxd = &sc->sc_rxdescs[(x)];		\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
									\
	__m->m_data = __m->m_ext.ext_buf;				\
	__rxd->td_bufaddr1 = __rxs->rxs_dmamap->dm_segs[0].ds_addr;	\
	__rxd->td_bufaddr2 = TULIP_CDRXADDR((sc), TULIP_NEXTRX((x)));	\
	__rxd->td_ctl =							\
	    ((__m->m_ext.ext_size - 1) << TDCTL_SIZE1_SHIFT) | TDCTL_CH; \
	__rxd->td_status = TDSTAT_OWN|TDSTAT_Rx_FS|TDSTAT_Rx_LS;	\
	TULIP_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
} while (0)

/* CSR access */
#define	TULIP_CSR_OFFSET(sc, csr)					\
	(TULIP_CSR_INDEX(csr) << (sc)->sc_regshift)

#define	TULIP_READ(sc, reg)						\
	bus_space_read_4((sc)->sc_st, (sc)->sc_sh,			\
	    TULIP_CSR_OFFSET((sc), (reg)))

#define	TULIP_WRITE(sc, reg, val)					\
	bus_space_write_4((sc)->sc_st, (sc)->sc_sh,			\
	    TULIP_CSR_OFFSET((sc), (reg)), (val))

#define	TULIP_SET(sc, reg, mask)					\
	TULIP_WRITE((sc), (reg), TULIP_READ((sc), (reg)) | (mask))

#define	TULIP_CLR(sc, reg, mask)					\
	TULIP_WRITE((sc), (reg), TULIP_READ((sc), (reg)) & ~(mask))

#define	TULIP_ISSET(sc, reg, mask)					\
	(TULIP_READ((sc), (reg)) & (mask))

#if BYTE_ORDER == BIG_ENDIAN
#define	TULIP_SP_FIELD_C(x)	((x) << 16)
#else
#define	TULIP_SP_FIELD_C(x)	(x)
#endif
#define	TULIP_SP_FIELD(x, f)	TULIP_SP_FIELD_C(((u_int16_t *)(x))[(f)])

#ifdef _KERNEL
extern const struct tulip_mediasw tlp_21040_mediasw;
extern const struct tulip_mediasw tlp_21040_tp_mediasw;
extern const struct tulip_mediasw tlp_21040_auibnc_mediasw;
extern const struct tulip_mediasw tlp_21041_mediasw;
extern const struct tulip_mediasw tlp_2114x_isv_mediasw;
extern const struct tulip_mediasw tlp_sio_mii_mediasw;
extern const struct tulip_mediasw tlp_pnic_mediasw;
extern const struct tulip_mediasw tlp_pmac_mediasw;
extern const struct tulip_mediasw tlp_al981_mediasw;

void	tlp_attach __P((struct tulip_softc *, const u_int8_t *));
int	tlp_intr __P((void *));
void	tlp_read_srom __P((struct tulip_softc *, int, int, u_int8_t *));
int	tlp_srom_crcok __P((const u_int8_t *));
int	tlp_isv_srom __P((const u_int8_t *));
int	tlp_isv_srom_enaddr __P((struct tulip_softc *, u_int8_t *));
int	tlp_parse_old_srom __P((struct tulip_softc *, u_int8_t *));

int	tlp_mediachange __P((struct ifnet *));
void	tlp_mediastatus __P((struct ifnet *, struct ifmediareq *));
#endif /* _KERNEL */

#endif /* _DEV_IC_TULIPVAR_H_ */
