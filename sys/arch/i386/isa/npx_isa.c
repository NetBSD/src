/*	$NetBSD: npx_isa.c,v 1.1.4.2 2000/06/22 17:00:36 minoura Exp $	*/

/*-
 * Copyright (c) 1994, 1995, 1998 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 William Jolitz.
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)npx.c	7.2 (Berkeley) 5/12/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpufunc.h>
#include <machine/intr.h>
#include <machine/specialreg.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <i386/isa/npxvar.h>

int npx_isa_probe(struct device *, struct cfdata *, void *);
void npx_isa_attach(struct device *, struct device *, void *);

struct cfattach npx_isa_ca = {
	sizeof(struct npx_softc), npx_isa_probe, npx_isa_attach
};

int
npx_isa_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;
	enum npx_type result;

	ia->ia_iosize = 16;
	ia->ia_msize = 0;

	if (bus_space_map(ia->ia_iot, 0xf0, ia->ia_iosize, 0, &ioh) != 0)
		return (0);

	result = npxprobe1(ia->ia_iot, ioh, ia->ia_irq);
	if (result != NPX_INTERRUPT)
		ia->ia_irq = IRQUNK;	/* zap the interrupt */

	bus_space_unmap(ia->ia_iot, ioh, ia->ia_iosize);

	return (result != NPX_NONE);
}

void
npx_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct npx_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;

	sc->sc_iot = ia->ia_iot;

	if (bus_space_map(sc->sc_iot, 0xf0, ia->ia_iosize, 0, &sc->sc_ioh)) {
		printf("\n");
		panic("npxattach: unable to map I/O space");
	}

	sc->sc_type = npxprobe1(sc->sc_iot, sc->sc_ioh, ia->ia_irq);

	switch (sc->sc_type) {
	case NPX_INTERRUPT:
		printf("\n");
		lcr0(rcr0() & ~CR0_NE);
		sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq,
		    IST_EDGE, IPL_NONE, npxintr, 0);
		break;
	case NPX_EXCEPTION:
		printf(": using exception 16\n");
		break;
	case NPX_BROKEN:
		printf(": error reporting broken; not using\n");
		sc->sc_type = NPX_NONE;
		return;
	case NPX_NONE:
		return;
	}

	npxattach(sc);
}
