/*	$NetBSD: obio_wdc.c,v 1.6.2.1 2017/12/03 11:35:53 jdolecek Exp $	*/

/* adapted from iq31244/wdc_obio.c:
 *	NetBSD: wdc_obio.c,v 1.5 2008/04/28 20:23:16 martin Exp
 */

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
__KERNEL_RCSID(0, "$NetBSD: obio_wdc.c,v 1.6.2.1 2017/12/03 11:35:53 jdolecek Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/gemini/gemini_var.h>
#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_obiovar.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

struct wdc_obio_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *wdc_chanlist[1];
	struct	ata_channel ata_channel;
	struct	wdc_regs wdc_regs;
	void	*sc_ih;
};

static int	wdc_obio_match(device_t, cfdata_t, void *);
static void	wdc_obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdc_obio, sizeof(struct wdc_obio_softc),
    wdc_obio_match, wdc_obio_attach, NULL, NULL);

static int
wdc_obio_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args * const obio = aux;

	if (obio->obio_addr == GEMINI_MIDE_BASEn(0)
	||  obio->obio_addr == GEMINI_MIDE_BASEn(1))
		return 1;

        return 0;

}

static void
wdc_obio_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_obio_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct obio_attach_args *obio = aux;
	uint chan;
	int i;

	/*
	 * we treat the two channels of the Gemini MIDE controller
	 * as seperate wdc controllers, because they have
	 * independent interrupts.  'chan' here is an MIDE chanel,
	 * (not to be confused with ATA channel).
	 */
	switch (obio->obio_addr) {
	case  GEMINI_MIDE_BASEn(0):
		chan = 0;
		break;
	case  GEMINI_MIDE_BASEn(1):
		chan = 1;
		break;
	default:
		panic("wdc_obio_attach: bad address");
	}

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->wdc_regs;
	wdr->cmd_iot = obio->obio_iot;
	wdr->ctl_iot = obio->obio_iot;
	if (bus_space_map(wdr->cmd_iot, obio->obio_addr, GEMINI_MIDE_SIZE,
	    0, &wdr->cmd_baseioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	for (i = 0; i < 8; i++) {
		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
		    GEMINI_MIDE_CMDBLK + i, 1, &wdr->cmd_iohs[i]) != 0) {
			aprint_error(": couldn't subregion registers\n");
			return;
		}
	}

	if (bus_space_subregion(wdr->ctl_iot, wdr->cmd_baseioh,
	    GEMINI_MIDE_CTLBLK, 1, &wdr->ctl_ioh) != 0) {
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

	wdc_init_shadow_regs(wdr);

	aprint_normal("\n");

	if (obio->obio_intr != OBIOCF_INTR_DEFAULT)
		sc->sc_ih = intr_establish(obio->obio_intr, IPL_BIO, IST_LEVEL_HIGH,
			wdcintr, &sc->ata_channel);
	else {
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_NOIRQ;
		aprint_normal_dev(self, "Using polled I/O\n");
	}

	wdcattach(&sc->ata_channel);
}
