/*	$NetBSD: com_cpcbus.c,v 1.1.6.2 2002/06/20 03:44:30 nathanw Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
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

#include <machine/bus.h>

#include <dev/ic/cpc700reg.h>
#include <dev/ic/cpc700var.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

struct com_cpc_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

static int	com_cpc_match(struct device *, struct cfdata *, void *);
static void	com_cpc_attach(struct device *, struct device *, void *);

struct cfattach com_cpcbus_ca = {
	sizeof(struct com_cpc_softc), com_cpc_match, com_cpc_attach
};

int
com_cpc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct cpcbus_attach_args *caa = aux;

	return (strcmp(caa->cpca_name, "com") == 0);
}

void
com_cpc_attach(struct device *parent, struct device *self, void *aux)
{
	struct cpcbus_attach_args *caa = aux;
	struct com_cpc_softc *sc = (struct com_cpc_softc *)self;
	int iobase = caa->cpca_addr;
	int irq = caa->cpca_irq;

	sc->sc_com.sc_iot = caa->cpca_tag;
	sc->sc_com.sc_iobase = iobase;

	if (!com_is_console(sc->sc_com.sc_iot, iobase, &sc->sc_com.sc_ioh) &&
	    bus_space_map(sc->sc_com.sc_iot, iobase, COM_NPORTS, 0,
			  &sc->sc_com.sc_ioh)) {
		printf("%s: can't map i/o space\n", self->dv_xname);
		return;
	}

	sc->sc_com.sc_frequency = CPC_COM_SPEED(caa->cpca_freq);

	com_attach_subr(&sc->sc_com);

	sc->sc_ih = intr_establish(irq, IST_LEVEL, IPL_SERIAL, comintr,
				   &sc->sc_com);
}
