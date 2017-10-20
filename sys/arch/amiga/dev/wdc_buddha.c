/*	$NetBSD: wdc_buddha.c,v 1.10 2017/10/20 07:06:06 jdolecek Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael L. Hitch and Ignatios Souvatzis.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bswap.h>

#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/zbusvar.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#define BUDDHA_MAX_CHANNELS 3

struct wdc_buddha_softc {
	struct wdc_softc sc_wdcdev;
	struct ata_channel *wdc_chanarray[BUDDHA_MAX_CHANNELS];
	struct ata_channel channels[BUDDHA_MAX_CHANNELS];
	struct bus_space_tag sc_iot;
	struct isr sc_isr;
	volatile char *ba;
};

int	wdc_buddha_probe(device_t, cfdata_t, void *);
void	wdc_buddha_attach(device_t, device_t, void *);
int	wdc_buddha_intr(void *);

CFATTACH_DECL_NEW(wdc_buddha, sizeof(struct wdc_buddha_softc),
    wdc_buddha_probe, wdc_buddha_attach, NULL, NULL);

int
wdc_buddha_probe(device_t parent, cfdata_t cfp, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != 4626)
		return 0;

	if ((zap->prodid != 0) &&	/* Buddha */
	    (zap->prodid != 42))	/* Catweasel */
		return 0;

	return 1;
}

void
wdc_buddha_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_buddha_softc *sc;
	struct zbus_args *zap;
	int nchannels;
	int ch;

	sc = device_private(self);
	sc->sc_wdcdev.sc_atac.atac_dev = self;
	zap = aux;

	sc->ba = zap->va;

	sc->sc_iot.base = (bus_addr_t)sc->ba;
	sc->sc_iot.absm = &amiga_bus_stride_4swap;

	nchannels = 2;
	if (zap->prodid == 42) {
		aprint_normal(": Catweasel Z2\n");
		nchannels = 3;
	} else if (zap->serno == 0) 
		aprint_normal(": Buddha\n");
	else
		aprint_normal(": Buddha Flash\n");

	/* XXX pio mode setting not implemented yet. */
	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanarray;
	sc->sc_wdcdev.sc_atac.atac_nchannels = nchannels;
	sc->sc_wdcdev.wdc_maxdrives = 2;

	wdc_allocate_regs(&sc->sc_wdcdev);

	for (ch = 0; ch < nchannels; ch++) {
		struct ata_channel *cp;
		struct wdc_regs *wdr;
		int i;

		cp = &sc->channels[ch];
		sc->wdc_chanarray[ch] = cp;

		cp->ch_channel = ch;
		cp->ch_atac = &sc->sc_wdcdev.sc_atac;

		/*
		 * XXX According to the Buddha docs, we should use a method
		 * array that adds 0x40 to the address for byte accesses, to
		 * get the slow timing for command accesses, and the 0x00 
		 * offset for the word (fast) accesses. This will be
		 * reconsidered when implementing setting the timing.
		 *
		 * XXX We also could consider to abuse the 32bit capability, or
		 * 32bit accesses to the words (which will read in two words)
		 * for better performance.
		 *		-is
		 */
		wdr = CHAN_TO_WDC_REGS(cp);

		wdr->cmd_iot = &sc->sc_iot;
		if (bus_space_map(wdr->cmd_iot, 0x210+ch*0x80, 8, 0,
		    &wdr->cmd_baseioh)) {
			aprint_error_dev(self, "couldn't map cmd registers\n");
			return;
		}

		wdr->ctl_iot = &sc->sc_iot;
		if (bus_space_map(wdr->ctl_iot, 0x250+ch*0x80, 2, 0,
		    &wdr->ctl_ioh)) {
			bus_space_unmap(wdr->cmd_iot, wdr->cmd_baseioh, 8);
			aprint_error_dev(self, "couldn't map ctl registers\n");
			return;
		}

		for (i = 0; i < WDC_NREG; i++) {
			if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
			    i, i == 0 ? 4 : 1, &wdr->cmd_iohs[i]) != 0) {
				aprint_error_dev(self,
				    "couldn't subregion cmd regs\n");
				return;
			}
		}

		wdc_init_shadow_regs(wdr);
		wdcattach(cp);
	}

	sc->sc_isr.isr_intr = wdc_buddha_intr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr (&sc->sc_isr);
	sc->ba[0xfc0] = 0;	/* enable interrupts */
}

int
wdc_buddha_intr(void *arg)
{
	struct wdc_buddha_softc *sc = (struct wdc_buddha_softc *)arg;
	int nchannels, i, ret;
	volatile char *p;

	ret = 0;
	nchannels = sc->sc_wdcdev.sc_atac.atac_nchannels;
	p = sc->ba;

	for (i=0; i<nchannels; i++) {
		if (p[0xf00 + 0x40*i] & 0x80) {
			wdcintr(&sc->channels[i]);
			ret = 1;
		}
	}

	return ret;
}
