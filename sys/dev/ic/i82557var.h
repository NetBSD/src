/*	$NetBSD: i82557var.h,v 1.1 1999/06/20 16:33:29 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
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
 * Number of transmit control blocks.  This determines the number
 * of transmit buffers that can be chained in the CB list.  This
 * must be a power of two.
 */
#define	FXP_NTXCB	128

/*
 * TxCB list index mask.  This is used to do list wrap-around.
 */
#define	FXP_TXCB_MASK	(FXP_NTXCB - 1)

/*
 * Number of receive frame area buffers.  These are large, so
 * choose wisely.
 */
#define	FXP_NRFABUFS	64

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
	 * The multicast setup CB.
	 */
	struct fxp_cb_mcs fcd_mcscb;

	/*
	 * The NIC statistics.
	 */
	struct fxp_stats fcd_stats;
};

#define	FXP_CDOFF(x)	offsetof(struct fxp_control_data, x)

/*
 * Receive buffer descriptor (software only).  This is the analog of
 * the software portion of the fxp_cb_tx.
 */
struct fxp_rxdesc {
	struct fxp_rxdesc *fr_next;	/* next in the chain */
	struct mbuf *fr_mbhead;		/* pointer to mbuf chain */
	bus_dmamap_t fr_dmamap;		/* our DMA map */
};

struct fxp_softc {
	struct device sc_dev;		/* generic device structures */
	void *sc_ih;			/* interrupt handler cookie */
	void *sc_sdhook;		/* shutdown hook */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_dma_tag_t sc_dmat;		/* bus dma tag */
	struct ethercom sc_ethercom;	/* ethernet common part */
#define	sc_if		sc_ethercom.ec_if

	/*
	 * We create a single DMA map that maps all data structure
	 * overhead, except for RFAs, which are mapped by the
	 * fxp_rxdesc DMA map on a per-mbuf basis.
	 */
	bus_dmamap_t sc_dmamap;
#define	sc_cddma	sc_dmamap->dm_segs[0].ds_addr

	/*
	 * These DMA maps map transmit and recieve buffers.
	 */
	bus_dmamap_t sc_tx_dmamaps[FXP_NTXCB];
	bus_dmamap_t sc_rx_dmamaps[FXP_NRFABUFS];

	/*
	 * Control data - TxCBs, stats, etc.
	 */
	struct fxp_control_data *control_data;

					/* receive buffer descriptors */
	struct fxp_rxdesc sc_rxdescs[FXP_NRFABUFS];
	struct mii_data sc_mii;		/* MII media information */
	struct fxp_cb_tx *cbl_first;	/* first active TxCB in list */
	struct fxp_cb_tx *cbl_last;	/* last active TxCB in list */
	int tx_queued;			/* # of active TxCB's */
	int need_mcsetup;		/* multicast filter needs programming */
	struct fxp_rxdesc *rfa_head;	/* first mbuf in receive frame area */
	struct fxp_rxdesc *rfa_tail;	/* last mbuf in receive frame area */
	int rx_idle_secs;		/* # of seconds RX has been idle */
	int all_mcasts;			/* receive all multicasts */
	int promisc_mode;		/* promiscuous mode enabled */
	int phy_primary_addr;		/* address of primary PHY */
	int phy_primary_device;		/* device type of primary PHY */
	int phy_10Mbps_only;		/* PHY is 10Mbps-only device */
#if NRND > 0
	rndsource_element_t rnd_source;	/* random source */
#endif
};

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
