/* $NetBSD: aucom_aubus.c,v 1.12.6.1 2006/04/22 11:37:41 simonb Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aucom_aubus.c,v 1.12.6.1 2006/04/22 11:37:41 simonb Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <machine/bus.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>
#include <mips/alchemy/include/aubusvar.h>

#include <mips/alchemy/dev/aucomreg.h>
#include <mips/alchemy/dev/aucomvar.h>

struct aucom_aubus_softc {
	struct com_softc sc_com;
	int sc_irq;
	void *sc_ih;
};

static int	aucom_aubus_probe(struct device *, struct cfdata *, void *);
static void	aucom_aubus_attach(struct device *, struct device *, void *);
static int	aucom_aubus_enable(struct com_softc *);
static void	aucom_aubus_disable(struct com_softc *);

CFATTACH_DECL(aucom_aubus, sizeof(struct aucom_aubus_softc),
    aucom_aubus_probe, aucom_aubus_attach, NULL, NULL);

int
aucom_aubus_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct aubus_attach_args *aa = aux;

	/* match only aucom devices */
	if (strcmp(aa->aa_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

void
aucom_aubus_attach(struct device *parent, struct device *self, void *aux)
{
	struct aucom_aubus_softc *asc = (void *)self;
	struct com_softc *sc = &asc->sc_com;
	struct aubus_attach_args *aa = aux;
	int addr = aa->aa_addr;
	
	sc->sc_iot = aa->aa_st;
	sc->sc_iobase = sc->sc_ioh = addr;
	asc->sc_irq = aa->aa_irq[0];

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
	sc->sc_type = COM_TYPE_AU1x00;
	sc->enable = aucom_aubus_enable;
	sc->disable = aucom_aubus_disable;

	/* Enable UART so we can access it. */
	aucom_aubus_enable(sc);
	sc->enabled = 1;

	/* Attach MI com driver. */
	aucom_attach_subr(sc);

	/* Disable UART if it's not the console. (XXX kgdb?) */
	if (!ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		aucom_aubus_disable(sc);
		sc->enabled = 0;
	}
}

int
aucom_aubus_enable(struct com_softc *sc)
{
	struct aucom_aubus_softc *asc = (void *)sc; /* XXX mi prototype */

	/* Ignore requests to enable an already enabled console. */
	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE) && (asc->sc_ih != NULL))
		return (0);

	/* Enable the UART module. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_modctl,
	    UMC_ME | UMC_CE);

	/* Establish the interrupt. */
	asc->sc_ih = au_intr_establish(asc->sc_irq, 0, IPL_SERIAL, IST_LEVEL,
	    aucomintr, sc);
	if (asc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	return (0);
}

void
aucom_aubus_disable(struct com_softc *sc)
{
	struct aucom_aubus_softc *asc = (void *)sc; /* XXX mi prototype */

	/* Ignore requests to disable the console. */
	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
		return;

	/* Disestablish the interrupt. */
	au_intr_disestablish(asc->sc_ih);

	/* Disable the UART module. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_modctl, 0);
}
