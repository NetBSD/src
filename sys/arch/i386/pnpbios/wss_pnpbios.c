/* $NetBSD: wss_pnpbios.c,v 1.2.4.1 1999/12/27 18:32:27 wrstuden Exp $ */
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/isa/ad1848var.h>
#include <dev/isa/wssreg.h>
#include <dev/isa/wssvar.h>

int wss_pnpbios_match __P((struct device *, struct cfdata *, void *));
void wss_pnpbios_attach __P((struct device *, struct device *, void *));

struct cfattach wss_pnpbios_ca = {
	sizeof(struct wss_softc), wss_pnpbios_match, wss_pnpbios_attach
};

int
wss_pnpbios_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "NMX2210"))
		return (0);

	return (2); /* beat sb */
}

void
wss_pnpbios_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wss_softc *sc = (void *)self;
	struct pnpbiosdev_attach_args *aa = aux;
	struct audio_attach_args arg;
#if 0
	static u_char interrupt_bits[12] = {
		-1, -1, -1, -1, -1, -1, -1, 0x08, -1, 0x10, 0x18, 0x20
	};
	static u_char dma_bits[4] = {1, 2, 0, 3};
#endif

	if (pnpbios_io_map(aa->pbt, aa->resc, 1, &sc->sc_iot, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}
	if (pnpbios_io_map(aa->pbt, aa->resc, 2, &sc->sc_iot, &sc->sc_opl_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->wss_ic = aa->ic;

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 0, &sc->wss_irq)) {
		printf(": can't get IRQ\n");
		return;
	}

	if (pnpbios_getdmachan(aa->pbt, aa->resc, 0, &sc->wss_playdrq)) {
		printf(": can't get DMA channel\n");
		return;
	}
	if (pnpbios_getdmachan(aa->pbt, aa->resc, 1, &sc->wss_recdrq))
		sc->wss_recdrq = -1;

	sc->sc_ad1848.sc_ad1848.sc_iot = sc->sc_iot;
	bus_space_subregion(sc->sc_iot, sc->sc_ioh, WSS_CODEC, 4,
			    &sc->sc_ad1848.sc_ad1848.sc_ioh);

	printf("\n");
	pnpbios_print_devres(self, aa);

	printf("%s", self->dv_xname);

	if (!ad1848_isa_probe(&sc->sc_ad1848)) {
		printf("%s: ad1848 probe failed\n", self->dv_xname);
		return;
	}

	wssattach(sc);
#if 0
	/* XXX recdrq */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, WSS_CONFIG,
		      (interrupt_bits[sc->wss_irq] | dma_bits[sc->wss_playdrq]));
#endif
	arg.type = AUDIODEV_TYPE_OPL;
	arg.hwif = 0;
	arg.hdl = 0;
	(void)config_found(self, &arg, audioprint);
}

