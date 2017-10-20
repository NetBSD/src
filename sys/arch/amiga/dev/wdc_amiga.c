/*	$NetBSD: wdc_amiga.c,v 1.40 2017/10/20 07:06:06 jdolecek Exp $ */

/*-
 * Copyright (c) 2000, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael L. Hitch.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_amiga.c,v 1.40 2017/10/20 07:06:06 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <sys/bswap.h>

#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/gayle.h>
#include <amiga/dev/zbusvar.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

struct wdc_amiga_softc {
	struct wdc_softc sc_wdcdev;
	struct	ata_channel *sc_chanlist[1];
	struct  ata_channel sc_channel;
	struct wdc_regs sc_wdc_regs;
	struct isr sc_isr;
	struct bus_space_tag cmd_iot;
	struct bus_space_tag ctl_iot;
	bool gayle_intr;
};

int	wdc_amiga_probe(device_t, cfdata_t, void *);
void	wdc_amiga_attach(device_t, device_t, void *);
int	wdc_amiga_intr(void *);

CFATTACH_DECL_NEW(wdc_amiga, sizeof(struct wdc_amiga_softc),
    wdc_amiga_probe, wdc_amiga_attach, NULL, NULL);

int
wdc_amiga_probe(device_t parent, cfdata_t cfp, void *aux)
{
	if ((!is_a4000() && !is_a1200() && !is_a600()) ||
	    !matchname(aux, "wdc"))
		return(0);
	return 1;
}

void
wdc_amiga_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_amiga_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	int i;

	aprint_normal("\n");

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	gayle_init();

	if (is_a4000()) {
		sc->cmd_iot.base = (bus_addr_t) ztwomap(GAYLE_IDE_BASE_A4000 + 2);
		sc->gayle_intr = false;
	} else {
		sc->cmd_iot.base = (bus_addr_t) ztwomap(GAYLE_IDE_BASE + 2);
		sc->gayle_intr = true;
	}

	sc->cmd_iot.absm = sc->ctl_iot.absm = &amiga_bus_stride_4swap;
	wdr->cmd_iot = &sc->cmd_iot;
	wdr->ctl_iot = &sc->ctl_iot;

	if (bus_space_map(wdr->cmd_iot, 0, 0x40, 0,
			  &wdr->cmd_baseioh)) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		    wdr->cmd_baseioh, i, i == 0 ? 4 : 1,
		    &wdr->cmd_iohs[i]) != 0) {

			bus_space_unmap(wdr->cmd_iot,
			    wdr->cmd_baseioh, 0x40);
			aprint_error_dev(self, "couldn't map registers\n");
			return;
		}
	}

	if (bus_space_subregion(wdr->cmd_iot,
	    wdr->cmd_baseioh, 0x406, 1, &wdr->ctl_ioh))
		return;

	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = &sc->sc_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->sc_channel.ch_channel = 0;
	sc->sc_channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_init_shadow_regs(wdr);

	sc->sc_isr.isr_intr = wdc_amiga_intr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr (&sc->sc_isr);

	if (sc->gayle_intr)
		gayle_intr_enable_set(GAYLE_INT_IDE);

	wdcattach(&sc->sc_channel);
}

int
wdc_amiga_intr(void *arg)
{
	struct wdc_amiga_softc *sc;
	uint8_t intreq;
	int ret;

	sc = (struct wdc_amiga_softc *)arg;
	ret = 0;
	intreq = gayle_intr_status();

	if (intreq & GAYLE_INT_IDE) {
		if (sc->gayle_intr)
			gayle_intr_ack(0x7C | (intreq & GAYLE_INT_IDEACK));
		ret = wdcintr(&sc->sc_channel);
	}

	return ret;
}

