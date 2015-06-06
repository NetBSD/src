/*	$NetBSD: sqvar.h,v 1.14.24.1 2015/06/06 14:40:03 skrll Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _ARCH_SGIMIPS_HPC_SQVAR_H_
#define	_ARCH_SGIMIPS_HPC_SQVAR_H_

#include <sys/queue.h>
#include <sys/callout.h>

#include <sys/rndsource.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>

/* Note, these must be powers of two for the magic NEXT/PREV macros to work */
#define SQ_NRXDESC		64
#define SQ_NTXDESC		64

#define	SQ_NRXDESC_MASK		(SQ_NRXDESC - 1)
#define	SQ_NEXTRX(x)		((x + 1) & SQ_NRXDESC_MASK)
#define	SQ_PREVRX(x)		((x - 1) & SQ_NRXDESC_MASK)

#define	SQ_NTXDESC_MASK		(SQ_NTXDESC - 1)
#define	SQ_NEXTTX(x)		((x + 1) & SQ_NTXDESC_MASK)
#define	SQ_PREVTX(x)		((x - 1) & SQ_NTXDESC_MASK)

/*
 * We pack all DMA control structures into one container so we can alloc just
 * one chunk of DMA-safe memory and pack them into it.  Otherwise, we'd have to
 * allocate a page for each descriptor, since the bus_dmamem_alloc() interface
 * does not allow us to allocate smaller chunks.
 */
struct sq_control {
	/* Receive descriptors */
	struct hpc_dma_desc	rx_desc[SQ_NRXDESC];

	/* Transmit descriptors */
	struct hpc_dma_desc	tx_desc[SQ_NTXDESC];
};

#define	SQ_CDOFF(x)		offsetof(struct sq_control, x)
#define	SQ_CDTXOFF(x)		SQ_CDOFF(tx_desc[(x)])
#define	SQ_CDRXOFF(x)		SQ_CDOFF(rx_desc[(x)])

#define	SQ_TYPE_8003		0
#define	SQ_TYPE_80C03		1

/* Trace Actions */
#define SQ_RESET		1
#define SQ_ADD_TO_DMA		2
#define SQ_START_DMA		3
#define SQ_DONE_DMA		4
#define SQ_RESTART_DMA		5
#define SQ_TXINTR_ENTER		6
#define SQ_TXINTR_EXIT		7
#define SQ_TXINTR_BUSY		8
#define SQ_IOCTL		9
#define SQ_ENQUEUE		10

struct sq_action_trace {
	int action;
	int line;
	int bufno;
	int status;
	int freebuf;
};

#define SQ_TRACEBUF_SIZE	100

#define SQ_TRACE(act, sc, buf, stat) do {				\
	(sc)->sq_trace[(sc)->sq_trace_idx].action = (act);		\
	(sc)->sq_trace[(sc)->sq_trace_idx].line = __LINE__;		\
	(sc)->sq_trace[(sc)->sq_trace_idx].bufno = (buf);		\
	(sc)->sq_trace[(sc)->sq_trace_idx].status = (stat);		\
	(sc)->sq_trace[(sc)->sq_trace_idx].freebuf = (sc)->sc_nfreetx;	\
	if (++(sc)->sq_trace_idx == SQ_TRACEBUF_SIZE)			\
		(sc)->sq_trace_idx = 0;					\
} while (/* CONSTCOND */0)

struct sq_softc {
	device_t		sc_dev;

	/* HPC registers */
	bus_space_tag_t		sc_hpct;
	bus_space_handle_t	sc_hpch;


	/* HPC external ethernet registers: aka Seeq 8003 registers */
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	bus_dma_tag_t		sc_dmat;

	struct ethercom		sc_ethercom;
	uint8_t			sc_enaddr[ETHER_ADDR_LEN];

	int			sc_type;

	struct sq_control*	sc_control;
#define	sc_rxdesc		sc_control->rx_desc
#define	sc_txdesc		sc_control->tx_desc

