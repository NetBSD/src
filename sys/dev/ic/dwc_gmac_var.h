/* $NetBSD: dwc_gmac_var.h,v 1.16 2019/09/13 07:55:06 msaitoh Exp $ */

/*-
 * Copyright (c) 2013, 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry and Martin Husemann.
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

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

/* Use DWCGMAC_MPSAFE inside the front-ends for interrupt handlers.  */
#ifdef NET_MPSAFE
#define DWCGMAC_MPSAFE	1
#endif

#ifdef DWCGMAC_MPSAFE
#define DWCGMAC_FDT_INTR_MPSAFE FDT_INTR_MPSAFE
#else
#define DWCGMAC_FDT_INTR_MPSAFE 0
#endif

/*
 * We could use 1024 DMA descriptors to fill up an 8k page (each is 16 byte).
 * However, on TX we probably will not need that many, and on RX we allocate
 * a full mbuf cluster for each, so secondary memory consumption will grow
 * rapidly.
 * So currently we waste half a page of dma memory and consume 512k Byte of
 * RAM for mbuf clusters.
 * XXX Maybe fine-tune later, or reconsider unsharing of RX/TX dmamap.
 */
#define		AWGE_RX_RING_COUNT	256
#define		AWGE_TX_RING_COUNT	256
#define		AWGE_TOTAL_RING_COUNT	\
			(AWGE_RX_RING_COUNT + AWGE_TX_RING_COUNT)

#define		AWGE_MAX_PACKET		0x7ff

struct dwc_gmac_dev_dmadesc;

struct dwc_gmac_desc_methods {
	void (*tx_init_flags)(struct dwc_gmac_dev_dmadesc *);
	void (*tx_set_owned_by_dev)(struct dwc_gmac_dev_dmadesc *);
	int  (*tx_is_owned_by_dev)(struct dwc_gmac_dev_dmadesc *);
	void (*tx_set_len)(struct dwc_gmac_dev_dmadesc *, int);
	void (*tx_set_first_frag)(struct dwc_gmac_dev_dmadesc *);
	void (*tx_set_last_frag)(struct dwc_gmac_dev_dmadesc *);

	void (*rx_init_flags)(struct dwc_gmac_dev_dmadesc *);
	void (*rx_set_owned_by_dev)(struct dwc_gmac_dev_dmadesc *);
	int  (*rx_is_owned_by_dev)(struct dwc_gmac_dev_dmadesc *);
	void (*rx_set_len)(struct dwc_gmac_dev_dmadesc *, int);
	uint32_t  (*rx_get_len)(struct dwc_gmac_dev_dmadesc *);
	int  (*rx_has_error)(struct dwc_gmac_dev_dmadesc *);
};

struct dwc_gmac_rx_data {
	bus_dmamap_t	rd_map;
	struct mbuf	*rd_m;
};

struct dwc_gmac_tx_data {
	bus_dmamap_t	td_map;
	bus_dmamap_t	td_active;
	struct mbuf	*td_m;
};

struct dwc_gmac_tx_ring {
	bus_addr_t			t_physaddr; /* PA of TX ring start */
	struct dwc_gmac_dev_dmadesc	*t_desc;    /* VA of TX ring start */
	struct dwc_gmac_tx_data	t_data[AWGE_TX_RING_COUNT];
	int				t_cur, t_next, t_queued;
	kmutex_t			t_mtx;
};

struct dwc_gmac_rx_ring {
	bus_addr_t			r_physaddr; /* PA of RX ring start */
	struct dwc_gmac_dev_dmadesc	*r_desc;    /* VA of RX ring start */
	struct dwc_gmac_rx_data	r_data[AWGE_RX_RING_COUNT];
	int				r_cur, r_next;
	kmutex_t			r_mtx;
};

struct dwc_gmac_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	uint32_t sc_flags;
#define	DWC_GMAC_FORCE_THRESH_DMA_MODE	0x01	/* force DMA to use threshold mode */
	struct ethercom sc_ec;
	struct mii_data sc_mii;
	kmutex_t sc_mdio_lock;
	bus_dmamap_t sc_dma_ring_map;		/* common dma memory for RX */
	bus_dma_segment_t sc_dma_ring_seg;	/* and TX ring */
	struct dwc_gmac_rx_ring sc_rxq;
	struct dwc_gmac_tx_ring sc_txq;
	const struct dwc_gmac_desc_methods *sc_descm;
	u_short sc_if_flags;			/* shadow of ether flags */
	uint16_t sc_mii_clk;
	bool sc_stopping;
	krndsource_t rnd_source;
	kmutex_t *sc_lock;			/* lock for softc operations */

	struct if_percpuq *sc_ipq;		/* softint-based input queues */

	void (*sc_set_speed)(struct dwc_gmac_softc *, int);
};

int dwc_gmac_attach(struct dwc_gmac_softc*, int /*phy_id*/,
    uint32_t /*mii_clk*/);
int dwc_gmac_intr(struct dwc_gmac_softc*);
