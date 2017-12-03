/*	$NetBSD: wdc_obio.c,v 1.8.2.1 2017/12/03 11:36:22 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: wdc_obio.c,v 1.8.2.1 2017/12/03 11:36:22 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <landisk/dev/obiovar.h>

struct wdc_obio_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *sc_chanlist[1];
	struct	ata_channel sc_channel;
	struct	wdc_regs sc_wdc_regs;
	void	*sc_ih;
};

static int	wdc_obio_probe(device_t, cfdata_t, void *);
static void	wdc_obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdc_obio, sizeof(struct wdc_obio_softc),
    wdc_obio_probe, wdc_obio_attach, NULL, NULL);

#define	WDC_OBIO_REG_NPORTS	WDC_NREG
#define	WDC_OBIO_REG_SIZE	(WDC_OBIO_REG_NPORTS * 2)
#define	WDC_OBIO_AUXREG_NPORTS	1
#define	WDC_OBIO_AUXREG_SIZE	(WDC_OBIO_AUXREG_NPORTS * 2)
#define	WDC_OBIO_AUXREG_OFFSET	0x2c

static int
wdc_obio_probe(device_t parent, cfdata_t cfp, void *aux)
{
	struct obio_attach_args *oa = aux;
	struct wdc_regs wdr;
	int result = 0;
	int i;

	if (oa->oa_nio < 1)
		return (0);
	if (oa->oa_nirq < 1)
		return (0);

	if (oa->oa_io[0].or_addr == IOBASEUNK)
		return (0);
	if (oa->oa_irq[0].or_irq == IRQUNK)
		return (0);

	wdr.cmd_iot = oa->oa_iot;
	if (bus_space_map(wdr.cmd_iot, oa->oa_io[0].or_addr,
	    WDC_OBIO_REG_SIZE, 0, &wdr.cmd_baseioh)) {
		goto out;
	}
	for (i = 0; i < WDC_OBIO_REG_NPORTS; i++) {
		if (bus_space_subregion(wdr.cmd_iot, wdr.cmd_baseioh,
		    i * 2, (i == 0) ? 2 : 1, &wdr.cmd_iohs[i])) {
			goto outunmap;
		}
	}
	wdc_init_shadow_regs(&wdr);

	wdr.ctl_iot = oa->oa_iot;
	if (bus_space_map(wdr.ctl_iot,
	    oa->oa_io[0].or_addr + WDC_OBIO_AUXREG_OFFSET,
	    WDC_OBIO_AUXREG_SIZE, 0, &wdr.ctl_ioh)) {
		goto outunmap;
	}

	result = wdcprobe(&wdr);
	if (result) {
		oa->oa_nio = 1;
		oa->oa_io[0].or_size = WDC_OBIO_REG_SIZE;
		oa->oa_nirq = 1;
		oa->oa_niomem = 0;
	}

	bus_space_unmap(wdr.ctl_iot, wdr.ctl_ioh, WDC_OBIO_AUXREG_SIZE);
outunmap:
	bus_space_unmap(wdr.cmd_iot, wdr.cmd_baseioh, WDC_OBIO_REG_SIZE);
out:
	return (result);
}

static void
wdc_obio_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_obio_softc *sc = device_private(self);
	struct obio_attach_args *oa = aux;
	struct wdc_regs *wdr;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	wdr->cmd_iot = oa->oa_iot;
	wdr->ctl_iot = oa->oa_iot;
	if (bus_space_map(wdr->cmd_iot, oa->oa_io[0].or_addr,
	    WDC_OBIO_REG_SIZE, 0, &wdr->cmd_baseioh)
	 || bus_space_map(wdr->ctl_iot,
	    oa->oa_io[0].or_addr + WDC_OBIO_AUXREG_OFFSET,
	    WDC_OBIO_AUXREG_SIZE, 0, &wdr->ctl_ioh)) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	for (i = 0; i < WDC_OBIO_REG_NPORTS; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		      wdr->cmd_baseioh, i * 2, (i == 0) ? 2 : 1,
		      &wdr->cmd_iohs[i]) != 0) {
			aprint_error_dev(self,
			    "couldn't subregion registers\n");
			return;
		}
	}

	sc->sc_ih = obio_intr_establish(oa->oa_irq[0].or_irq, IPL_BIO, wdcintr,
	    &sc->sc_channel);

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_PREATA;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = &sc->sc_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->sc_channel.ch_channel = 0;
	sc->sc_channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_init_shadow_regs(wdr);

	wdcattach(&sc->sc_channel);
}
