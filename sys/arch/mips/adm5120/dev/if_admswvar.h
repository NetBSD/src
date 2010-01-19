/* $NetBSD: if_admswvar.h,v 1.2 2010/01/19 22:06:21 pooka Exp $ */

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
#ifndef	_IF_ADMSWVAR_H_
#define	_IF_ADMSWVAR_H_

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_admswvar.h,v 1.2 2010/01/19 22:06:21 pooka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/wdog.h>

#include <uvm/uvm_extern.h>		/* for PAGE_SIZE */

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/endian.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/sysmon/sysmonvar.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_obiovar.h>

#include <mips/adm5120/dev/if_admswreg.h>

#define	ADMSW_EVENT_COUNTERS

#define	MAC_BUFLEN	0x07ff

#define	ADMSW_NTXHDESC	4
#define	ADMSW_NRXHDESC	32
#define	ADMSW_NTXLDESC	32
#define	ADMSW_NRXLDESC	32

#define	ADMSW_NTXHDESC_MASK	(ADMSW_NTXHDESC - 1)
#define	ADMSW_NRXHDESC_MASK	(ADMSW_NRXHDESC - 1)
#define	ADMSW_NTXLDESC_MASK	(ADMSW_NTXLDESC - 1)
#define	ADMSW_NRXLDESC_MASK	(ADMSW_NRXLDESC - 1)

#define	ADMSW_NEXTTXH(x)	(((x) + 1) & ADMSW_NTXHDESC_MASK)
#define	ADMSW_NEXTRXH(x)	(((x) + 1) & ADMSW_NRXHDESC_MASK)
#define	ADMSW_NEXTTXL(x)	(((x) + 1) & ADMSW_NTXLDESC_MASK)
#define	ADMSW_NEXTRXL(x)	(((x) + 1) & ADMSW_NRXLDESC_MASK)

struct admsw_control_data {
	/* The transmit descriptors. */
	struct admsw_desc acd_txhdescs[ADMSW_NTXHDESC];

	/* The receive descriptors. */
	struct admsw_desc acd_rxhdescs[ADMSW_NRXHDESC];

	/* The transmit descriptors. */
	struct admsw_desc acd_txldescs[ADMSW_NTXLDESC];

	/* The receive descriptors. */
	struct admsw_desc acd_rxldescs[ADMSW_NRXLDESC];
};

#define	ADMSW_CDOFF(x)		offsetof(struct admsw_control_data, x)
#define	ADMSW_CDTXHOFF(x)	ADMSW_CDOFF(acd_txhdescs[(x)])
#define	ADMSW_CDTXLOFF(x)	ADMSW_CDOFF(acd_txldescs[(x)])
#define	ADMSW_CDRXHOFF(x)	ADMSW_CDOFF(acd_rxhdescs[(x)])
#define	ADMSW_CDRXLOFF(x)	ADMSW_CDOFF(acd_rxldescs[(x)])

struct admsw_descsoft {
	struct mbuf *ds_mbuf;
	bus_dmamap_t ds_dmamap;
};

/*
 * Software state per device.
 */
struct admsw_softc {
	struct device sc_dev;		/* generic device information */
	uint8_t		sc_enaddr[ETHER_ADDR_LEN];
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_ioh;	/* MAC space handle */
	struct sysmon_wdog	sc_smw;
	struct ifmedia sc_ifmedia[SW_DEVS];
	int ndevs;			/* number of IFF_RUNNING interfaces */
	struct ethercom sc_ethercom[SW_DEVS];	/* Ethernet common data */
	void *sc_sdhook;		/* shutdown hook */
	void *sc_ih;			/* interrupt cookie */
	struct admsw_descsoft sc_txhsoft[ADMSW_NTXHDESC];
	struct admsw_descsoft sc_rxhsoft[ADMSW_NRXHDESC];
	struct admsw_descsoft sc_txlsoft[ADMSW_NTXLDESC];
	struct admsw_descsoft sc_rxlsoft[ADMSW_NRXLDESC];
	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define	sc_cddma	sc_cddmamap->dm_segs[0].ds_addr
	struct admsw_control_data *sc_control_data;
#define	sc_txhdescs	sc_control_data->acd_txhdescs
#define	sc_rxhdescs	sc_control_data->acd_rxhdescs
#define	sc_txldescs	sc_control_data->acd_txldescs
#define	sc_rxldescs	sc_control_data->acd_rxldescs

