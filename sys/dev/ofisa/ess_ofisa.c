/*	$NetBSD: ess_ofisa.c,v 1.7.8.1 2002/06/20 16:33:26 gehenna Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ess_ofisa.c,v 1.7.8.1 2002/06/20 16:33:26 gehenna Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#include <dev/isa/essreg.h>
#include <dev/isa/essvar.h>

int	ess_ofisa_match __P((struct device *, struct cfdata *, void *));
void	ess_ofisa_attach __P((struct device *, struct device *, void *));

struct cfattach ess_ofisa_ca = {
	sizeof(struct ess_softc), ess_ofisa_match, ess_ofisa_attach
};

int
ess_ofisa_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofisa_attach_args *aa = aux;
	static const char *const compatible_strings[] = {
		"ESST,es1887-codec",		/* ESS 1887 */
		"ESST,es1888-codec",		/* ESS 1888 */
		"ESST,es888-codec",		/* ESS 888 */
		NULL,
	};
	int rv = 0;

	/* XXX table */
	if (of_compatible(aa->oba.oba_phandle, compatible_strings) != -1) {
		rv = 10;
	}

	return (rv);
}

void
ess_ofisa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ess_softc *sc = (void *)self;
	struct ofisa_attach_args *aa = aux;
	struct ofisa_reg_desc reg;
	struct ofisa_intr_desc intr[2];
	struct ofisa_dma_desc dma[2];
	int n, ndrq;
	char *model;

	/*
	 * We're living on an OFW.  We have to ask the OFW what our
	 * registers and interrupts properties look like.
	 *
	 * We expect:
	 *
	 *	1      i/o register region
	 *	1 or 2 interrupts
	 *	2      dma channels
	 */

	n = ofisa_reg_get(aa->oba.oba_phandle, &reg, 1);
	if (n != 1) {
		printf(": error getting register data\n");
		return;
	}
	if (reg.type != OFISA_REG_TYPE_IO) {
		printf(": register type not i/o\n");
		return;
	}
	if (reg.len != ESS_NPORT) {
		printf(": weird register size (%lu, expected %d)\n",
		    (unsigned long)reg.len, ESS_NPORT);
		return;
	}

	n = ofisa_intr_get(aa->oba.oba_phandle, intr, 2);
	if (n == 1) {
		sc->sc_audio1.irq = intr[0].irq;
		sc->sc_audio1.ist = intr[0].share;
		sc->sc_audio2.irq = intr[0].irq;
		sc->sc_audio2.ist = intr[0].share;
	} else if (n == 2) {
		sc->sc_audio1.irq = intr[0].irq;
		sc->sc_audio1.ist = intr[0].share;
		sc->sc_audio2.irq = intr[1].irq;
		sc->sc_audio2.ist = intr[1].share;
	} else {
		printf(": error getting interrupt data\n");
		return;
	}

	ndrq = ofisa_dma_get(aa->oba.oba_phandle, dma, 2);
	if (ndrq != 2) {
		printf(": error getting DMA data\n");
		return;
	}
	sc->sc_audio1.drq = dma[0].drq;
	sc->sc_audio2.drq = dma[1].drq;

	sc->sc_ic = aa->ic;

	sc->sc_iot = aa->iot;
	sc->sc_iobase = reg.addr;
	if (bus_space_map(sc->sc_iot, sc->sc_iobase, reg.len, 0,
	    &sc->sc_ioh)) {
		printf(": unable to map register space\n");
		return;
	}

	/* 
	 * The Shark firmware doesn't program the ESS ISA address registers.
	 * Do that here instead of inside essmatch() since we want to defer
	 * to the firmware on other platforms.
	 */
	if (ess_config_addr(sc))
		return;
	if (essmatch(sc) == 0) {
		printf(": essmatch failed\n");
		return;
	}

	n = OF_getproplen(aa->oba.oba_phandle, "model");
	if (n > 0) {
		model = alloca(n);
		if (OF_getprop(aa->oba.oba_phandle, "model", model, n) == n)
			printf(": %s\n%s", model, sc->sc_dev.dv_xname);
	}

	essattach(sc);
}
