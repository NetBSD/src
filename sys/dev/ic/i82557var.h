/*	$NetBSD: i82557var.h,v 1.9 1999/12/12 17:46:36 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999 The NetBSD Foundation, Inc.
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

/*                  
 * Copyright (c) 1995, David Greenman
 * All rights reserved.
 *              
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:             
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.  
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: if_fxpvar.h,v 1.4 1997/11/29 08:11:01 davidg Exp
 */

/*
 * Misc. defintions for the Intel i82557 fast Ethernet controller
 * driver.
 */

/*
 * Transmit descriptor list size.
 */
#define	FXP_NTXCB		128
#define	FXP_NTXCB_MASK		(FXP_NTXCB - 1)
#define	FXP_NEXTTX(x)		((x + 1) & FXP_NTXCB_MASK)
#define	FXP_NTXSEG		16

/*
 * Number of receive frame area buffers.  These are large, so
 * choose wisely.
 */
#define	FXP_NRFABUFS		64

/*
 * Maximum number of seconds that the reciever can be idle before we
 * assume it's dead and attempt to reset it by reprogramming the
 * multicast filter.  This is part of a work-around for a bug in the
 * NIC.  See fxp_stats_update().
 */
#define	FXP_MAX_RX_IDLE	15

/*
 * Misc. DMA'd data structures are allocated in a single clump, that
 * maps to a single DMA segment, to make several things easier (computing
 * offsets, setting up DMA maps, etc.)
 */
struct fxp_control_data {
	/*
	 * The transmit control blocks.  The first if these
	 * is also used as the config CB.
	 */
	struct fxp_cb_tx fcd_txcbs[FXP_NTXCB];

	/*
	 * The transmit buffer descriptors.
	 */
	struct fxp_tbdlist {
		struct fxp_tbd tbd_d[FXP_NTXSEG];
	} fcd_tbdl[FXP_NTXCB];

	/*
	 * The configuration CB.
	 */
	struct fxp_cb_config fcd_configcb;

	/*
	 * The Individual Address CB.
	 */
	struct fxp_cb_ias fcd_iascb;

	/*
	 * The multicast setup CB.
	 */
	struct fxp_cb_mcs fcd_mcscb;

	/*
	 * The NIC statistics.
	 */
	struct fxp_stats fcd_stats;
};

#define	FXP_CDOFF(x)	offsetof(struct fxp_control_data, x)
#define	FXP_CDTXOFF(x)	FXP_CDOFF(fcd_txcbs[(x)])
#define	FXP_CDTBDOFF(x)	FXP_CDOFF(fcd_tbdl[(x)])
#define	FXP_CDCONFIGOFF	FXP_CDOFF(fcd_configcb)
#define	FXP_CDIASOFF	FXP_CDOFF(fcd_iascb)
#define	FXP_CDMCSOFF	FXP_CDOFF(fcd_mcscb)
#define	FXP_CDSTATSOFF	FXP_CDOFF(fcd_stats)

/*
 * Software state for transmit descriptors.
 */
struct fxp_txsoft {
	struct mbuf *txs_mbuf;		/* head of mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
};

/*
 * Software state per device.
 */
struct fxp_softc {
	struct device sc_dev;		/* generic device structures */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_dma_tag_t sc_dmat;		/* bus dma tag */
	struct ethercom sc_ethercom;	/* ethernet common part */
	void *sc_sdhook;		/* shutdown hook */
	void *sc_powerhook;		/* power hook */
	void *sc_ih;			/* interrupt handler cookie */

	struct mii_data sc_mii;		/* MII/media information */

	/*
	 * We create a single DMA map that maps all data structure
	 * overhead, except for RFAs, which are mapped by the
	 * fxp_rxdesc DMA map on a per-mbuf basis.
	 */
	bus_dmamap_t sc_dmamap;
#define	sc_cddma	sc_dmamap->dm_segs[0].ds_addr

	/*
	 * Software state for transmit descriptors.
	 */
	struct fxp_txsoft sc_txsoft[FXP_NTXCB];

	struct ifqueue sc_rxq;		/* receive buffer queue */
	bus_dmamap_t sc_rxmaps[FXP_NRFABUFS]; /* free receive buffer DMA maps */
	int	sc_rxfree;		/* free map index */
	int	sc_rxidle;		/* # of seconds RX has been idle */

	/*
	 * Control data structures.
	 */
	struct fxp_control_data *sc_control_data;

	int	sc_flags;		/* misc. flags */

#define	FXPF_WANTINIT		0x01	/* want a re-init */
#define	FXPF_MII		0x02	/* device uses MII */

	int	sc_txpending;		/* number of TX requests pending */
	int	sc_txdirty;		/* first dirty TX descriptor */
	int	sc_txlast;		/* last used TX descriptor */

	int phy_primary_addr;		/* address of primary PHY */
	int phy_primary_device;		/* device type of primary PHY */
	int phy_10Mbps_only;		/* PHY is 10Mbps-only device */

	int	sc_enabled;	/* boolean; power enabled on interface */
	int	(*sc_enable) __P((struct fxp_softc *));
	void	(*sc_disable) __P((struct fxp_softc *));

