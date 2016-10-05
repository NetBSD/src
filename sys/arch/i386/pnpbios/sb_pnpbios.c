/* $NetBSD: sb_pnpbios.c,v 1.16.30.1 2016/10/05 20:55:29 skrll Exp $ */
/*
 * Copyright (c) 1999
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sb_pnpbios.c,v 1.16.30.1 2016/10/05 20:55:29 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <sys/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/mulaw.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/isa/sbreg.h>
#include <dev/isa/sbvar.h>

#include <dev/isa/sbdspvar.h>

int sb_pnpbios_match(device_t, cfdata_t, void *);
void sb_pnpbios_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sb_pnpbios, sizeof(struct sbdsp_softc),
    sb_pnpbios_match, sb_pnpbios_attach, NULL, NULL);

int
sb_pnpbios_match(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "NMX2210") &&
	    strcmp(aa->idstr, "CRX0002"))	/* Cyrix XpressAudio */
		return (0);

	return (1);
}

void
sb_pnpbios_attach(device_t parent, device_t self, void *aux)
{
	struct sbdsp_softc *sc = device_private(self);
	struct pnpbiosdev_attach_args *aa = aux;

	sc->sc_dev = self;
	if (pnpbios_io_map(aa->pbt, aa->resc, 0, &sc->sc_iot, &sc->sc_ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	sc->sc_ic = aa->ic;

	/* XXX These are only for setting chip configuration registers. */
	pnpbios_getiobase(aa->pbt, aa->resc, 0, 0, &sc->sc_iobase);

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 0, &sc->sc_irq, NULL)) {
		aprint_error(": can't get IRQ\n");
		return;
	}

	if (pnpbios_getdmachan(aa->pbt, aa->resc, 0, &sc->sc_drq8)) {
		aprint_error(": can't get DMA channel\n");
		return;
	}
	if (pnpbios_getdmachan(aa->pbt, aa->resc, 1, &sc->sc_drq16))
		sc->sc_drq16 = -1;

	aprint_normal("\n");
	pnpbios_print_devres(self, aa);

	aprint_normal("%s", device_xname(self));

	if (!sbmatch(sc, 0, device_cfdata(self))) {
		aprint_error_dev(self, "sbmatch failed\n");
		return;
	}

	sc->sc_ih = pnpbios_intr_establish(aa->pbt, aa->resc, 0, IPL_AUDIO,
					   sbdsp_intr, sc);

	sbattach(sc);
}
