/*-
 * Copyright (c) 2001, 2002, 2005 Genetec corp.
 * All rights reserved.
 *
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _EVBARM_G42XXEB_VAR_H
#define _EVBARM_G42XXEB_VAR_H

#include <sys/conf.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <evbarm/g42xxeb/g42xxeb_reg.h>


/* 
 * G42xxeb on-board IO bus
 */
struct obio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_obioreg_ioh;

	/* handle to PXA2x0's memory controller.
	   XXX: shouldn't be here. */
	bus_space_handle_t sc_memctl_ioh;

	void	*sc_ih;		/* interrupt handler for obio on pxaip */
	void	*sc_si;		/* software interrupt handler */
	int	sc_intr;
	uint16_t  sc_intr_mask;
	uint16_t  sc_intr_pending;
	int	sc_ipl;		/* Max ipl among sub interrupts */
	struct obio_handler {
		int	(* func)(void *);
		void	*arg;
		int	level;
	} sc_handler[G42XXEB_N_INTS];
};

typedef void *obio_chipset_tag_t;

struct obio_attach_args {
	obio_chipset_tag_t	oba_sc;		
	bus_space_tag_t		oba_iot; 	/* Bus tag */
	bus_addr_t		oba_addr;	/* i/o address  */
	int			oba_intr;
};

/* on-board hex LED */
void hex_led( uint32_t value );

/*
 * IRQ handler
 */
void *obio_intr_establish(struct obio_softc *, int, int, int, 
    int (*)(void *), void *);
void obio_intr_disestablish(struct obio_softc *, int, int (*)(void *));
void obio_intr_mask(struct obio_softc *, struct obio_handler *);
void obio_intr_unmask(struct obio_softc *, struct obio_handler *);

#define obio_update_intrmask(sc) \
	bus_space_write_2( (sc)->sc_iot, (sc)->sc_obioreg_ioh, \
	    G42XXEB_INTMASK, (sc)->sc_intr_mask | (sc)->sc_intr_pending );
	
void obio_peripheral_reset(struct obio_softc *, int, int);

#endif /* _EVBARM_G42XXEB_VAR_H */
