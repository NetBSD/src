/* $NetBSD: gemini_gmacvar.h,v 1.2 2008/12/15 04:44:27 matt Exp $ */
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
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

#ifndef _ARM_GEMINI_GEMINI_GMACVAR_H
#define _ARM_GEMINI_GEMINI_GMACVAR_H

#include <sys/device.h>
#include <sys/queue.h>
#include <net/if.h>
#include <net/if_media.h>
#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <arm/gemini/gemini_gmacreg.h>

typedef struct gmac_hwqueue gmac_hwqueue_t;
typedef struct gmac_hwqmem gmac_hwqmem_t;
typedef struct gmac_mapcache gmac_mapcache_t;

#define	MAX_TXMAPS	256
#define	MIN_TXMAPS	16
#define	MAX_RXMAPS	256
#define	MIN_RXMAPS	32

#define	RXQ_NDESCS	128
#define	TXQ_NDESCS	128

struct gmac_mapcache {
	bus_dma_tag_t mc_dmat;
	bus_size_t mc_mapsize;
	size_t mc_nsegs;
	size_t mc_free;
	size_t mc_used;
	size_t mc_max;
	bus_dmamap_t mc_maps[0];
};

struct gmac_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;

	/*
	 * MDIO state
	 */
	kmutex_t sc_mdiolock;
	uint32_t sc_mdiobits;
	device_t sc_gpio_dev;
	uint8_t sc_gpio_mdclk;
	uint8_t sc_gpio_mdin;
	uint8_t sc_gpio_mdout;

	/*
	 * What GMAC ports have attached?
	 */
	uint8_t sc_ports;
	uint8_t sc_running;

	/*
	 * The hardware free queue and software free queue are shared
	 * resources.  As are the dmamap caches.
	 */
	gmac_hwqueue_t *sc_swfreeq;
	gmac_hwqueue_t *sc_hwfreeq;
	gmac_mapcache_t *sc_rxmaps;
	gmac_mapcache_t *sc_txmaps;
	size_t sc_swfree_min;		/* min mbufs to keep on swfreeq */
	size_t sc_rxpkts_per_sec; 

	/*
	 * What interrupts are enabled for both ports?
	 */
	uint32_t sc_int_enabled[5];
	uint32_t sc_int_select[5];
};

struct gmac_attach_args {
	bus_space_tag_t gma_iot;
	bus_space_handle_t gma_ioh;
	bus_dma_tag_t gma_dmat;

	int gma_port;
	int gma_phy;
	int gma_intr;

	mii_readreg_t gma_mii_readreg;
	mii_writereg_t gma_mii_writereg;
};

struct gmac_hwqmem {
	bus_dma_tag_t hqm_dmat;
	bus_dmamap_t hqm_dmamap;
	gmac_mapcache_t *hqm_mc;
	gmac_desc_t *hqm_base;
	bus_size_t hqm_memsize;
	bus_dma_segment_t hqm_segs[1];
	size_t hqm_ndesc;
	size_t hqm_nqueue;
	int hqm_nsegs;
	uint8_t hqm_refs;
	uint8_t hqm_flags;
#define	HQM_TX			0x0001
#define	HQM_RX			0x0000
#define	HQM_PRODUCER		0x0002
#define	HQM_CONSUMER		0x0000
};

struct gmac_hwqueue {
	gmac_desc_t *hwq_base;
	gmac_hwqmem_t *hwq_hqm;
	union {
		SLIST_ENTRY(gmac_hwqueue) qun_link;
		SLIST_HEAD(,gmac_hwqueue) qun_producers;
		struct gmac_hwqueue *qun_producer;
	} hwq_qun;
#define	hwq_link	hwq_qun.qun_link
#define	hwq_producers	hwq_qun.qun_producers
#define	hwq_producer	hwq_qun.qun_producer
	struct ifnet *hwq_ifp;
	struct ifqueue hwq_ifq;
	struct mbuf *hwq_rxmbuf;
	struct mbuf **hwq_mp;
	bus_space_tag_t hwq_iot;
	bus_space_handle_t hwq_qrwptr_ioh;
	/*
	 * These are in units of gmac_desc_t, not bytes.
	 */
	size_t hwq_qoff;	/* offset in descriptors to start */
	uint16_t hwq_wptr;	/* next descriptor to write */
	uint16_t hwq_rptr;	/* next descriptor to read */
	uint16_t hwq_free;	/* descriptors can be produced */
	uint16_t hwq_size;	/* total number of descriptors */
	uint8_t hwq_ref;
};

#ifdef _KERNEL
gmac_hwqmem_t *
	gmac_hwqmem_create(gmac_mapcache_t *, size_t, size_t, int);
void	gmac_hwqmem_destroy(gmac_hwqmem_t *);

void	gmac_hwqueue_destroy(gmac_hwqueue_t *);
gmac_hwqueue_t *
	gmac_hwqueue_create(gmac_hwqmem_t *, bus_space_tag_t,
	    bus_space_handle_t, bus_size_t, bus_size_t, size_t);

gmac_desc_t *
	gmac_hwqueue_desc(gmac_hwqueue_t *, size_t);
void	gmac_hwqueue_sync(gmac_hwqueue_t *);
void	gmac_hwqueue_consume(gmac_hwqueue_t *);
void	gmac_hwqueue_produce(gmac_hwqueue_t *, size_t);

gmac_mapcache_t *
	gmac_mapcache_create(bus_dma_tag_t, size_t, bus_size_t, int);
void	gmac_mapcache_destroy(gmac_mapcache_t **);
int	gmac_mapcache_fill(gmac_mapcache_t *, size_t);
bus_dmamap_t
	gmac_mapcache_get(gmac_mapcache_t *);
void	gmac_mapcache_put(gmac_mapcache_t *, bus_dmamap_t);

void	gmac_intr_update(struct gmac_softc *);
#endif

#endif /* _ARM_GEMINI_GEMINI_GMACVAR_H */
