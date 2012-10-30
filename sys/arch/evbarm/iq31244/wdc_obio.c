/*	$NetBSD: wdc_obio.c,v 1.6.2.1 2012/10/30 17:19:23 yamt Exp $ */

/*-
 * Copyright (c) 1998, 2003, 2005 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_obio.c,v 1.6.2.1 2012/10/30 17:19:23 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/xscale/i80321var.h>
#include <evbarm/iq80321/iq80321reg.h>
#include <evbarm/iq80321/obiovar.h>

#include <evbarm/iq31244/iq31244reg.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

struct wdc_obio_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *wdc_chanlist[1];
	struct	ata_channel ata_channel;
	struct	ata_queue wdc_chqueue;
	struct	wdc_regs wdc_regs;
	void	*sc_ih;
};

static int	wdc_obio_probe(device_t, cfdata_t, void *);
static void	wdc_obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdc_obio, sizeof(struct wdc_obio_softc),
    wdc_obio_probe, wdc_obio_attach, NULL, NULL);

static int
wdc_obio_probe(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
wdc_obio_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_obio_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct obio_attach_args *oba = aux;
	int i;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->wdc_regs;
	wdr->cmd_iot = oba->oba_st;
	wdr->ctl_iot = oba->oba_st;
	if (bus_space_map(wdr->cmd_iot, oba->oba_addr, IQ31244_CFLASH_SIZE,
	    0, &wdr->cmd_baseioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	for (i = 0; i < 8; i++) {
		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
		    IQ31244_CFLASH_CMD_BASE + i, 1, &wdr->cmd_iohs[i]) != 0) {
			aprint_error(": couldn't subregion registers\n");
			return;
		}
	}

	if (bus_space_subregion(wdr->ctl_iot, wdr->cmd_baseioh,
	    IQ31244_CFLASH_CTL_BASE, 1, &wdr->ctl_ioh) != 0) {
		aprint_error(": couldn't subregion registers\n");
		return;
	}

	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->wdc_chanlist[0] = &sc->ata_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->ata_channel.ch_channel = 0;
	sc->ata_channel.ch_atac = &sc->sc_wdcdev.sc_atac;
	sc->ata_channel.ch_queue = &sc->wdc_chqueue;
	wdc_init_shadow_regs(&sc->ata_channel);

	aprint_normal("\n");

	/* 
	 * The interrupt line is controlled by a jumper.  We can't detect 
	 * this, except by allowing a request to time out, so assume that
	 * if the config file hasn't specified the IRQ, then the jumper isn't
	 * fitted.
	 */
	if (oba->oba_irq != -1)
		sc->sc_ih = i80321_intr_establish(oba->oba_irq, IPL_BIO,
			wdcintr, &sc->ata_channel);
	else {
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_NOIRQ;
		aprint_normal_dev(self, "Using polled I/O\n");
	}

	wdcattach(&sc->ata_channel);
}
