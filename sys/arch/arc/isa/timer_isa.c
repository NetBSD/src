/*	$NetBSD: timer_isa.c,v 1.7 2003/08/07 16:26:50 agc Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: timer_isa.c,v 1.7 2003/08/07 16:26:50 agc Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/isa/isareg.h>
#include <dev/ic/i8253reg.h>

#include <arc/arc/timervar.h>
#include <arc/isa/timer_isavar.h>

#define TIMER_IOSIZE	4
#define TIMER_IRQ	0

struct timer_isa_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

/* Definition of the driver for autoconfig. */
int timer_isa_match __P((struct device *, struct cfdata *, void *));
void timer_isa_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(timer_isa, sizeof(struct timer_isa_softc),
    timer_isa_match, timer_isa_attach, NULL, NULL);

/* ISA timer access code */
void timer_isa_init __P((struct device *));

struct timerfns timerfns_isa = {
	timer_isa_init
};

int timer_isa_conf = 0;

int
timer_isa_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;

	if (ia->ia_nio < 1 ||
	    (ia->ia_io[0].ir_addr != ISACF_PORT_DEFAULT &&
	     ia->ia_io[0].ir_addr != IO_TIMER1))
		return (0);

	if (ia->ia_niomem > 0 &&
	    (ia->ia_iomem[0].ir_addr != ISACF_IOMEM_DEFAULT))
		return (0);

	if (ia->ia_nirq > 0 &&
	    (ia->ia_irq[0].ir_irq != ISACF_IRQ_DEFAULT &&
	     ia->ia_irq[0].ir_irq != TIMER_IRQ))
		return (0);

	if (ia->ia_ndrq > 0 &&
	    (ia->ia_drq[0].ir_drq != ISACF_DRQ_DEFAULT))
		return (0);

	if (!timer_isa_conf)
		return (0);

	if (bus_space_map(ia->ia_iot, IO_TIMER1, TIMER_IOSIZE, 0, &ioh))
		return (0);

	bus_space_unmap(ia->ia_iot, ioh, TIMER_IOSIZE);

	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = IO_TIMER1;
	ia->ia_io[0].ir_size = TIMER_IOSIZE;

	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return (1);
}

void
timer_isa_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct timer_isa_softc *sc = (struct timer_isa_softc *)self;
	struct isa_attach_args *ia = aux;
	void *ih;

	printf("\n");

	sc->sc_iot = ia->ia_iot;
	if (bus_space_map(sc->sc_iot, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_ioh))
		panic("timer_isa_attach: couldn't map clock I/O space");

	ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq, IST_PULSE,
	    IPL_CLOCK, (int (*)(void *))hardclock,
	    NULL /* clockframe is hardcoded */);
	if (ih == NULL)
		printf("%s: can't establish interrupt\n", sc->sc_dev.dv_xname);

	timerattach(&sc->sc_dev, &timerfns_isa);
}

void
timer_isa_init(self)
	struct device *self;
{
	struct timer_isa_softc *sc = (struct timer_isa_softc *)self;

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_MODE,
	    TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_CNTR0,
	    TIMER_DIV(hz) % 256);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_CNTR0,
	    TIMER_DIV(hz) / 256);
}
