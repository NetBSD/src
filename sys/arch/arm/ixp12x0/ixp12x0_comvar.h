/*      $NetBSD: ixp12x0_comvar.h,v 1.6.34.1 2012/10/30 17:19:05 yamt Exp $        */
/*-
 * Copyright (c) 2001, The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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

#ifndef _IXP12X0_COMVAR_H_
#define _IXP12X0_COMVAR_H_

/* Hardware flag masks */
#define COM_HW_NOIEN		0x01
#define COM_HW_DEV_OK		0x20
#define COM_HW_CONSOLE		0x40
#define COM_HW_KGDB		0x80

#define RX_TTY_BLOCKED		0x01
#define RX_TTY_OVERFLOWED	0x02
#define RX_IBUF_BLOCKED		0x04
#define RX_IBUF_OVERFLOWED	0x08
#define RX_ANY_BLOCK		0x0f

#define IXPCOM_RING_SIZE	2048

struct ixpcom_softc {
	device_t		sc_dev;
	bus_addr_t		sc_baseaddr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t 	sc_ioh;

	void			*sc_si;

	struct tty		*sc_tty;

	u_char			*sc_rbuf, *sc_ebuf;

 	u_char			*sc_tba;
 	u_int			sc_tbc, sc_heldtbc;

	u_char			*volatile sc_rbget,
				*volatile sc_rbput;
 	volatile u_int		sc_rbavail;

	/* status flags */
	int			sc_hwflags, sc_swflags;

	volatile u_int		sc_rx_flags,
				sc_tx_busy,
				sc_tx_done,
				sc_tx_stopped,
				sc_st_check,
				sc_rx_ready;
	volatile int		sc_heldchange;

	/* control registers */
	u_int			sc_xie;
	u_int			sc_rie;
	u_int			sc_speed;

	/* power management hooks */
	int			(*enable)(struct ixpcom_softc *);
	int			(*disable)(struct ixpcom_softc *);

	int			enabled;
};

extern struct ixpcom_softc* ixpcom_sc;

void	ixpcom_attach_subr(struct ixpcom_softc *);

int	ixpcomintr(void* arg);
int	ixpcomcnattach(bus_space_tag_t, bus_addr_t, bus_space_handle_t,
		       int, tcflag_t);

#endif /* _IXP12X0_COMVAR_H_ */
