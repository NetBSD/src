/*	$NetBSD: epgpiovar.h,v 1.1 2005/11/12 05:33:23 hamajima Exp $	*/

/*
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

#ifndef	_EPGPIOVAR_H_
#define	_EPGPIOVAR_H_

typedef enum {
	PORT_A = 0,
	PORT_B = 1,
	PORT_C = 2,
	PORT_D = 3,
	PORT_E = 4,
	PORT_F = 5,
	PORT_G = 6,
	PORT_H = 7
} epgpio_port;

struct epgpio_softc;

int epgpio_read(struct epgpio_softc *,epgpio_port, int bit);
void epgpio_set(struct epgpio_softc *,epgpio_port, int bit);
void epgpio_clear(struct epgpio_softc *,epgpio_port, int bit);
void epgpio_in(struct epgpio_softc *,epgpio_port, int bit);
void epgpio_out(struct epgpio_softc *,epgpio_port, int bit);

void *epgpio_intr_establish(struct epgpio_softc *,epgpio_port, int bit,
			    int flag, int ipl, int (*)(void *), void *);
void epgpio_intr_disestablish(struct epgpio_softc *,epgpio_port, int bit);

/* interrupt flags */
#define	EDGE_TRIGGER	0x01
#define	LEVEL_SENSE	0x00
#define	RISING_EDGE	0x02
#define	HIGH_LEVEL	0x02
#define	FALLING_EDGE	0x00
#define	LOW_LEVEL	0x00
#define	DEBOUNCE	0x04

struct epgpio_attach_args {
	struct epgpio_softc	*ga_gc;
	bus_space_tag_t		ga_iot;		/* Bus tag */
	bus_addr_t		ga_addr;	/* i/o address */
	int			ga_port;
	int			ga_bit1;
	int			ga_bit2;
};

#endif	/* _EPGPIOVAR_H_ */
