/*	$NetBSD: kbd_iomd.c,v 1.1 2001/10/05 22:27:41 reinoud Exp $	*/

/*
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * kbd.c
 *
 * Keyboard driver functions
 *
 * Created      : 09/10/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/select.h>
#include <sys/poll.h>

#include <machine/bus.h>
#include <machine/irqhandler.h>
#include <machine/kbd.h>
#include <arm/iomd/iomdvar.h>
#include <arm/iomd/kbdvar.h>

/* Local function prototypes */

static int  kbd_iomd_probe  __P((struct device *, struct cfdata *, void *));
static void kbd_iomd_attach __P((struct device *, struct device *, void *));

extern struct kbd_softc *console_kbd;

/* Device structures */

struct cfattach kbd_iomd_ca = {
	sizeof(struct kbd_softc), kbd_iomd_probe, kbd_iomd_attach
};

static int
kbd_iomd_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct kbd_attach_args *ka = aux;

	if (strcmp(ka->ka_name, "kbd") == 0)
		return(1);

	return(0);
}


static void
kbd_iomd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct kbd_softc *sc = (void *)self;
	struct kbd_attach_args *ka = aux;
	int error;

	sc->sc_iot = ka->ka_iot;
	sc->sc_ioh = ka->ka_ioh;

	error = kbdreset(sc);
	if (error == 1)
		printf(": Cannot enable keyboard");
	else if (error == 2)
		printf(": No keyboard present");

	printf("\n");

	sc->sc_ih = intr_claim(ka->ka_rxirq, IPL_TTY, "kbd rx", kbdintr, sc);
	if (!sc->sc_ih)
		panic("%s: Cannot claim RX interrupt\n", sc->sc_device.dv_xname);

	if (sc->sc_device.dv_unit == 0)
		console_kbd = sc;
}

/* End of kbd_iomd.c */
