/* $NetBSD: wdc_upc.c,v 1.8 2003/09/25 19:29:49 mycroft Exp $ */
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_upc.c,v 1.8 2003/09/25 19:29:49 mycroft Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ata/atavar.h> /* XXX needed by wdcvar.h */

#include <dev/ic/upcvar.h>
#include <dev/ic/wdcvar.h>

static int wdc_upc_match(struct device *, struct cfdata *, void *);
static void wdc_upc_attach(struct device *, struct device *, void *);

struct wdc_upc_softc {
	struct wdc_softc sc_wdc;
	struct channel_softc *sc_chanptr;
	struct channel_softc sc_channel;
};

CFATTACH_DECL(wdc_upc, sizeof(struct wdc_upc_softc),
    wdc_upc_match, wdc_upc_attach, NULL, NULL);

static int
wdc_upc_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/* upc_submatch does it all anyway */
	return 1;
}

static void
wdc_upc_attach(struct device *parent, struct device *self, void *aux)
{
	struct wdc_upc_softc *sc = (struct wdc_upc_softc *)self;
	struct upc_attach_args *ua = aux;

	sc->sc_wdc.cap = WDC_CAPABILITY_DATA16;
	sc->sc_wdc.PIO_cap = 1; /* XXX ??? */
	sc->sc_wdc.DMA_cap = 0;
	sc->sc_wdc.UDMA_cap = 0;
	sc->sc_wdc.nchannels = 1;
	sc->sc_chanptr = &sc->sc_channel;
	sc->sc_wdc.channels = &sc->sc_chanptr;
	sc->sc_channel.cmd_iot = ua->ua_iot;
	sc->sc_channel.cmd_ioh = ua->ua_ioh;
	sc->sc_channel.ctl_iot = ua->ua_iot;
	sc->sc_channel.ctl_ioh = ua->ua_ioh2;
	sc->sc_channel.channel = 0;
	sc->sc_channel.wdc = &sc->sc_wdc;
	sc->sc_channel.ch_queue = malloc(sizeof(struct channel_queue),
	    M_DEVBUF, M_NOWAIT);
	if (sc->sc_channel.ch_queue == NULL) {
		printf("%s: can't allocate memory for command queue\n",
		sc->sc_wdc.sc_dev.dv_xname);
		return;
	}

	upc_intr_establish(ua->ua_irqhandle, IPL_BIO, wdcintr,
			   &sc->sc_channel);

	printf("\n");

	wdcattach(&sc->sc_wdc);
}