	int sc_txfree;			/* number of free Tx descriptors */
	int sc_txnext;			/* next Tx descriptor to use */
	int sc_txdirty;			/* first dirty Tx descriptor */

	int sc_rxptr;			/* next ready Rx descriptor */

#ifdef ADMSW_EVENT_COUNTERS
	struct evcnt sc_ev_txstall;	/* Tx stalled */
	struct evcnt sc_ev_rxstall;	/* Rx stalled */
	struct evcnt sc_ev_txintr;	/* Tx interrupts */
	struct evcnt sc_ev_rxintr;	/* Rx interrupts */
#if 1
	struct evcnt sc_ev_rxsync;	/* Rx syncs */
#endif
#endif

};

#define	ADMSW_CDTXHADDR(sc, x)	((sc)->sc_cddma + ADMSW_CDTXHOFF((x)))
#define	ADMSW_CDTXLADDR(sc, x)	((sc)->sc_cddma + ADMSW_CDTXLOFF((x)))
#define	ADMSW_CDRXHADDR(sc, x)	((sc)->sc_cddma + ADMSW_CDRXHOFF((x)))
#define	ADMSW_CDRXLADDR(sc, x)	((sc)->sc_cddma + ADMSW_CDRXLOFF((x)))

#define	ADMSW_CDTXHSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    ADMSW_CDTXHOFF((x)), sizeof(struct admsw_desc), (ops))

#define	ADMSW_CDTXLSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    ADMSW_CDTXLOFF((x)), sizeof(struct admsw_desc), (ops))

#define	ADMSW_CDRXHSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    ADMSW_CDRXHOFF((x)), sizeof(struct admsw_desc), (ops))

#define	ADMSW_CDRXLSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    ADMSW_CDRXLOFF((x)), sizeof(struct admsw_desc), (ops))

#define	ADMSW_INIT_RXHDESC(sc, x)				\
do {								\
	struct admsw_descsoft *__ds = &(sc)->sc_rxhsoft[(x)];	\
	struct admsw_desc *__desc = &(sc)->sc_rxhdescs[(x)];	\
	struct mbuf *__m = __ds->ds_mbuf;			\
								\
	__m->m_data = __m->m_ext.ext_buf + 2;			\
	__desc->data = __ds->ds_dmamap->dm_segs[0].ds_addr + 2;	\
	__desc->cntl = 0;					\
	__desc->len = min(MCLBYTES - 2, MAC_BUFLEN - 2);	\
	__desc->status = 0;					\
	if ((x) == ADMSW_NRXHDESC - 1)				\
		__desc->data |= ADM5120_DMA_RINGEND;		\
	__desc->data |= ADM5120_DMA_OWN;			\
	ADMSW_CDRXHSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
} while (0)

#define	ADMSW_INIT_RXLDESC(sc, x)				\
do {								\
	struct admsw_descsoft *__ds = &(sc)->sc_rxlsoft[(x)];	\
	struct admsw_desc *__desc = &(sc)->sc_rxldescs[(x)];	\
	struct mbuf *__m = __ds->ds_mbuf;			\
								\
	__m->m_data = __m->m_ext.ext_buf + 2;			\
	__desc->data = __ds->ds_dmamap->dm_segs[0].ds_addr + 2;	\
	__desc->cntl = 0;					\
	__desc->len = min(MCLBYTES - 2, MAC_BUFLEN - 2);	\
	__desc->status = 0;					\
	if ((x) == ADMSW_NRXLDESC - 1)				\
		__desc->data |= ADM5120_DMA_RINGEND;		\
	__desc->data |= ADM5120_DMA_OWN;			\
	ADMSW_CDRXLSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
} while (0)

void admwdog_attach(struct admsw_softc *);

#endif /* _IF_ADMSWVAR_H_ */
