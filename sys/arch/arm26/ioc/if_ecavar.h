/*	$NetBSD: if_ecavar.h,v 1.3 2001/09/20 21:54:11 bjh21 Exp $	*/

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
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_eco.h>

#include <machine/bus.h>
#include <machine/fiq.h>

struct eca_fiqstate {
	void		*efs_fiqhandler;
	struct mbuf	*efs_rx_curmbuf;
	struct fiq_regs	efs_rx_fiqregs;
	u_int32_t	efs_rx_flags;
	u_int8_t	efs_rx_myaddr;
	struct mbuf	*efs_tx_curmbuf;
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
	void		*sc_rx_soft;
	struct eca_fiqstate sc_fiqstate;
	struct mbuf	*sc_rcvmbuf;
	struct mbuf	*sc_txmbuf;
	u_int8_t	sc_sr2;
	void		*sc_tx_soft;
	int		sc_transmitting;
};

extern char eca_fiqhandler[], eca_efiqhandler[];
extern char eca_fiqhandler_rx[], eca_fiqhandler_tx[];

#endif

/* Flags for efs_rx_flags. */
#define ERXF_GOTHDR	0x01
#define ERXF_FLAGFILL	0x02

#endif
