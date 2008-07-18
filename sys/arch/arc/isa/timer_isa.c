/*	$NetBSD: timer_isa.c,v 1.11.52.1 2008/07/18 16:37:26 simonb Exp $	*/
/*	$OpenBSD: clock_mc.c,v 1.9 1998/03/16 09:38:26 pefo Exp $	*/
/*	NetBSD: clock_mc.c,v 1.2 1995/06/28 04:30:30 cgd Exp 	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: timer_isa.c,v 1.11.52.1 2008/07/18 16:37:26 simonb Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/i8253reg.h>

#include <arc/arc/timervar.h>
#include <arc/isa/timer_isavar.h>

#define TIMER_IOSIZE	4
#define TIMER_IRQ	0

struct timer_isa_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

/* Definition of the driver for autoconfig. */
static int timer_isa_match(device_t, cfdata_t, void *);
static void timer_isa_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(timer_isa, sizeof(struct timer_isa_softc),
    timer_isa_match, timer_isa_attach, NULL, NULL);

/* ISA timer access code */
static void timer_isa_init(device_t);

struct timerfns timerfns_isa = {
	timer_isa_init
};

int timer_isa_conf = 0;

static int
timer_isa_match(device_t parent, cfdata_t cf, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;

	if (ia->ia_nio < 1 ||
	    (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT &&
	     ia->ia_io[0].ir_addr != IO_TIMER1))
		return 0;

	if (ia->ia_niomem > 0 &&
	    (ia->ia_iomem[0].ir_addr != ISA_UNKNOWN_IOMEM))
		return 0;

	if (ia->ia_nirq > 0 &&
	    (ia->ia_irq[0].ir_irq != ISA_UNKNOWN_IRQ &&
	     ia->ia_irq[0].ir_irq != TIMER_IRQ))
		return 0;

	if (ia->ia_ndrq > 0 &&
	    (ia->ia_drq[0].ir_drq != ISA_UNKNOWN_DRQ))
		return 0;

	if (!timer_isa_conf)
		return 0;

	if (bus_space_map(ia->ia_iot, IO_TIMER1, TIMER_IOSIZE, 0, &ioh))
		return 0;

	bus_space_unmap(ia->ia_iot, ioh, TIMER_IOSIZE);

	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = IO_TIMER1;
	ia->ia_io[0].ir_size = TIMER_IOSIZE;

	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return 1;
}

static void
timer_isa_attach(device_t parent, device_t self, void *aux)
{
	struct timer_isa_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;
	void *ih;

	sc->sc_dev = self;

	aprint_normal("\n");

	sc->sc_iot = ia->ia_iot;
	if (bus_space_map(sc->sc_iot, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_ioh))
		panic("timer_isa_attach: couldn't map clock I/O space");

	ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq, IST_PULSE,
	    IPL_CLOCK, (int (*)(void *))hardclock,
	    NULL /* clockframe is hardcoded */);
	if (ih == NULL)
		aprint_error_dev(self, "can't establish interrupt\n");

	timerattach(self, &timerfns_isa);
}

static void
timer_isa_init(device_t self)
{
	struct timer_isa_softc *sc = device_private(self);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_MODE,
	    TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_CNTR0,
	    TIMER_DIV(hz) % 256);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_CNTR0,
	    TIMER_DIV(hz) / 256);
}