	/* DMA structures for control data (DMA RX/TX descriptors) */
	int			sc_ncdseg;
	bus_dma_segment_t	sc_cdseg;
	bus_dmamap_t		sc_cdmap;
#define	sc_cddma		sc_cdmap->dm_segs[0].ds_addr

	int			sc_nextrx;

	/* DMA structures for RX packet data */
	bus_dma_segment_t	sc_rxseg[SQ_NRXDESC];
	bus_dmamap_t		sc_rxmap[SQ_NRXDESC];
	struct mbuf*		sc_rxmbuf[SQ_NRXDESC];

	int			sc_nexttx;
	int			sc_prevtx;
	int			sc_nfreetx;

	/* DMA structures for TX packet data */
	bus_dma_segment_t	sc_txseg[SQ_NTXDESC];
	bus_dmamap_t		sc_txmap[SQ_NTXDESC];
	struct mbuf*		sc_txmbuf[SQ_NTXDESC];

	uint8_t			sc_rxcmd;	/* prototype rxcmd */

	struct evcnt		sq_intrcnt;	/* count interrupts */

	krndsource_t	rnd_source;	/* random source */
	struct hpc_values       *hpc_regs;      /* HPC register definitions */

	int			sq_trace_idx;
	struct sq_action_trace	sq_trace[SQ_TRACEBUF_SIZE];
};

#define	SQ_CDTXADDR(sc, x)	((sc)->sc_cddma + SQ_CDTXOFF((x)))
#define	SQ_CDRXADDR(sc, x)	((sc)->sc_cddma + SQ_CDRXOFF((x)))

static inline void
SQ_CDTXSYNC(struct sq_softc *sc, int __x, int __n, int ops)
{
	/* If it will wrap around, sync to the end of the ring. */
	if ((__x + __n) > SQ_NTXDESC) {
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cdmap,
		    SQ_CDTXOFF(__x), sizeof(struct hpc_dma_desc) *
		    (SQ_NTXDESC - __x), (ops));
		__n -= (SQ_NTXDESC - __x);
		__x = 0;
	}

	/* Now sync whatever is left. */
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cdmap,
	    SQ_CDTXOFF(__x), sizeof(struct hpc_dma_desc) * __n, (ops));
}

#define	SQ_CDRXSYNC(sc, x, ops)						\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cdmap,			\
	    SQ_CDRXOFF((x)), sizeof(struct hpc_dma_desc), (ops))

static inline void
SQ_INIT_RXDESC(struct sq_softc *sc, unsigned int x)
{
	struct hpc_dma_desc* __rxd = &(sc)->sc_rxdesc[(x)];
	struct mbuf *__m = (sc)->sc_rxmbuf[(x)];

	__m->m_data = __m->m_ext.ext_buf;
	if (sc->hpc_regs->revision == 3) {
		__rxd->hpc3_hdd_bufptr =
		    (sc)->sc_rxmap[(x)]->dm_segs[0].ds_addr;
		__rxd->hpc3_hdd_ctl = __m->m_ext.ext_size | HPC3_HDD_CTL_OWN |
		    HPC3_HDD_CTL_INTR | HPC3_HDD_CTL_EOPACKET |
		    ((x) == (SQ_NRXDESC  - 1) ? HPC3_HDD_CTL_EOCHAIN : 0);
	} else {
		__rxd->hpc1_hdd_bufptr =
		    (sc)->sc_rxmap[(x)]->dm_segs[0].ds_addr |
		    ((x) == (SQ_NRXDESC - 1) ? HPC1_HDD_CTL_EOCHAIN : 0);
		__rxd->hpc1_hdd_ctl = __m->m_ext.ext_size | HPC1_HDD_CTL_OWN |
		    HPC1_HDD_CTL_INTR | HPC1_HDD_CTL_EOPACKET;
	}
	__rxd->hdd_descptr = SQ_CDRXADDR((sc), SQ_NEXTRX((x)));
	SQ_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

#endif	/* _ARCH_SGIMIPS_HPC_SQVAR_H_ */
