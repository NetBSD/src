/* $NetBSD: dwc_eqos_var.h,v 1.2 2022/01/08 22:24:53 mrg Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * DesignWare Ethernet Quality-of-Service controller
 */

#ifndef _DWC_EQOS_VAR_H
#define _DWC_EQOS_VAR_H

#include <dev/ic/dwc_eqos_reg.h>

#define	EQOS_DMA_DESC_COUNT	256

struct eqos_bufmap {
	bus_dmamap_t		map;
	struct mbuf		*mbuf;
};

struct eqos_ring {
	bus_dmamap_t		desc_map;
	bus_dma_segment_t	desc_dmaseg;
	struct eqos_dma_desc	*desc_ring;
	bus_addr_t		desc_ring_paddr;
	struct eqos_bufmap	buf_map[EQOS_DMA_DESC_COUNT];
	u_int			cur, next, queued;
};

struct eqos_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	int			sc_phy_id;
	uint32_t		sc_csr_clock;
	uint32_t		sc_clock_range;

	uint32_t		sc_hw_feature[4];

	struct ethercom		sc_ec;
	struct mii_data		sc_mii;
	callout_t		sc_stat_ch;
	kmutex_t		sc_lock;
	kmutex_t		sc_txlock;

	struct eqos_ring	sc_tx;
	struct eqos_ring	sc_rx;

	krndsource_t		sc_rndsource;

	/* Indents indicate groups within evcnt. */
	struct evcnt		sc_ev_intr;
	struct evcnt		 sc_ev_rxintr;
	struct evcnt		 sc_ev_txintr;
	struct evcnt		 sc_ev_mac;
	struct evcnt		 sc_ev_mtl;
	struct evcnt		 sc_ev_status;
	struct evcnt		  sc_ev_rwt;
	struct evcnt		  sc_ev_excol;
	struct evcnt		  sc_ev_lcol;
	struct evcnt		  sc_ev_exdef;
	struct evcnt		  sc_ev_lcarr;
	struct evcnt		  sc_ev_ncarr;
	struct evcnt		  sc_ev_tjt;
};

int	eqos_attach(struct eqos_softc *);
int	eqos_intr(void *);

#endif /* !_DWC_EQOS_VAR_H */
