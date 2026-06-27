/*	$NetBSD: fecvar.h,v 1.1 2026/06/27 13:28:34 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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

#ifndef _POWERPC_MPC5200_FECVAR_H_
#define _POWERPC_MPC5200_FECVAR_H_

#include <sys/device.h>
#include <sys/bus.h>
#include <sys/callout.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <dev/mii/miivar.h>

#include <powerpc/mpc5200/bestcommvar.h>

#define	FEC_NRXDESC	256	/* receive buffer descriptors	*/
#define	FEC_NTXDESC	32	/* transmit buffer descriptors	*/

struct fec_softc {
	device_t		sc_dev;
	struct ethercom		sc_ec;		/* Ethernet common	*/
#define	sc_if		sc_ec.ec_if
	struct mii_data		sc_mii;		/* MII / PHY		*/
	callout_t		sc_tick_ch;	/* MII tick		*/

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	bus_addr_t		sc_pa;		/* FEC block physical base */

	void			*sc_ih;		/* MAC interrupt handle	*/
	int			sc_irq;		/* flat MAC IRQ		*/

	uint8_t			sc_enaddr[ETHER_ADDR_LEN];

	uint32_t		sc_ipb_freq;	/* IPB clock (MII divisor) */

	/* BestComm receive/transmit rings. */
	struct bestcomm_bdring	sc_rxring;
	struct bestcomm_bdring	sc_txring;
	struct mbuf		*sc_rxmbuf[FEC_NRXDESC];
	bus_dmamap_t		sc_rxmap[FEC_NRXDESC];
	struct mbuf		*sc_txmbuf[FEC_NTXDESC];
	bus_dmamap_t		sc_txmap[FEC_NTXDESC];
	u_int			sc_rxptr;	/* next rx BD to inspect */
	u_int			sc_txnext;	/* next tx BD to fill	*/
	u_int			sc_txdirty;	/* oldest unreaped tx BD */
	u_int			sc_txbusy;	/* tx BDs in flight	*/
	void			*sc_rxih;	/* rx completion intr	*/
	void			*sc_txih;	/* tx completion intr	*/
};

#endif /* _POWERPC_MPC5200_FECVAR_H_ */
