/*      $NetBSD: ixp425_comvar.h,v 1.1 2003/05/23 00:57:25 ichiro Exp $        */
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

#ifndef _IXP425_COMVAR_H_
#define _IXP425_COMVAR_H_

#define	FREQ			14745600

/* Hardware flag masks */
#define COM_HW_NOIEN		0x01
#define COM_HW_FIFO		0x02
#define COM_HW_DEV_OK		0x20
#define COM_HW_CONSOLE		0x40
#define COM_HW_KGDB		0x80

#define RX_TTY_BLOCKED		0x01
#define RX_TTY_OVERFLOWED	0x02
#define RX_IBUF_BLOCKED		0x04
#define RX_IBUF_OVERFLOWED	0x08
#define RX_ANY_BLOCK		0x0f

#define IXPCOM_RING_SIZE	2048

struct ixp4xx_com_softc {
	struct device		sc_dev;
	void			*sc_si;
	struct tty		*sc_tty;

	struct callout		sc_diag_callout;

	bus_addr_t		sc_baseaddr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t 	sc_ioh;

	int			sc_frequency;

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
	volatile int		sc_msr, sc_msr_delta, sc_msr_mask, sc_mcr,
				sc_mcr_active, sc_lcr, sc_ier, sc_fcr,
				sc_dlbl, sc_dlbh;
	u_char			sc_mcr_dtr, sc_mcr_rts, sc_msr_cts, sc_msr_dcd;

	/* power management hooks */
	int			(*enable)(struct ixp4xx_com_softc *);
	int			(*disable)(struct ixp4xx_com_softc *);

	int			enabled;
};

extern struct ixp4xx_com_softc* ixp4xx_com_sc;

int	ixp4xx_comspeed(long, long);

void	ixp4xx_com_attach_subr(struct ixp4xx_com_softc *);

int	ixp4xxcomintr(void* arg);

int	ixp4xx_comcnattach(bus_space_tag_t, bus_addr_t, bus_space_handle_t, int, int, tcflag_t);

#endif /* _IXP425_COMVAR_H_ */
