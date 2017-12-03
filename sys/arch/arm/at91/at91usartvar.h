/*      $NetBSD: at91usartvar.h,v 1.5.6.2 2017/12/03 11:35:51 jdolecek Exp $	*/

/*-
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
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

#ifndef _AT91USARTVAR_H_
#define _AT91USARTVAR_H_

#include <sys/tty.h>
#include <arm/at91/at91pdcvar.h>
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

#define AT91USART_RING_SIZE	MAX(PAGE_SIZE, 2048)

struct at91usart_softc {
	device_t		sc_dev;
	bus_addr_t		sc_hwbase;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t 	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	int			sc_pid;

	/* hardware flow control callbacks: */
	void			(*hwflow)(struct at91usart_softc *, int enabled);
	void			(*start_tx)(struct at91usart_softc *sc);
	void			(*stop_tx)(struct at91usart_softc *sc);
	void			(*rx_started)(struct at91usart_softc *sc);
	void			(*rx_stopped)(struct at91usart_softc *sc);
	void			(*rx_rts_ctl)(struct at91usart_softc *sc, int enabled);

	at91pdc_fifo_t		sc_rx_fifo;
	at91pdc_fifo_t		sc_tx_fifo;

	u_char			*sc_tba;
	int			sc_tbc;

	void			*sc_si;

	struct tty		*sc_tty;

	/* status flags */
	int			sc_hwflags, sc_swflags;

	volatile u_int		sc_rx_flags,
				sc_rx_ready;

	volatile u_int		sc_csr, sc_brgr, sc_ier, sc_poll;

	/* power management hooks */
	int			(*enable)(struct at91usart_softc *);
	int			(*disable)(struct at91usart_softc *);

	int			enabled;
#ifdef RND_COM
	krndsource_t  rnd_source;
#endif
};

struct at91bus_attach_args;
void	at91usart_attach_subr(struct at91usart_softc *, struct at91bus_attach_args *);
int	at91usart_cn_attach(bus_space_tag_t, bus_addr_t, bus_space_handle_t, uint32_t mstclk, int, tcflag_t);

#endif /* _AT91USARTVAR_H_ */
