/* $NetBSD: dtide.c,v 1.23 2006/10/01 21:34:30 bjh21 Exp $ */

/*-
 * Copyright (c) 2000, 2001 Ben Harris
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
/*
 * dtide.c - Driver for the D.T. Software IDE interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dtide.c,v 1.23 2006/10/01 21:34:30 bjh21 Exp $");

#include <sys/param.h>

#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <dev/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>
#include <dev/podulebus/dtidereg.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

struct dtide_softc {
	struct wdc_softc sc_wdc;
	struct ata_channel *sc_chp[DTIDE_NCHANNELS];/* pointers to sc_chan */
	struct ata_channel sc_chan[DTIDE_NCHANNELS];
	struct ata_queue sc_chq[DTIDE_NCHANNELS];
	struct wdc_regs sc_wdc_regs[DTIDE_NCHANNELS];
	bus_space_tag_t		sc_magict;
	bus_space_handle_t	sc_magich;
};

static int dtide_match(struct device *, struct cfdata *, void *);
static void dtide_attach(struct device *, struct device *, void *);

CFATTACH_DECL(dtide, sizeof(struct dtide_softc),
    dtide_match, dtide_attach, NULL, NULL);

static const int dtide_cmdoffsets[] = { DTIDE_CMDBASE0, DTIDE_CMDBASE1 };
static const int dtide_ctloffsets[] = { DTIDE_CTLBASE0, DTIDE_CTLBASE1 };

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
	struct dtide_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct ata_channel *ch;
	int i, j;
	bus_space_tag_t bst;

	sc->sc_wdc.regs = sc->sc_wdc_regs;

	sc->sc_wdc.sc_atac.atac_cap = ATAC_CAP_DATA16 | ATAC_CAP_NOIRQ;
	sc->sc_wdc.sc_atac.atac_pio_cap = 0; /* XXX correct? */
	sc->sc_wdc.sc_atac.atac_nchannels = DTIDE_NCHANNELS;
	sc->sc_wdc.sc_atac.atac_channels = sc->sc_chp;
	sc->sc_magict = pa->pa_fast_t;
	bus_space_map(pa->pa_fast_t, pa->pa_fast_base + DTIDE_MAGICBASE, 0, 1,
	    &sc->sc_magich);
	podulebus_shift_tag(pa->pa_fast_t, DTIDE_REGSHIFT, &bst);
	printf("\n");
	for (i = 0; i < DTIDE_NCHANNELS; i++) {
		ch = sc->sc_chp[i] = &sc->sc_chan[i];
		wdr = &sc->sc_wdc_regs[i];
		ch->ch_channel = i;
		ch->ch_atac = &sc->sc_wdc.sc_atac;
		wdr->cmd_iot = bst;
		wdr->ctl_iot = bst;
		ch->ch_queue = &sc->sc_chq[i];
		ch->ch_ndrive = 2;
		bus_space_map(pa->pa_fast_t,
		    pa->pa_fast_base + dtide_cmdoffsets[i], 0, 8,
		    &wdr->cmd_baseioh);
		for (j = 0; j < WDC_NREG; j++)
			bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
			    j, j == 0 ? 4 : 1, &wdr->cmd_iohs[j]);
		wdc_init_shadow_regs(ch);
		bus_space_map(pa->pa_fast_t,
		    pa->pa_fast_base + dtide_ctloffsets[i], 0, 8,
		    &wdr->ctl_ioh);
		wdcattach(ch);
	}
}
