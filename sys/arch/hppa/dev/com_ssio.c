/*	$NetBSD: com_ssio.c,v 1.1.10.2 2014/08/20 00:03:04 tls Exp $	*/

/*	$OpenBSD: com_ssio.c,v 1.2 2007/06/24 16:28:39 kettenis Exp $	*/

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
#include <sys/tty.h>

#include <sys/bus.h>
#include <machine/iomod.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <hppa/hppa/machdep.h>
#include <hppa/dev/ssiovar.h>

void *ssio_intr_establish(int, int, int (*)(void *), void *,
    const char *);

#define COM_SSIO_FREQ	1843200

struct com_ssio_softc {
	struct	com_softc sc_com;	/* real "com" softc */
	void	*sc_ih;			/* interrupt handler */
};

int     com_ssio_match(device_t, cfdata_t, void *);
void    com_ssio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_ssio, sizeof(struct com_ssio_softc), com_ssio_match,
    com_ssio_attach, NULL, NULL);

int
com_ssio_match(device_t parent, cfdata_t match, void *aux)
{
	cfdata_t cf = match;
	struct ssio_attach_args *saa = aux;

	if (strcmp(saa->saa_name, "com") != 0)
		return (0);

	/* Check locators. */
	if (cf->ssiocf_irq != SSIO_UNK_IRQ && cf->ssiocf_irq != saa->saa_irq)
		return (0);

	return (1);
}

void
com_ssio_attach(device_t parent, device_t self, void *aux)
{
	struct com_ssio_softc *sc_ssio = device_private(self);
	struct com_softc *sc = &sc_ssio->sc_com;
	struct ssio_attach_args *saa = aux;
	int pagezero_cookie;

	bus_addr_t iobase;
	bus_space_handle_t ioh;
	bus_space_tag_t iot;

	sc->sc_dev = self;
	iobase = saa->saa_iobase;
	iot = saa->saa_iot;
	if (bus_space_map(iot, iobase, COM_NPORTS,
	    0, &ioh)) {
		aprint_error(": can't map I/O space\n");
		return;
	}

        /* Test if this is the console. */
	pagezero_cookie = hppa_pagezero_map();
	if (PAGE0->mem_cons.pz_class == PCL_DUPLEX &&
	    PAGE0->mem_cons.pz_hpa == (struct iomod *)ioh) {
		bus_space_unmap(iot, ioh, COM_NPORTS);
		if (comcnattach(iot, iobase, B9600, COM_SSIO_FREQ,
		    COM_TYPE_NORMAL, 
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0) {
			aprint_error(": can't comcnattach\n");
			hppa_pagezero_unmap(pagezero_cookie);
			return;
		}
	}
	hppa_pagezero_unmap(pagezero_cookie);

	sc->sc_frequency = COM_SSIO_FREQ;
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);
	com_attach_subr(sc);

	sc_ssio->sc_ih = ssio_intr_establish(IPL_TTY, saa->saa_irq,
	    comintr, sc, device_xname(self));
}
