/*	$NetBSD: if_enetvar.h,v 1.2.4.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IF_ENETVAR_H_
#define _ARM_IMX_IF_ENETVAR_H_

#include <sys/rndsource.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <dev/mii/miivar.h>

#define ENET_TX_RING_CNT	256
#define ENET_RX_RING_CNT	256

struct enet_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
};

struct enet_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

struct enet_softc {
	device_t sc_dev;

	bus_addr_t sc_addr;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;

	int sc_unit;
	int sc_imxtype;
	int sc_rgmii;
	unsigned int sc_pllclock;

	/* interrupts */
	void *sc_ih;
	void *sc_ih2;	/* for i.MX7 */
	void *sc_ih3;	/* for i.MX7 */
	callout_t sc_tick_ch;
	bool sc_stopping;

	/* TX */
	struct enet_txdesc *sc_txdesc_ring;	/* [ENET_TX_RING_CNT] */
	bus_dmamap_t sc_txdesc_dmamap;
	struct enet_rxdesc *sc_rxdesc_ring;	/* [ENET_RX_RING_CNT] */
	bus_dmamap_t sc_rxdesc_dmamap;
	struct enet_txsoft sc_txsoft[ENET_TX_RING_CNT];
	int sc_tx_considx;
	int sc_tx_prodidx;
	int sc_tx_free;

	/* RX */
	struct enet_rxsoft sc_rxsoft[ENET_RX_RING_CNT];
	int sc_rx_readidx;

	/* misc */
	int sc_if_flags;			/* local copy of if_flags */
	int sc_flowflags;			/* 802.3x flow control flags */
	struct ethercom sc_ethercom;		/* interface info */
	struct mii_data sc_mii;
	uint8_t sc_enaddr[ETHER_ADDR_LEN];
	krndsource_t sc_rnd_source;

#ifdef ENET_EVENT_COUNTER
	struct evcnt sc_ev_t_drop;
	struct evcnt sc_ev_t_packets;
	struct evcnt sc_ev_t_bc_pkt;
	struct evcnt sc_ev_t_mc_pkt;
	struct evcnt sc_ev_t_crc_align;
	struct evcnt sc_ev_t_undersize;
	struct evcnt sc_ev_t_oversize;
	struct evcnt sc_ev_t_frag;
	struct evcnt sc_ev_t_jab;
	struct evcnt sc_ev_t_col;
	struct evcnt sc_ev_t_p64;
	struct evcnt sc_ev_t_p65to127n;
	struct evcnt sc_ev_t_p128to255n;
	struct evcnt sc_ev_t_p256to511;
	struct evcnt sc_ev_t_p512to1023;
	struct evcnt sc_ev_t_p1024to2047;
	struct evcnt sc_ev_t_p_gte2048;
	struct evcnt sc_ev_t_octets;
	struct evcnt sc_ev_r_packets;
	struct evcnt sc_ev_r_bc_pkt;
	struct evcnt sc_ev_r_mc_pkt;
	struct evcnt sc_ev_r_crc_align;
	struct evcnt sc_ev_r_undersize;
	struct evcnt sc_ev_r_oversize;
	struct evcnt sc_ev_r_frag;
	struct evcnt sc_ev_r_jab;
	struct evcnt sc_ev_r_p64;
	struct evcnt sc_ev_r_p65to127;
	struct evcnt sc_ev_r_p128to255;
	struct evcnt sc_ev_r_p256to511;
	struct evcnt sc_ev_r_p512to1023;
	struct evcnt sc_ev_r_p1024to2047;
	struct evcnt sc_ev_r_p_gte2048;
	struct evcnt sc_ev_r_octets;
#endif /* ENET_EVENT_COUNTER */
};

void enet_attach_common(device_t, bus_space_tag_t, bus_dma_tag_t,
    bus_addr_t, bus_size_t, int);
int enet_match(device_t, cfdata_t, void *);
void enet_attach(device_t, device_t, void *);

#endif /* _ARM_IMX_IF_ENETVAR_H_ */
