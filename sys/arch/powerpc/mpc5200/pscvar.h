/*	$NetBSD: pscvar.h,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2008, 2026 The NetBSD Foundation, Inc.
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

#ifndef _POWERPC_MPC5200_PSCVAR_H_
#define _POWERPC_MPC5200_PSCVAR_H_

#include <sys/device.h>
#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/tty.h>

#include <dev/cons.h>

#define	PSC_RING_SIZE	1024		/* receive ring (power of two) */
#define	PSC_RING_MASK	(PSC_RING_SIZE - 1)

struct psc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	int			sc_node;
	bool			sc_console;

	int			sc_irq;
	void			*sc_ih;		/* hard interrupt */
	void			*sc_si;		/* soft interrupt */
	kmutex_t		sc_lock;	/* protects the register set */
	struct tty		*sc_tty;
	uint16_t		sc_imr;		/* shadow of PSC_IMR */

	/* Receive ring, filled at IPL_HIGH, drained at IPL_SOFTSERIAL. */
	uint8_t			sc_rbuf[PSC_RING_SIZE];
	u_int			sc_rbput;	/* producer (interrupt) */
	u_int			sc_rbget;	/* consumer (soft) */
	volatile bool		sc_rx_ready;

	/* Transmit state. */
	uint8_t			*sc_tba;	/* next byte to send */
	u_int			sc_tbc;		/* bytes remaining */
	volatile bool		sc_tx_busy;
	volatile bool		sc_tx_done;
};

/* MPC5200 PSC console device, exported for the OFW console selection. */
extern struct consdev psccons;

#endif /* _POWERPC_MPC5200_PSCVAR_H_ */
