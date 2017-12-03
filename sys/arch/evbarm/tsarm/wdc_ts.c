/*	$NetBSD: wdc_ts.c,v 1.9.2.1 2017/12/03 11:36:07 jdolecek Exp $ */

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_ts.c,v 1.9.2.1 2017/12/03 11:36:07 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/ep93xx/ep93xxvar.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>
#include <evbarm/tsarm/tspldvar.h>

struct wdc_ts_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *wdc_chanlist[1];
	struct	ata_channel ata_channel;
	struct	wdc_regs wdc_regs;
	void	*sc_ih;
};

static int	wdc_ts_probe(device_t, cfdata_t, void *);
static void	wdc_ts_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdc_ts, sizeof(struct wdc_ts_softc),
    wdc_ts_probe, wdc_ts_attach, NULL, NULL);

static int
wdc_ts_probe(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
wdc_ts_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_ts_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct tspld_attach_args *ta = aux;
	int i;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->wdc_regs;
	wdr->cmd_iot = ta->ta_iot;
	wdr->ctl_iot = ta->ta_iot;
	if (bus_space_map(wdr->cmd_iot, 0x11000000, 8, 0, &wdr->cmd_baseioh) ||
	    bus_space_map(wdr->ctl_iot, 0x10400006, 1, 0, &wdr->ctl_ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	for (i = 0; i < 8; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		      wdr->cmd_baseioh, i, 1, &wdr->cmd_iohs[i]) != 0) {
			aprint_error(": couldn't subregion registers\n");
			return;
		}
	}


	if (bus_space_map(wdr->cmd_iot, 0x21000000, 2, 0, &wdr->cmd_iohs[0])) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_PREATA;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->wdc_chanlist[0] = &sc->ata_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->ata_channel.ch_channel = 0;
	sc->ata_channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_init_shadow_regs(wdr);

	aprint_normal("\n");

	sc->sc_ih = ep93xx_intr_establish(32, IPL_BIO, wdcintr, &sc->ata_channel);

	wdcattach(&sc->ata_channel);
}
