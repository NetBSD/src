/*	$NetBSD: if_ecavar.h,v 1.2 2001/09/15 17:27:25 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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

#ifndef _ECAVAR_H_
#define _ECAVAR_H_

#ifndef _LOCORE
#include <net/if.h>
#include <net/if_eco.h>

struct eca_rxstate {
	struct mbuf	*erx_curmbuf;
	u_int32_t	erx_flags;
	u_int8_t	erx_myaddr;
};

struct eca_txstate {
	struct mbuf	*etx_curmbuf;
};

struct eca_softc {
	struct device	sc_dev;
	struct ecocom 	sc_ec;
	u_int8_t	sc_cr1;
	u_int8_t	sc_cr2;
	u_int8_t	sc_cr3;
	u_int8_t	sc_cr4;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct mbuf	*sc_rcvmbuf;
	void		*sc_rx_soft;
	struct eca_rxstate sc_rxstate;
	u_int8_t	sc_sr2;
	struct mbuf	*sc_txmbuf;
	void		*sc_tx_soft;
	struct eca_txstate sc_txstate;
	int		sc_transmitting;
};

extern char eca_fiqhandler_rx[], eca_efiqhandler_rx[];
extern char eca_fiqhandler_tx[], eca_efiqhandler_tx[];

#endif
/* Sync with rxstate above */
#define ERX_CURMBUF	0
#define ERX_FLAGS	4
#define ERXF_GOTHDR	0x01
#define ERXF_FLAGFILL	0x02
#define ERX_MYADDR	8
/* Sync with txstate above */
#define ETX_CURMBUF	0

#endif
