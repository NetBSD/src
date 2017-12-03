/*      $NetBSD: epcomvar.h,v 1.6.6.2 2017/12/03 11:35:52 jdolecek Exp $        */
/*-
 * Copyright (c) 2004 Jesse Off
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

#ifndef _EPCOMVAR_H_
#define _EPCOMVAR_H_

#ifdef RND_COM
#include <sys/rndsource.h>
#endif

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

#define EPCOM_RING_SIZE	2048

struct epcom_softc {
	device_t		sc_dev;
	bus_addr_t		sc_hwbase;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t 	sc_ioh;

	void			*sc_si;

	struct tty		*sc_tty;

	u_char			*sc_rbuf, *sc_ebuf;

 	u_char			*sc_tba;
 	u_int			sc_tbc;

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

	/* control registers */
	u_int			sc_lcrlo;
	u_int			sc_lcrmid;
	u_int			sc_lcrhi;
	u_int			sc_ctrl;

	/* power management hooks */
	int			(*enable)(struct epcom_softc *);
	int			(*disable)(struct epcom_softc *);

	int			enabled;
#ifdef RND_COM
	krndsource_t  rnd_source;
#endif
};

void	epcom_attach_subr(struct epcom_softc *);

int	epcomintr(void* arg);
int	epcomcnattach(bus_space_tag_t, bus_addr_t, bus_space_handle_t,
		       int, tcflag_t);

#endif /* _EPCOMVAR_H_ */
