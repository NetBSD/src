/* $NetBSD: aucom_aubus.c,v 1.1 2002/07/29 15:39:13 simonb Exp $ */

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

#include <machine/bus.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>
#include <mips/alchemy/include/aubusvar.h>

#include <mips/alchemy/dev/aucomreg.h>
#include <mips/alchemy/dev/aucomvar.h>

struct aucom_aubus_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

static int	aucom_aubus_probe(struct device *, struct cfdata *, void *);
static void	aucom_aubus_attach(struct device *, struct device *, void *);

struct cfattach aucom_aubus_ca = {
	sizeof(struct aucom_aubus_softc), aucom_aubus_probe, aucom_aubus_attach
};

int
aucom_aubus_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct aubus_attach_args *aa = aux;

	/* match only aucom devices */
	if (strcmp(aa->aa_name, cf->cf_driver->cd_name) == 0)
		return (1);

	return (0);
}

void
aucom_aubus_attach(struct device *parent, struct device *self, void *aux)
{
	struct aucom_aubus_softc *msc = (void *)self;
	struct com_softc *sc = &msc->sc_com;
	struct aubus_attach_args *aa = aux;
	void *ih;
	int addr = aa->aa_addr;
	int irq = aa->aa_irq[0];
	
	sc->sc_iot = aa->aa_st;
	sc->sc_iobase = sc->sc_ioh = addr;

	if (aucom_is_console(sc->sc_iot, sc->sc_iobase, &sc->sc_ioh) == 0 &&
	    bus_space_map(sc->sc_iot, sc->sc_iobase, COM_NPORTS, 0,
			  &sc->sc_ioh) != 0) {
		printf(": can't map i/o space\n");
		return;
	}

	/*
	 * The input to the clock divider is the internal pbus clock (1/4 the
	 * processor frequency).  The actual baud rate of the interface will
	 * be pbus_freq / CLKDIV.
	 */
	sc->sc_frequency = curcpu()->ci_cpu_freq / 4;

	sc->sc_hwflags = COM_HW_NO_TXPRELOAD;

	aucom_attach_subr(sc);

	ih = au_intr_establish(irq, 0, IPL_SERIAL, IST_LEVEL, aucomintr, sc);
	if (ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}
}