	int sc_eeprom_size;		/* log2 size of EEPROM */
#if NRND > 0
	rndsource_element_t rnd_source;	/* random source */
#endif
	
};

#define	FXP_RXMAP_GET(sc)	((sc)->sc_rxmaps[(sc)->sc_rxfree++])
#define	FXP_RXMAP_PUT(sc, map)	(sc)->sc_rxmaps[--(sc)->sc_rxfree] = (map)

#define	FXP_CDTXADDR(sc, x)	((sc)->sc_cddma + FXP_CDTXOFF((x)))
#define	FXP_CDTBDADDR(sc, x)	((sc)->sc_cddma + FXP_CDTBDOFF((x)))

#define	FXP_CDTX(sc, x)		(&(sc)->sc_control_data->fcd_txcbs[(x)])
#define	FXP_CDTBD(sc, x)	(&(sc)->sc_control_data->fcd_tbdl[(x)])

#define	FXP_DSTX(sc, x)		(&(sc)->sc_txsoft[(x)])

#define	FXP_CDTXSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_dmamap,			\
	    FXP_CDTXOFF((x)), sizeof(struct fxp_cb_tx), (ops))

#define	FXP_CDTBDSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_dmamap,			\
	    FXP_CDTBDOFF((x)), sizeof(struct fxp_tbdlist), (ops))

#define	FXP_CDCONFIGSYNC(sc, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_dmamap,			\
	    FXP_CDCONFIGOFF, sizeof(struct fxp_cb_config), (ops))

#define	FXP_CDIASSYNC(sc, ops)						\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_dmamap,			\
	    FXP_CDIASOFF, sizeof(struct fxp_cb_ias), (ops))

#define	FXP_CDMCSSYNC(sc, ops)						\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_dmamap,			\
	    FXP_CDMCSOFF, sizeof(struct fxp_cb_mcs), (ops))

#define	FXP_RXBUFSIZE(m)	((m)->m_ext.ext_size -			\
				 (sizeof(struct fxp_rfa) +		\
				  RFA_ALIGNMENT_FUDGE))

#define	FXP_RFASYNC(sc, m, ops)						\
	bus_dmamap_sync((sc)->sc_dmat, M_GETCTX((m), bus_dmamap_t),	\
	    RFA_ALIGNMENT_FUDGE, sizeof(struct fxp_rfa), (ops))

#define	FXP_RXBUFSYNC(sc, m, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, M_GETCTX((m), bus_dmamap_t),	\
	    RFA_ALIGNMENT_FUDGE + sizeof(struct fxp_rfa),		\
	    FXP_RXBUFSIZE((m)), (ops))

#define	FXP_MTORFA(m)	(struct fxp_rfa *)((m)->m_ext.ext_buf +		\
					   RFA_ALIGNMENT_FUDGE)

#define	FXP_INIT_RFABUF(sc, m)						\
do {									\
	bus_dmamap_t __rxmap = M_GETCTX((m), bus_dmamap_t);		\
	struct mbuf *__p_m;						\
	struct fxp_rfa *__rfa, *__p_rfa;				\
	u_int32_t __v;							\
									\
	(m)->m_data = (m)->m_ext.ext_buf + sizeof(struct fxp_rfa) +	\
	    RFA_ALIGNMENT_FUDGE;					\
									\
	__rfa = FXP_MTORFA((m));					\
	__rfa->size = htole16(FXP_RXBUFSIZE((m)));			\
	/* BIG_ENDIAN: no need to swap to store 0 */			\
	__rfa->rfa_status = 0;						\
	__rfa->rfa_control = htole16(FXP_RFA_CONTROL_EL);		\
	/* BIG_ENDIAN: no need to swap to store 0 */			\
	__rfa->actual_size = 0;						\
									\
	/* NOTE: the RFA is misaligned, so we must copy. */		\
	/* BIG_ENDIAN: no need to swap to store 0xffffffff */		\
	__v = 0xffffffff;						\
	memcpy((void *)&__rfa->link_addr, &__v, sizeof(__v));		\
	memcpy((void *)&__rfa->rbd_addr, &__v, sizeof(__v));		\
									\
	FXP_RFASYNC((sc), (m),						\
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);			\
									\
	FXP_RXBUFSYNC((sc), (m), BUS_DMASYNC_PREREAD);			\
									\
	if ((__p_m = (sc)->sc_rxq.ifq_tail) != NULL) {			\
		__p_rfa = FXP_MTORFA(__p_m);				\
		__v = htole32(__rxmap->dm_segs[0].ds_addr +		\
		    RFA_ALIGNMENT_FUDGE);				\
		FXP_RFASYNC((sc), __p_m,				\
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);	\
		memcpy((void *)&__p_rfa->link_addr, &__v,		\
		    sizeof(__v));					\
		__p_rfa->rfa_control &= htole16(~FXP_RFA_CONTROL_EL);	\
		FXP_RFASYNC((sc), __p_m,				\
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);		\
	}								\
	IF_ENQUEUE(&(sc)->sc_rxq, (m));					\
} while (0)

/* Macros to ease CSR access. */
#define	CSR_READ_1(sc, reg)						\
	bus_space_read_1((sc)->sc_st, (sc)->sc_sh, (reg))
#define	CSR_READ_2(sc, reg)						\
	bus_space_read_2((sc)->sc_st, (sc)->sc_sh, (reg))
#define	CSR_READ_4(sc, reg)						\
	bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (reg))
#define	CSR_WRITE_1(sc, reg, val)					\
	bus_space_write_1((sc)->sc_st, (sc)->sc_sh, (reg), (val))
#define	CSR_WRITE_2(sc, reg, val)					\
	bus_space_write_2((sc)->sc_st, (sc)->sc_sh, (reg), (val))
#define	CSR_WRITE_4(sc, reg, val)					\
	bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (reg), (val))

void	fxp_attach __P((struct fxp_softc *));
int	fxp_intr __P((void *));

int	fxp_enable __P((struct fxp_softc*));
void	fxp_disable __P((struct fxp_softc*));

