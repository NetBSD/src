/*	$NetBSD: wdc_amiga.c,v 1.28 2006/01/29 21:42:41 dsl Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_amiga.c,v 1.28 2006/01/29 21:42:41 dsl Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
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
	struct	ata_queue sc_chqueue;
	struct wdc_regs sc_wdc_regs;
	struct isr sc_isr;
	volatile u_char *sc_intreg;
	struct bus_space_tag cmd_iot;
	struct bus_space_tag ctl_iot;
	char	sc_a1200;
};

int	wdc_amiga_probe(struct device *, struct cfdata *, void *);
void	wdc_amiga_attach(struct device *, struct device *, void *);
int	wdc_amiga_intr(void *);

CFATTACH_DECL(wdc_amiga, sizeof(struct wdc_amiga_softc),
    wdc_amiga_probe, wdc_amiga_attach, NULL, NULL);

int
wdc_amiga_probe(struct device *parent, struct cfdata *cfp, void *aux)
{
	if ((!is_a4000() && !is_a1200()) || !matchname(aux, "wdc"))
		return(0);
	return 1;
}

void
wdc_amiga_attach(struct device *parent, struct device *self, void *aux)
{
	struct wdc_amiga_softc *sc = (void *)self;
	struct wdc_regs *wdr;
	int i;

	printf("\n");

	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	if (is_a4000()) {
		sc->cmd_iot.base = (u_long)ztwomap(0xdd2020 + 2);
		sc->sc_intreg = (volatile u_char *)ztwomap(0xdd2020 + 0x1000);
		sc->sc_a1200 = 0;
	} else {
		sc->cmd_iot.base = (u_long) ztwomap(0xda0000 + 2);
		sc->ctl_iot.base = (u_long) ztwomap(0xda4000);
		gayle_init();
		sc->sc_intreg = &gayle.intreq;
		sc->sc_a1200 = 1;
	}
	sc->cmd_iot.absm = sc->ctl_iot.absm = &amiga_bus_stride_4swap;
	wdr->cmd_iot = &sc->cmd_iot;
	wdr->ctl_iot = &sc->ctl_iot;

	if (bus_space_map(wdr->cmd_iot, 0, 0x40, 0,
			  &wdr->cmd_baseioh)) {
		printf("%s: couldn't map registers\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
		return;
	}

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		    wdr->cmd_baseioh, i, i == 0 ? 4 : 1,
		    &wdr->cmd_iohs[i]) != 0) {

			bus_space_unmap(wdr->cmd_iot,
			    wdr->cmd_baseioh, 0x40);
			printf("%s: couldn't map registers\n",
			    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
			return;
		}
	}

	if (sc->sc_a1200)
		wdr->ctl_ioh = sc->ctl_iot.base;
	else if (bus_space_subregion(wdr->cmd_iot,
	    wdr->cmd_baseioh, 0x406, 1, &wdr->ctl_ioh))
		return;

	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = &sc->sc_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_channel.ch_channel = 0;
	sc->sc_channel.ch_atac = &sc->sc_wdcdev.sc_atac;
	sc->sc_channel.ch_queue = &sc->sc_chqueue;
	sc->sc_channel.ch_ndrive = 2;

	wdc_init_shadow_regs(&sc->sc_channel);

	sc->sc_isr.isr_intr = wdc_amiga_intr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr (&sc->sc_isr);

	if (sc->sc_a1200)
		gayle.intena |= GAYLE_INT_IDE;

	wdcattach(&sc->sc_channel);
}

int
wdc_amiga_intr(void *arg)
{
	struct wdc_amiga_softc *sc = (struct wdc_amiga_softc *)arg;
	u_char intreq = *sc->sc_intreg;
	int ret = 0;

	if (intreq & GAYLE_INT_IDE) {
		if (sc->sc_a1200)
			gayle.intreq = 0x7c | (intreq & 0x03);
		ret = wdcintr(&sc->sc_channel);
	}

	return ret;
}
