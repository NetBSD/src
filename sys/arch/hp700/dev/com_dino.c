/*	$OpenBSD: com_dino.c,v 1.4 2007/07/15 19:25:49 kettenis Exp $	*/

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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
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
#include <hp700/hp700/machdep.h>

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

int	com_dino_match(device_t, cfdata_t, void *);
void	com_dino_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_dino, sizeof(struct com_dino_softc), com_dino_match,
    com_dino_attach, NULL, NULL);

int
com_dino_match(device_t parent, cfdata_t match, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_FIO ||
	    ca->ca_type.iodc_sv_model != HPPA_FIO_GRS232)
		return (0);

	return (1);
	/* HOZER comprobe1(ca->ca_iot, ca->ca_hpa + IOMOD_DEVOFFSET); */
}

void
com_dino_attach(device_t parent, device_t self, void *aux)
{
	void *sc_dino = device_private(parent);
	struct com_dino_softc *sc_comdino = device_private(self);
	struct com_softc *sc = &sc_comdino->sc_com;
	struct confargs *ca = aux;
	struct com_dino_regs *regs = (struct com_dino_regs *)ca->ca_hpa;
	int pagezero_cookie;

	bus_addr_t iobase;
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	iobase = (bus_addr_t)ca->ca_hpa + IOMOD_DEVOFFSET;
	sc->sc_frequency = COM_DINO_FREQ;

	/* Test if this is the console.  Compare either HPA or device path. */
	pagezero_cookie = hp700_pagezero_map();
	if (PAGE0->mem_cons.pz_class == PCL_DUPLEX &&
	    PAGE0->mem_cons.pz_hpa == (struct iomod *)ca->ca_hpa) {

		/*
		 * This port is the console.  In this case we must call
		 * comcnattach() and later com_is_console() to initialize
		 * everything properly.
		 */

		if (comcnattach(ca->ca_iot, iobase, B9600,
		    sc->sc_frequency, COM_TYPE_NORMAL,
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0) {
			aprint_error(": can't comcnattach\n");
			hp700_pagezero_unmap(pagezero_cookie);
			return;
		}
	}
	hp700_pagezero_unmap(pagezero_cookie);

	/*
	 * Get the already initialized console ioh via com_is_console() if
	 * this is the console or map the I/O space if this isn't the console.
	 */

	if (!com_is_console(ca->ca_iot, iobase, &ioh) &&
	    bus_space_map(ca->ca_iot, iobase, COM_NPORTS, 0, &ioh) != 0) {
		aprint_error(": can't map I/O space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, ca->ca_iot, ioh, iobase);

	/* select clock freq */
	regs->test = COM_DINO_CLK_SEL;

	com_attach_subr(sc);

	ca->ca_irq = 10;

	sc_comdino->sc_ih = dino_intr_establish(sc_dino, ca->ca_irq, IPL_TTY,
	    comintr, sc);
}
