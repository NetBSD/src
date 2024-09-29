/*      $NetBSD: if_cemacvar.h,v 1.5 2024/09/29 09:13:14 skrll Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#ifndef _IF_CEMACVAR_H_
#define _IF_CEMACVAR_H_

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#define	RX_QLEN 64
#define	TX_QLEN	2		/* I'm very sorry but that's where we can get */

struct cemac_qmeta {
	struct mbuf	*m;
	bus_dmamap_t	m_dmamap;
};

struct cemac_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	uint8_t			sc_enaddr[ETHER_ADDR_LEN];
	struct ethercom		sc_ethercom;
	mii_data_t		sc_mii;

	void			*rbqpage;
	unsigned		rbqlen;
	bus_addr_t		rbqpage_dsaddr;
	bus_dmamap_t		rbqpage_dmamap;
	void			*tbqpage;
	unsigned		tbqlen;
	bus_addr_t		tbqpage_dsaddr;
	bus_dmamap_t		tbqpage_dmamap;

	volatile struct eth_dsc *RDSC;
	int			rxqi;
	struct cemac_qmeta	rxq[RX_QLEN];
	volatile struct eth_dsc *TDSC;
	int			txqi, txqc;
	struct cemac_qmeta	txq[TX_QLEN];
	callout_t		cemac_tick_ch;

	unsigned		cemac_flags;
#define CEMAC_FLAG_GEM	__BIT(0)

	bool			sc_txbusy;
};

int cemac_intr(void *);

void cemac_attach_common(struct cemac_softc *);


#endif /* _IF_CEMACVAR_H_ */
