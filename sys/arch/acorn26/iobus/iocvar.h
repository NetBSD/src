/* $NetBSD: iocvar.h,v 1.6.14.1 2012/05/23 10:07:36 yamt Exp $ */
/*-
 * Copyright (c) 1998, 1999 Ben Harris
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
/*
 * iocvar.h - aspects of the IOC driver that are visible to the world.
 */

#ifndef _ARM26_IOCVAR_H
#define _ARM26_IOCVAR_H

#include <sys/timetc.h>
#include <machine/irq.h>

/* Structure passed to children of an IOC */

struct ioc_attach_args {
	/* Means of accessing the device at various speeds */
	bus_space_tag_t		ioc_fast_t;
	bus_space_handle_t	ioc_fast_h;
	bus_space_tag_t		ioc_medium_t;
	bus_space_handle_t	ioc_medium_h;
	bus_space_tag_t		ioc_slow_t;
	bus_space_handle_t	ioc_slow_h;
	bus_space_tag_t		ioc_sync_t;
	bus_space_handle_t	ioc_sync_h;
	int			ioc_bank; /* only for ioc_print */
	int			ioc_offset;
};

struct ioc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct irq_handler	*sc_clkirq;
	struct evcnt		sc_clkev;
	struct irq_handler	*sc_sclkirq;
	struct evcnt		sc_sclkev;
	struct timecounter	sc_tc;
	u_int			sc_tcbase;
	uint8_t			sc_ctl;
};

extern device_t the_ioc;

/* Public IOC functions */

extern int ioc_irq_status(int);
extern void ioc_irq_waitfor(int);
extern void ioc_irq_clear(int);
extern uint32_t ioc_irq_status_full(void);
extern void ioc_irq_setmask(uint32_t);

extern void ioc_fiq_setmask(uint32_t);

extern void ioc_counter_start(device_t, int, int);

/*
 * Control Register
 */

/*
 * ioc_ctl_{read,write}
 *
 * Functions to manipulate the IOC control register.  The bottom six
 * bits of the control register map to bidirectional pins on the chip.
 * The output circuits are open-drain, so a pin is made an input by
 * writing '1' to it.
 */

static inline u_int
ioc_ctl_read(device_t self)
{
	struct ioc_softc *sc = device_private(self);

	return bus_space_read_1(sc->sc_bst, sc->sc_bsh, IOC_CTL);
}

static inline void
ioc_ctl_write(device_t self, u_int value, u_int mask)
{
	struct ioc_softc *sc = device_private(self);
	int s;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	
	s = splhigh();
	sc->sc_ctl = (sc->sc_ctl & ~mask) | (value & mask);
	bus_space_write_1(bst, bsh, IOC_CTL, sc->sc_ctl);
	splx(s);
}

#endif
