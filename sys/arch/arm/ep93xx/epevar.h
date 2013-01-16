/*      $NetBSD: epevar.h,v 1.5.12.2 2013/01/16 05:32:46 yamt Exp $        */
/*-
 * Copyright (c) 2004 Jesse Off
 * All rights reserved
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _EPEVAR_H_
#define _EPEVAR_H_

#define TX_QLEN	16
#define RX_QLEN 64

struct epe_qmeta {
	struct mbuf 	*m;
	bus_dmamap_t	m_dmamap;
};

struct epe_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	int			sc_intr;
	uint8_t			sc_enaddr[ETHER_ADDR_LEN];
	struct ethercom		sc_ec;
	mii_data_t		sc_mii;
	void *			ctrlpage;
	bus_addr_t		ctrlpage_dsaddr;
	bus_dmamap_t		ctrlpage_dmamap;
	uint32_t		*TXDQ;
	uint32_t		TXDQ_avail;
	uint32_t		*TXDQ_cur;
	uint32_t		*TXStsQ;
	uint32_t		*TXStsQ_cur;
	uint32_t		*RXDQ;
	uint32_t		*RXStsQ;
	uint32_t		*RXStsQ_cur;
	struct epe_qmeta	rxq[RX_QLEN];
	struct epe_qmeta	txq[TX_QLEN];
	struct callout		epe_tick_ch;
};

#endif /* _EPEVAR_H_ */
