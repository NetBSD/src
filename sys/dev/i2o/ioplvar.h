/*	$NetBSD: ioplvar.h,v 1.2.4.3 2002/06/23 17:46:06 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _I2O_IOPLVAR_H_
#define	_I2O_IOPLVAR_H_

#define	IOPL_TICK_HZ		3
#define	IOPL_DESCRIPTORS	128
#define	IOPL_MAX_SEGS		16
#define	IOPL_MAX_BATCH		4
#define	IOPL_MAX_MULTI		256
#define	IOPL_BATCHING_ENABLED	1

struct iopl_media {
	u_int			ilm_i2o;
	u_int			ilm_ifmedia;
};

struct iopl_tx {
	SLIST_ENTRY(iopl_tx)	tx_chain;
	struct mbuf		*tx_mbuf;
	bus_dmamap_t		tx_dmamap;
	int			tx_ident;
};

struct iopl_rx {
	SLIST_ENTRY(iopl_rx)	rx_chain;
	struct mbuf		*rx_mbuf;
	bus_dmamap_t		rx_dmamap;
	int			rx_ident;
};

struct iopl_softc {
	struct device		sc_dv;		/* Generic device info */
	int			sc_tid;		/* Device TID */
	int			sc_flags;	/* Control flags */
	bus_dma_tag_t		sc_dmat;	/* DMA tag */

	struct iop_initiator	sc_ii_evt;	/* Event initiator */
	struct iop_initiator	sc_ii_pg;	/* PG retrieval initiator */
	struct iop_initiator	sc_ii_rx;	/* Receive initiator */
	struct iop_initiator	sc_ii_tx;	/* Transmit initiator */
	struct iop_initiator	sc_ii_null;	/* Bit-bucket */

	struct callout		sc_pg_callout;	/* PG retrieval callout */
	int			sc_ms_pg;	/* Medium-specific PG #  */
	int			sc_next_pg;	/* Next PG to retrieve */
	int			sc_mtype;	/* Medium type */
	int			sc_curmbps;	/* Current link speed */
	int			sc_conntype;	/* Connection type */

	struct iopl_tx		*sc_tx;		/* Transmit descriptors */
	SLIST_HEAD(,iopl_tx)	sc_tx_free;	/* Free TX descriptor list */
	u_int			sc_tx_freecnt;	/* # of free TX descriptors */
	u_int			sc_tx_maxout;	/* Max outstanding xmits */
	u_int			sc_tx_maxsegs;	/* Max segments per xmit */
	u_int			sc_tx_maxreq;	/* Max xmits per message */
	u_int			sc_tx_ohead;	/* S/G list overhead */
	u_int			sc_tx_tcw;	/* Transmit control word */

	struct iopl_rx		*sc_rx;		/* Receive descriptors */
	SLIST_HEAD(,iopl_rx)	sc_rx_free;	/* Free RX descriptor list */
	u_int			sc_rx_freecnt;	/* # of free RX descriptors */
	u_int			sc_rx_maxbkt;	/* Max receive buckets */
	u_int			sc_rx_prepad;	/* Pre-padding for alignment */
	u_int			sc_rx_csumflgs;	/* Checksum flags */

	u_int			sc_mcast_max;	/* Maximum multicast addrs */
	u_int			sc_mcast_cnt;	/* Active multicast addrs */

	/* Pull dst address from a packet and insert into buffer context. */
	void			(*sc_munge)(struct mbuf *, u_int8_t *);

	/*
	 * Interface info
	 */
	union {
		struct	ifnet sci_if;
		struct	ethercom sci_ec;
	} sc_if;

	struct ifmedia		sc_ifmedia;	/* ifmedia linkage */

	/* 
	 * Parameter group buffers.  We retrieve only one parameter group at
	 * a time.
	 */
	struct {
		struct	i2o_param_op_results pr;
		struct	i2o_param_read_results prr;
		union {
			struct	i2o_param_lan_stats ls;
			struct	i2o_param_lan_802_3_stats les;
			struct	i2o_param_lan_fddi_stats lfs;
			struct	i2o_param_lan_media_operation lmo;
		} p;
	} __attribute__ ((__packed__)) sc_pb;
};
#define	IOPL_MEDIA_CHANGE	0x01
#define	IOPL_LINK		0x02
#define	IOPL_FDX		0x04
#define	IOPL_INITTED		0x08

#endif	/* !_I2O_IOPLVAR_H_ */
