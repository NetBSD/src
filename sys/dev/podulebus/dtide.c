/* $NetBSD: dtide.c,v 1.1 2001/06/08 20:13:24 bjh21 Exp $ */

/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * dtide.c - Driver for the D.T. Software IDE interface.
 */

#include <sys/param.h>

__RCSID("$NetBSD: dtide.c,v 1.1 2001/06/08 20:13:24 bjh21 Exp $");

#include <sys/device.h>
#include <sys/systm.h>
#include <machine/bus.h>

#include <dev/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>
#include <dev/podulebus/dtidereg.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

struct dtide_softc {
	struct wdc_softc sc_wdc;
	struct channel_softc *sc_chp[2]; /* pointers to the following */
	struct channel_softc sc_chan[2];
	void			*sc_ih[2];
	struct evcnt		sc_intrcnt[2];
	bus_space_tag_t		sc_magict;
	bus_space_handle_t	sc_magich;
};

static int dtide_match(struct device *, struct cfdata *, void *);
static void dtide_attach(struct device *, struct device *, void *);

struct cfattach dtide_ca = {
	sizeof(struct dtide_softc), dtide_match, dtide_attach
};

static const char *dtide_intrnames[] = {
	"channel 0 intr", "channel 1 intr",
};

static int
dtide_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct podulebus_attach_args *pa = aux;

	return pa->pa_product == PODULE_DTSOFT_IDE;
}

static void
dtide_attach(struct device *parent, struct device *self, void *aux)
{
	struct podulebus_attach_args *pa = aux;
	struct dtide_softc *sc = (void *)self;
	int i;
	bus_space_tag_t bst;

	sc->sc_wdc.cap = WDC_CAPABILITY_DATA16;
	sc->sc_wdc.PIO_cap = 0; /* XXX correct? */
	sc->sc_wdc.DMA_cap = 0; /* XXX correct? */
	sc->sc_wdc.UDMA_cap = 0;
	sc->sc_wdc.nchannels = 2;
	sc->sc_wdc.channels = sc->sc_chp;
	sc->sc_chp[0] = &sc->sc_chan[0];
	sc->sc_chp[1] = &sc->sc_chan[1];
	sc->sc_magict = pa->pa_fast_t;
	bus_space_map(pa->pa_fast_t, pa->pa_fast_base + DTIDE_MAGICBASE, 0, 1,
	    &sc->sc_magich);
	podulebus_shift_tag(pa->pa_fast_t, DTIDE_REGSHIFT, &bst);
	for (i = 0; i < 2; i++) {
		sc->sc_chan[i].channel = i;
		sc->sc_chan[i].wdc = &sc->sc_wdc;
		sc->sc_chan[i].cmd_iot = bst;
		sc->sc_chan[i].ctl_iot = bst;
	}
	bus_space_map(bst, pa->pa_fast_base + DTIDE_CMDBASE0, 0, 0x100,
	    &sc->sc_chan[0].cmd_ioh);
	bus_space_map(bst, pa->pa_fast_base + DTIDE_CTLBASE0, 0, 0x100,
	    &sc->sc_chan[0].ctl_ioh);
	bus_space_map(bst, pa->pa_fast_base + DTIDE_CMDBASE1, 0, 0x100,
	    &sc->sc_chan[1].cmd_ioh);
	bus_space_map(bst, pa->pa_fast_base + DTIDE_CTLBASE1, 0, 0x100,
	    &sc->sc_chan[1].ctl_ioh);
	printf("\n");
	for (i = 0; i < 2; i++) {
		evcnt_attach_dynamic(&sc->sc_intrcnt[i], EVCNT_TYPE_INTR, NULL,
		    self->dv_xname, dtide_intrnames[i]);
		sc->sc_ih[i] = podulebus_irq_establish(pa->pa_ih, IPL_BIO,
		    wdcintr, &sc->sc_chan[i], &sc->sc_intrcnt[i]);
		wdcattach(&sc->sc_chan[i]);
	}
}
