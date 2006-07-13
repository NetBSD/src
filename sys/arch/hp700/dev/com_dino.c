/*	$OpenBSD: com_dino.c,v 1.1 2004/02/13 20:39:31 mickey Exp $	*/

/*
 * Copyright (c) 2004 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF MIND,
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <hp700/dev/cpudevs.h>

void *dino_intr_establish(void *sc, int irq, int pri,
    int (*handler)(void *v), void *arg);

#define	COM_DINO_FREQ	7272700

struct com_dino_softc {
	struct  com_softc sc_com;	/* real "com" softc */
	void	*sc_ih;			/* interrupt handler */
};

struct com_dino_regs {
	u_int8_t	reset;
	u_int8_t	pad0[3];
	u_int8_t	test;
#define	COM_DINO_PAR_LOOP	0x01
#define	COM_DINO_CLK_SEL	0x02
	u_int8_t	pad1[3];
	u_int32_t	iodc;
	u_int8_t	pad2[0x54];
	u_int8_t	dither;
};

int	com_dino_match(struct device *, struct cfdata *, void *);
void	com_dino_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_dino, sizeof(struct com_dino_softc), com_dino_match, 
    com_dino_attach, NULL, NULL);

int
com_dino_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_FIO ||
	    ca->ca_type.iodc_sv_model != HPPA_FIO_GRS232)
		return (0);

	return (1);
	/* HOZER comprobe1(ca->ca_iot, ca->ca_hpa + IOMOD_DEVOFFSET); */
}

void
com_dino_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	struct com_dino_softc *sc_dino = (void *)self;
	struct confargs *ca = aux;
	struct com_dino_regs *regs = (struct com_dino_regs *)ca->ca_hpa;
	bus_addr_t iobase;
	bus_space_handle_t ioh;

	iobase = (bus_addr_t)ca->ca_hpa + IOMOD_DEVOFFSET;

#ifdef work_in_progress
	/*
	 * XXX: gdamore: this is all wrong, the correct way is to use
	 * com_is_console() as a test, which takes the bus_space_tag
	 * and io base addresses as arguments.  comconsioh is not
	 * present anymore, and was never really exported anyway.
	 *
	 * This code was already ifdef'd out, but if someone wants me
	 * to fix it, let me know and I'll try to do so.  I don't have
	 * hp700 h/w to test with, though.  - gdamore@NetBSD.org
	 */
	if (sc->sc_iobase == CONADDR)
		sc->sc_ioh = comconsioh;
	else 
#endif

	if (bus_space_map(ca->ca_iot, iobase, COM_NPORTS, 0, &ioh)) {
		printf(": cannot map io space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, ca->ca_iot, ioh, iobase);

#ifdef work_in_progress
	/* XXX gdamore: wtf? */
	if (sc->sc_iobase != CONADDR) {
		/* regs->reset = 0xd0;
		DELAY(1000); */
	}
#endif

	/* select clock freq */
	regs->test = COM_DINO_CLK_SEL;
	sc->sc_frequency = COM_DINO_FREQ;

	com_attach_subr(sc);

	sc_dino->sc_ih = dino_intr_establish(parent, ca->ca_irq, IPL_TTY,
	    comintr, sc);
}
