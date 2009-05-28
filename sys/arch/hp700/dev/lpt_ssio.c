/*	$OpenBSD: lpt_ssio.c,v 1.1 2007/06/20 18:22:15 kettenis Exp $	*/

/*
 * Copyright (c) 2007 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

#include <hp700/dev/ssiovar.h>

int lpt_ssio_match(device_t, cfdata_t, void *);
void lpt_ssio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lpt_ssio, sizeof(struct lpt_softc),
    lpt_ssio_match, lpt_ssio_attach, NULL, NULL);

int
lpt_ssio_match(device_t parent, cfdata_t match, void *aux)
{
	cfdata_t cf = match;
	struct ssio_attach_args *saa = aux;

	if (strcmp(saa->saa_name, "lpt") != 0)
		return (0);

	/* Check locators. */
	if (cf->ssiocf_irq != SSIO_UNK_IRQ && cf->ssiocf_irq != saa->saa_irq)
		return (0);

	return (1);
}

void
lpt_ssio_attach(device_t parent, device_t self, void *aux)
{
	struct lpt_softc *sc = device_private(self);
	struct ssio_attach_args *saa = aux;

	sc->sc_dev = self;
	sc->sc_iot = saa->saa_iot;
	if (bus_space_map(sc->sc_iot, saa->saa_iobase, LPT_NPORTS,
	    0, &sc->sc_ioh)) {
		aprint_error(": can't map i/o ports\n");
		return;
	}

	lpt_attach_subr(sc);

	sc->sc_ih = ssio_intr_establish(IPL_TTY, saa->saa_irq,
	    lptintr, sc, device_xname(self));

	aprint_normal("\n");
}
