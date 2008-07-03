/*	$NetBSD: at91piovar.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Based on arch/arm/ep93xx/epgiovar.h
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_AT91PIOVAR_H_
#define	_AT91PIOVAR_H_

typedef enum {
  AT91_PIOA,
  AT91_PIOB,
  AT91_PIOC,
  AT91_PIOD,
  AT91_PIOE,
  AT91_PIOF,
  AT91_PIOG,
  AT91_PIOH,
  AT91_PIO_COUNT,
  AT91_PIO_INVALID = AT91_PIO_COUNT
} at91pio_port;

struct at91pio_softc;

struct at91pio_softc *at91pio_sc(at91pio_port);

int at91pio_read(struct at91pio_softc *, int bit);
void at91pio_set(struct at91pio_softc *, int bit);
void at91pio_clear(struct at91pio_softc *, int bit);
void at91pio_in(struct at91pio_softc *, int bit);
void at91pio_out(struct at91pio_softc *, int bit);
void at91pio_per(struct at91pio_softc *, int bit, int perab);

void *at91pio_intr_establish(struct at91pio_softc *,int bit,
			       int ipl, int (*)(void *), void *);
void at91pio_intr_disestablish(struct at91pio_softc *,int bit,
				 void *cookie);

struct at91pio_attach_args {
	struct at91pio_softc	*paa_sc;
	bus_space_tag_t		paa_iot;	/* Bus tag	*/
	bus_addr_t		paa_addr;	/* i/o address	*/
	int			paa_pid;	/* peripheral id */
	int			paa_bit;	/* pin number	*/
};

#endif	/* _AT91PIOVAR_H_ */
