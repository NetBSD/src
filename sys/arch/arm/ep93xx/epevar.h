/*      $NetBSD: epevar.h,v 1.3.26.1 2007/03/12 05:47:03 rmind Exp $        */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	int			sc_intr;
	u_int8_t		sc_enaddr[ETHER_ADDR_LEN];
	struct ethercom		sc_ec;
	mii_data_t		sc_mii;
	void *			ctrlpage;
	bus_addr_t		ctrlpage_dsaddr;
	bus_dmamap_t		ctrlpage_dmamap;
	u_int32_t		*TXDQ;
	u_int32_t		TXDQ_avail;
	u_int32_t		*TXDQ_cur;
	u_int32_t		*TXStsQ;
	u_int32_t		*TXStsQ_cur;
	u_int32_t		*RXDQ;
	u_int32_t		*RXStsQ;
	u_int32_t		*RXStsQ_cur;
	struct epe_qmeta	rxq[RX_QLEN];
	struct epe_qmeta	txq[TX_QLEN];
	struct callout		epe_tick_ch;
};

#endif /* _EPEVAR_H_ */
