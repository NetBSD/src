/*	$NetBSD: com_mainbus.c,v 1.1.2.2 2002/03/16 15:59:16 jdolecek Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

struct com_mainbus_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

static int	com_mainbus_probe(struct device *, struct cfdata *, void *);
static void	com_mainbus_attach(struct device *, struct device *, void *);

struct cfattach com_mainbus_ca = {
	sizeof(struct com_mainbus_softc), com_mainbus_probe, com_mainbus_attach
};

int comfound = 0;

int
com_mainbus_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	/* match only com devices */
	if (strcmp(maa->mb_name, cf->cf_driver->cd_name) != 0)
		return 0;

	return (comfound < 2);
}

struct com_softc *com0; /* XXX */

void
com_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_mainbus_softc *msc = (void *)self;
	struct com_softc *sc = &msc->sc_com;
	struct mainbus_attach_args *maa = aux;
	int addr = maa->mb_addr;
	int irq = maa->mb_irq;
	
	sc->sc_iot = galaxy_make_bus_space_tag(0, 0);
	sc->sc_iobase = sc->sc_ioh = addr;
	/* UART is clocked externally @ 11.0592MHz == COM_FREQ*6 */
	sc->sc_frequency = COM_FREQ * 6;

	comfound ++;

	/* XXX console check */
	/* XXX map */

	com_attach_subr(sc);

	intr_establish(irq, IST_LEVEL, IPL_SERIAL, comintr, sc);
}
