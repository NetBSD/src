/*	$NetBSD: lpt_pioc.c,v 1.1.2.2 1997/10/15 05:41:50 thorpej Exp $	*/

/*
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1990 William F. Jolitz, TeleMuse
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This software is a component of "386BSD" developed by 
 *	William F. Jolitz, TeleMuse.
 * 4. Neither the name of the developer nor the name "386BSD"
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS A COMPONENT OF 386BSD DEVELOPED BY WILLIAM F. JOLITZ 
 * AND IS INTENDED FOR RESEARCH AND EDUCATIONAL PURPOSES ONLY. THIS 
 * SOFTWARE SHOULD NOT BE CONSIDERED TO BE A COMMERCIAL PRODUCT. 
 * THE DEVELOPER URGES THAT USERS WHO REQUIRE A COMMERCIAL PRODUCT 
 * NOT MAKE USE OF THIS WORK.
 *
 * FOR USERS WHO WISH TO UNDERSTAND THE 386BSD SYSTEM DEVELOPED
 * BY WILLIAM F. JOLITZ, WE RECOMMEND THE USER STUDY WRITTEN 
 * REFERENCES SUCH AS THE  "PORTING UNIX TO THE 386" SERIES 
 * (BEGINNING JANUARY 1991 "DR. DOBBS JOURNAL", USA AND BEGINNING 
 * JUNE 1991 "UNIX MAGAZIN", GERMANY) BY WILLIAM F. JOLITZ AND 
 * LYNNE GREER JOLITZ, AS WELL AS OTHER BOOKS ON UNIX AND THE 
 * ON-LINE 386BSD USER MANUAL BEFORE USE. A BOOK DISCUSSING THE INTERNALS 
 * OF 386BSD ENTITLED "386BSD FROM THE INSIDE OUT" WILL BE AVAILABLE LATE 1992.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE DEVELOPER BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Device Driver for AT parallel printer port
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/irqhandler.h>
#include <arm32/dev/lptreg.h>
#include <arm32/dev/lptvar.h>
#include <arm32/mainbus/piocvar.h>

#include "locators.h"

/* Prototypes for functions */

static int  lpt_pioc_probe  __P((struct device *, struct cfdata *, void *));
static void lpt_pioc_attach __P((struct device *, struct device *, void *));

/* device attach structure */

struct cfattach lpt_pioc_ca = {
	sizeof(struct lpt_softc), lpt_pioc_probe, lpt_pioc_attach
};

/*
 * int lpt_pioc_probe(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Make sure we are trying to attach a lpt device and then
 * probe for one.
 */

static int
lpt_pioc_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pioc_attach_args *pa = aux;
	int rv;

	if (pa->pa_name && strcmp(pa->pa_name, "lpt") != 0)
		return(0);

	/* We need an offset */
	if (pa->pa_offset == PIOCCF_OFFSET_DEFAULT)
		return(0);

	rv = lptprobe(pa->pa_iot, pa->pa_iobase + pa->pa_offset);

	if (rv) {
		pa->pa_iosize = rv;
		return(1);
	}
	return(0);
}

/*
 * void lpt_pioc_attach(struct device *parent, struct device *self, void *aux)
 *
 * attach the lpt device
 */

static void
lpt_pioc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct lpt_softc *sc = (void *)self;
	struct pioc_attach_args *pa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	if (pa->pa_irq != MAINBUSCF_IRQ_DEFAULT) {
		printf("\n");
		sc->sc_irq = pa->pa_irq;
	} else {
		printf(": polled\n");
		sc->sc_irq = LPT_NOIRQ;
	}

	sc->sc_iobase = pa->pa_iobase + pa->pa_offset;
	sc->sc_state = 0;

	iot = sc->sc_iot = pa->pa_iot;
	if (bus_space_map(iot, sc->sc_iobase, LPT_NPORTS, 0, &ioh))
		panic("lptattach: couldn't map I/O ports");
	sc->sc_ioh = ioh;

	bus_space_write_1(iot, ioh, lpt_control, LPC_NINIT);

	if (pa->pa_irq != MAINBUSCF_IRQ_DEFAULT)
		sc->sc_ih = intr_claim(pa->pa_irq, IPL_TTY, "lpt",
		    lptintr, sc);
}

/* End of lpt_pioc.c */
